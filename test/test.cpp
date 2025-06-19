#include "dma_controller.h"

int main() {
    std::string device = "/dev/xdma0_user";
    base_addrs bases;
    bases.DMA_BASE = 0x60000;
    bases.XDMA_BIAS = 0x44a00000;

    const uint64_t BRAM_BASE = 0x20000;
    const uint64_t DDR_BASE = 0x80000000;
    uint32_t BUFFER_SIZE = 1024;

    masks msk;

    descriptor_t descriptor[3] = {
        // NXTDESC, NXTDESC_MSB, BUFFER_ADDRESS, BUFFER_ADDRESS_MSB, RESERVED, CONTROL, STATUS, APP
        {0x0, 0x0, 0x0, 0x0, {0}, BUFFER_SIZE | msk.SOF_MASK, 0x0, {0}},
        {0x0, 0x0, 0x0, 0x0, {0}, BUFFER_SIZE | msk.NO_MASK, 0x0, {0}},
        {0x0, 0x0, 0x0, 0x0, {0}, BUFFER_SIZE | msk.EOF_MASK, 0x0, {0}}
    };

    descriptor[0].NXTDESC = static_cast<uint32_t>((BRAM_BASE + 1 * sizeof(descriptor_t)) & 0xFFFFFFFF);
    descriptor[0].NXTDESC_MSB = static_cast<uint32_t>((BRAM_BASE + 1 * sizeof(descriptor_t)) >> 32);
    descriptor[1].NXTDESC = static_cast<uint32_t>((BRAM_BASE + 2 * sizeof(descriptor_t)) & 0xFFFFFFFF);
    descriptor[1].NXTDESC_MSB = static_cast<uint32_t>((BRAM_BASE + 2 * sizeof(descriptor_t)) >> 32);
    descriptor[2].NXTDESC = static_cast<uint32_t>((BRAM_BASE + 0 * sizeof(descriptor_t)) & 0xFFFFFFFF);
    descriptor[2].NXTDESC_MSB = static_cast<uint32_t>((BRAM_BASE + 0 * sizeof(descriptor_t)) >> 32);

    descriptor[0].BUFFER_ADDRESS = static_cast<uint32_t>((DDR_BASE + 0 * BUFFER_SIZE) & 0xFFFFFFFF);
    descriptor[0].BUFFER_ADDRESS_MSB = static_cast<uint32_t>((DDR_BASE + 0 * BUFFER_SIZE) >> 32);
    descriptor[1].BUFFER_ADDRESS = static_cast<uint32_t>((DDR_BASE + 1 * BUFFER_SIZE) & 0xFFFFFFFF);
    descriptor[1].BUFFER_ADDRESS_MSB = static_cast<uint32_t>((DDR_BASE + 1 * BUFFER_SIZE) >> 32);
    descriptor[2].BUFFER_ADDRESS = static_cast<uint32_t>((DDR_BASE + 2 * BUFFER_SIZE) & 0xFFFFFFFF);
    descriptor[2].BUFFER_ADDRESS_MSB = static_cast<uint32_t>((DDR_BASE + 2 * BUFFER_SIZE) >> 32);

    descListNode *descLN = new descListNode(descriptor[0], new descListNode(descriptor[1], new descListNode(descriptor[2])));

    DmaController DmaController(device, bases);

    DmaController.reset();

    DmaController.write_desc(descLN, BRAM_BASE);

    DmaController.mm2s_curdesc_addr = BRAM_BASE + 0 * sizeof(descriptor_t);
    DmaController.mm2s_taildesc_addr = BRAM_BASE + 2 * sizeof(descriptor_t);
    DmaController.s2mm_curdesc_addr = BRAM_BASE + 0 * sizeof(descriptor_t);
    DmaController.s2mm_taildesc_addr = BRAM_BASE + 2 * sizeof(descriptor_t);

    DmaController.start_dma(DmaChannel::S2MM);

    DmaController.write_desc(descLN, BRAM_BASE);

    DmaController.start_dma(DmaChannel::MM2S);

    return 0;
}
