#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdarg.h>
#include <string.h>
#include "reg_rw.h"

// #define __LOG

void iprint(const char *format, ...) {
#ifdef __LOG
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
#else
    (void)format;
#endif
}

static int map_memory(const char *device, uint64_t addr, size_t size, mem_region *region) {
    off_t pgsz, target_aligned;

    /* Check for target page alignment */
    pgsz = sysconf(_SC_PAGESIZE);
    region->offset = addr & (pgsz - 1);
    target_aligned = addr & (~(pgsz - 1));

    /* Open device */
    if ((region->fd = open(device, O_RDWR | O_SYNC)) == -1) {
        iprint("\033[31m[Error] Character device %s open failed: %s.\033[0m\n", device, strerror(errno));
        return -errno;
    }

    /* Map memory */
    region->map_base = mmap(NULL, region->offset + size, PROT_READ | PROT_WRITE, MAP_SHARED,
                            region->fd, target_aligned);
    if (region->map_base == (void *)-1) {
        iprint("\033[31m[Error] Memory 0x%lx mapping failed: %s.\033[0m\n", addr, strerror(errno));
        close(region->fd);
        return -errno;
    }

    region->map_ptr = region->map_base + region->offset;
    return 0;
}

static void unmap_memory(mem_region *region) {
    if (munmap(region->map_base, region->offset + 4) == -1) {
        iprint("\033[31m[Error] Memory unmap failed: %s.\033[0m\n", strerror(errno));
    }
    close(region->fd);
}

static uint32_t read_register(void *map_ptr, uint64_t addr) {
    uint32_t value = *((uint32_t *)map_ptr);
    value = ltohl(value); // Convert byte order
    iprint("Read 32-bit value at address 0x%lx (%p): 0x%08x\n", addr, map_ptr, value);
    return value;
}

static void write_register(void *map_ptr, uint64_t addr, uint32_t value) {
    iprint("Write 32-bit value 0x%08x to 0x%lx (%p)\n", value, addr, map_ptr);
    *((uint32_t *)map_ptr) = htoll(value); // Convert byte order
}

int reg_rd(const char *device, uint64_t addr, uint32_t *value) {
    mem_region region = {0};
    int err = 0;

    iprint("device: %s, address: 0x%lx (0x%lx+0x%lx), access read.\n",
           device, addr, addr - region.offset, region.offset);

    if ((err = map_memory(device, addr, 4, &region)) != 0) {
        return err;
    }

    iprint("Character device %s opened.\n", device);
    iprint("Memory 0x%lx mapped at address %p.\n", addr - region.offset, region.map_base);

    *value = read_register(region.map_ptr, addr);
    unmap_memory(&region);
    return err;
}

int reg_wr(const char *device, uint64_t addr, uint32_t value) {
    mem_region region = {0};
    int err = 0;

    iprint("device: %s, address: 0x%lx (0x%lx+0x%lx), access write.\n",
           device, addr, addr - region.offset, region.offset);

    if ((err = map_memory(device, addr, 4, &region)) != 0) {
        return err;
    }

    iprint("Character device %s opened.\n", device);
    iprint("Memory 0x%lx mapped at address %p.\n", addr - region.offset, region.map_base);

    write_register(region.map_ptr, addr, value);
    unmap_memory(&region);
    return err;
}
