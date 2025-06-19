# CXX Project for Host PC Configuring FPGA DMA-SG

This project provides a C++ implementation for configuring the FPGA DMA-SG (Scatter-Gather) mode, which supports the host to interact with the FPGA through memory-mapped registers. It is suitable for scenarios that require high-performance data transfer.

## Directory Structure

```
dma-sg/
├── include/
│   ├── dma_controller.h
│   └── reg_rw.h
├── dma-sg/
│   └── dma_controller.cpp
├── tools/
│   └── reg_rw.c
├── test/
│   └── test.cpp
└── CMakeLists.txt
```

## Getting Started

### Requirements

- **OS**: Linux (tested on Ubuntu 22.04)
- **CMake**: 3.10 or higher
- **Compiler**: GCC supporting C++17
- **Hardware**: FPGA with XDMA driver loaded
- **Permissions**: Sudo access for device I/O operations

### Preparation

1. Ensure XDMA driver is installed and device node exists:

    ```bash
    ls /dev/xdma*  # Should show /dev/xdma0_user etc.
    ```

2. Update hardware-specific addresses in `test/test.cpp`:

    ```cpp
    bases.DMA_BASE = 0x60000;             // DMA control register base
    bases.XDMA_BIAS = 0x44a00000;         // Address translation offset
    const uint64_t BRAM_BASE = 0x20000;   // Descriptor memory base
    const uint64_t DDR_BASE = 0x80000000; // Data buffer base
    ```

    **Note**:

    - `XDMA_BIAS` (PCIe to AXI Translation) is configured in PCIe XDMA IP at PCIe: BARs.

    - `DDR_BASE` is configured in Vivado Block Design (the same with in cpp).

    - The corresponding addresses of `DMA_BASE` and `BRAM_BASE` configured in Block Design are more than those in cpp with `XDMA_BIAS` added.

3. Custom DMA-SG descriptor chain

    Use cpp ‌`ListNode` (Linked List) data structure to build a descriptor, declared as follows:

    ```cpp
    struct descListNode {
        descriptor_t desc;
        descListNode *next;
        descListNode(descriptor_t x) : desc(x), next(nullptr) {}
        descListNode(descriptor_t x, descListNode *next) : desc(x), next(next) {}
    };
    ```

4. For DMA IP details, refer to official documentation:

    [AXI DMA LogiCORE IP Product Guide (PG021)](https://docs.amd.com/r/en-US/pg021_axi_dma/)

### Instructions

- Complie

```bash
mkdir build && cd build
cmake ..
make
```

- Run

```bash
sudo ./bin/dma_test
```

- Expected output

<img src="assets/output.svg" width=900/>

### Error Handling

If an error is encountered, a description will be output on the command line. Please refer to it and fix it.

For Example:

- Fail writing register

<img src="assets/output1.svg" width=900/>

which may caused by "no sudo" or pcie driver. For more information about **[Read/Write Register]** Errors, enable `#define __LOG` in `tools/reg_rw.c` and rebuild the project.

- Check DMA status error

<img src="assets/output2.svg" width=900/>

captured by the function `DmaController::check_dma_error`.

## Key Components

1. **Memory Register Access**
   - Provides low-level memory-mapped I/O operations
   - Supports both read (`reg_rd`) and write (`reg_wr`) operations
   - Handles endianness conversion automatically
   - Implements memory mapping/unmapping via `mmap`

2. **DMA Controller**
   - Manages scatter-gather descriptor chains
   - Supports MM2S (Memory-to-Stream) and S2MM (Stream-to-Memory) channels
   - Key features:
     - Descriptor list configuration
     - DMA channel reset
     - Error detection and handling
     - Transfer status monitoring

3. **Test Application**
   - Demonstrates DMA controller usage
   - Configures a descriptor chain with 3 entries
   - Performs S2MM followed by MM2S transfers

