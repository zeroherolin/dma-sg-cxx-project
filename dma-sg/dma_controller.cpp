#include <iostream>
#include <chrono>
#include <thread>
#include "dma_controller.h"
#include "reg_rw.h"

DmaController::DmaController(const std::string& device, const base_addrs& bases)
    : device_(device), bases_(bases) {}

void DmaController::readRegister(const char *device, uint64_t addr, uint32_t *value, bool verbose) {
    if (verbose)
        std::cout << "\033[1;35m[Reading register]\033[0m - device: " << device << ", address: 0x" << std::hex << addr << std::endl;
    int err = reg_rd(device, addr, value);
    if (err) {
        std::cerr << "\033[1;31m[Error] Fail reading register at address 0x" << std::hex << addr << ".\033[0m" << std::endl;
        exit(err);
    } else if (verbose)
        std::cout << "- Read value: 0x" << std::hex << *value << std::endl;
}

void DmaController::writeRegister(const char *device, uint64_t addr, uint32_t value) {
    std::cout << "\033[1;35m[Writing register]\033[0m - device: " << device << ", address: 0x" << std::hex << addr << ", value: 0x" << value << std::endl;
    int err = reg_wr(device, addr, value);
    if (err) {
        std::cerr << "\033[1;31m[Error] Fail writing register at address 0x" << std::hex << addr << ".\033[0m" << std::endl;
        exit(err);
    }
}

void DmaController::reset() {
    std::cout << "\033[1;36mSoft resetting DMA...\033[0m" << std::endl;

    writeRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.MM2S_DMACR, masks_.RESET_MASK);
    while (true) {
        uint32_t mm2s_dmacr;
        readRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.MM2S_DMACR, &mm2s_dmacr);
        if ((mm2s_dmacr & masks_.RESET_MASK) == 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "MM2S DMA reset complete." << std::endl;

    writeRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.S2MM_DMACR, masks_.RESET_MASK);
    while (true) {
        uint32_t s2mm_dmacr;
        readRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.S2MM_DMACR, &s2mm_dmacr);
        if ((s2mm_dmacr & masks_.RESET_MASK) == 0) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    std::cout << "S2MM DMA reset complete." << std::endl;
}

void DmaController::fetch_status(DmaChannel channel) {
    uint64_t addr = (channel == DmaChannel::MM2S) ? bases_.DMA_BASE + reg_offsets_.MM2S_DMASR : bases_.DMA_BASE + reg_offsets_.S2MM_DMASR;
    uint32_t *dmasr = (channel == DmaChannel::MM2S) ? &mm2s_dmasr_val_ : &s2mm_dmasr_val_;
    readRegister(device_.c_str(), addr, dmasr, false);
}

void DmaController::check_dma_error(DmaChannel channel) {
    uint32_t dmasr = (channel == DmaChannel::MM2S) ? mm2s_dmasr_val_ : s2mm_dmasr_val_;
    if ((dmasr & masks_.ERROR_MASK) != 0) {
        std::cerr << "\033[1;31m[ERROR] Wrong status register value: 0x" << std::hex << dmasr << "\033[0m" << std::endl;
        if (dmasr & (1 << 4)) std::cerr << "- DMAIntErr: DMA internal error" << std::endl;
        if (dmasr & (1 << 5)) std::cerr << "- DMASlvErr: AXI slave error" << std::endl;
        if (dmasr & (1 << 6)) std::cerr << "- DMADecErr: Address decode error" << std::endl;
        if (dmasr & (1 << 8)) std::cerr << "- SGIntErr: Descriptor complete bit error" << std::endl;
        if (dmasr & (1 << 9)) std::cerr << "- SGSlvErr: SG AXI slave error" << std::endl;
        if (dmasr & (1 << 10)) std::cerr << "- SGDecErr: SG address decode error" << std::endl;
        exit(dmasr & masks_.ERROR_MASK);
    }
}

