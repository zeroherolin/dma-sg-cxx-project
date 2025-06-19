#ifndef __REG_RW_H__
#define __REG_RW_H__

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ltoh: little endian to host */
/* htol: host to little endian */
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define ltohl(x) (x)
#define htoll(x) (x)
#elif __BYTE_ORDER == __BIG_ENDIAN
#define ltohl(x) __bswap_32(x)
#define htoll(x) __bswap_32(x)
#endif

typedef struct {
    int fd;
    void *map_base;
    void *map_ptr;
    off_t offset;
} mem_region;

int reg_rd(const char *device, uint64_t addr, uint32_t *value);
int reg_wr(const char *device, uint64_t addr, uint32_t value);

#ifdef __cplusplus
}
#endif

#endif
