#ifndef __DMA_CONTROLLER_H__
#define __DMA_CONTROLLER_H__

#include <string>
#include <cstdint>

struct base_addrs {
    uint64_t DMA_BASE;
    uint64_t XDMA_BIAS;
};

struct reg_offsets {
    const int64_t MM2S_DMACR = 0x00;
    const int64_t MM2S_DMASR = 0x04;
    const int64_t MM2S_CURDESC = 0x08;
    const int64_t MM2S_CURDESC_MSB = 0x0c;
    const int64_t MM2S_TAILDESC = 0x10;
    const int64_t MM2S_TAILDESC_MSB = 0x14;
    
    const int64_t S2MM_DMACR = 0x30;
    const int64_t S2MM_DMASR = 0x34;
    const int64_t S2MM_CURDESC = 0x38;
    const int64_t S2MM_CURDESC_MSB = 0x3c;
    const int64_t S2MM_TAILDESC = 0x40;
    const int64_t S2MM_TAILDESC_MSB = 0x44;
};

struct masks {
    const uint32_t NO_MASK = 0;
    const uint32_t SOF_MASK = (1 << 27);   // TXSOF/RXSOF
    const uint32_t EOF_MASK = (1 << 26);   // TXEOF/RXEOF
    const uint32_t RS_MASK = (1 << 0);     // DMACR.RS
    const uint32_t RESET_MASK = (1 << 2);  // DMACR.Reset
    const uint32_t IDLE_MASK = (1 << 1);   // DMASR.Idle
    const uint32_t HALTED_MASK = (1 << 0); // DMASR.Halted
    const uint32_t ERROR_MASK = (0x7 << 4) | (0x7 << 8);
};

typedef struct {
    uint32_t NXTDESC;
    uint32_t NXTDESC_MSB;
    uint32_t BUFFER_ADDRESS;
    uint32_t BUFFER_ADDRESS_MSB;
    uint32_t RESERVED[2] = {0};
    uint32_t CONTROL;
    uint32_t STATUS = 0;
    uint32_t APP[5] = {0};
} __attribute__((aligned(64))) descriptor_t;

struct descListNode {
    descriptor_t desc;
    descListNode *next;
    descListNode(descriptor_t x) : desc(x), next(nullptr) {}
    descListNode(descriptor_t x, descListNode *next) : desc(x), next(next) {}
};

enum class DmaChannel {
    MM2S = 0, // Memory Map to Stream
    S2MM = 1  // Stream to Memory Map
};

class DmaController {
public:
    DmaController(const std::string& device, const base_addrs& bases);

    uint64_t mm2s_curdesc_addr;
    uint64_t mm2s_taildesc_addr;
    uint64_t s2mm_curdesc_addr;
    uint64_t s2mm_taildesc_addr;

    void reset();
    void write_desc(descListNode *head, uint64_t desc_addr);
    void start_dma(DmaChannel channel);

private:
    std::string device_;
    base_addrs bases_;
    reg_offsets reg_offsets_;
    masks masks_;
    uint32_t mm2s_dmasr_val_;
    uint32_t s2mm_dmasr_val_;

    void readRegister(const char *device, uint64_t addr, uint32_t *value, bool verbose = true);
    void writeRegister(const char *device, uint64_t addr, uint32_t value);
    void fetch_status(DmaChannel channel);
    void check_dma_error(DmaChannel channel);
};

#endif