void DmaController::write_desc(descListNode *head, uint64_t desc_addr) {
    std::cout << "\033[1;36mStarting to write descriptors...\033[0m" << std::endl;

    descListNode *current = head;
    if (current == nullptr) {
        std::cerr << "\033[1;31m[Error] Descriptor list is empty.\033[0m" << std::endl;
        exit(EXIT_FAILURE);
    }

    while (true) {
        std::cout << "Writing descriptor at address 0x" << std::hex << desc_addr << std::endl;
        uint64_t next_desc_addr = static_cast<uint64_t>(current->desc.NXTDESC_MSB) << 32 | current->desc.NXTDESC + bases_.XDMA_BIAS;
        writeRegister(device_.c_str(), desc_addr + offsetof(descriptor_t, NXTDESC), static_cast<uint32_t>(next_desc_addr & 0xFFFFFFFF));
        writeRegister(device_.c_str(), desc_addr + offsetof(descriptor_t, NXTDESC_MSB), static_cast<uint32_t>(next_desc_addr >> 32));
        writeRegister(device_.c_str(), desc_addr + offsetof(descriptor_t, BUFFER_ADDRESS), current->desc.BUFFER_ADDRESS);
        writeRegister(device_.c_str(), desc_addr + offsetof(descriptor_t, BUFFER_ADDRESS_MSB), current->desc.BUFFER_ADDRESS_MSB);
        writeRegister(device_.c_str(), desc_addr + offsetof(descriptor_t, CONTROL), current->desc.CONTROL);
        writeRegister(device_.c_str(), desc_addr + offsetof(descriptor_t, STATUS), current->desc.STATUS);

        if (current->next == nullptr) break;

        desc_addr = static_cast<uint64_t>(current->desc.NXTDESC_MSB) << 32 | current->desc.NXTDESC;
        current = current->next;
    }

    std::cout << "Descriptors written done." << std::endl;
}

void DmaController::start_dma(DmaChannel channel) {
    std::cout << "\033[1;36mStarting DMA for " << (channel == DmaChannel::MM2S ? "MM2S...\033[0m" : "S2MM...\033[0m") << std::endl;

    if (channel == DmaChannel::MM2S) {
        std::cout << "Setting MM2S current descriptor address to 0x" << std::hex << mm2s_curdesc_addr << std::endl;
        writeRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.MM2S_CURDESC, mm2s_curdesc_addr + bases_.XDMA_BIAS);
        std::cout << "Enabling Channel MM2S (DMACR.RS=1) ..." << std::endl;
        uint32_t dmacr_val;
        readRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.MM2S_DMACR, &dmacr_val);
        writeRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.MM2S_DMACR, dmacr_val | masks_.RS_MASK);
        std::cout << "Setting MM2S tail descriptor address to 0x" << std::hex << mm2s_taildesc_addr << std::endl;
        writeRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.MM2S_TAILDESC, mm2s_taildesc_addr + bases_.XDMA_BIAS);
    } else {
        std::cout << "Setting S2MM current descriptor address to 0x" << std::hex << s2mm_curdesc_addr << std::endl;
        writeRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.MM2S_CURDESC, s2mm_curdesc_addr + bases_.XDMA_BIAS);
        std::cout << "Enabling Channel S2MM (DMACR.RS=1) ..." << std::endl;
        uint32_t dmacr_val;
        readRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.S2MM_DMACR, &dmacr_val);
        writeRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.S2MM_DMACR, dmacr_val | masks_.RS_MASK);
        std::cout << "Setting S2MM tail descriptor address to 0x" << std::hex << s2mm_taildesc_addr << std::endl;
        writeRegister(device_.c_str(), bases_.DMA_BASE + reg_offsets_.S2MM_TAILDESC, s2mm_taildesc_addr + bases_.XDMA_BIAS);
    }
    std::cout << (channel == DmaChannel::MM2S ? "MM2S" : "S2MM") << " DMA transfer triggered." << std::endl;
    std::cout << "Waiting for DMA transfer to complete..." << std::endl;

    while (true) {
        fetch_status(channel);
        check_dma_error(channel);
        if ((channel == DmaChannel::MM2S && (mm2s_dmasr_val_ & masks_.HALTED_MASK)) ||
            (channel == DmaChannel::S2MM && (s2mm_dmasr_val_ & masks_.HALTED_MASK))) {
            std::cerr << "\033[1;31m[Error] DMA transfer halted.\033[0m" << std::endl;
            exit(EXIT_FAILURE);
        }
        if ((channel == DmaChannel::MM2S && (mm2s_dmasr_val_ & masks_.IDLE_MASK)) ||
            (channel == DmaChannel::S2MM && (s2mm_dmasr_val_ & masks_.IDLE_MASK))) {
            std::cout << "\033[1;32m" << (channel == DmaChannel::MM2S ? "MM2S" : "S2MM") << " DMA transfer complete.\033[0m" << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
