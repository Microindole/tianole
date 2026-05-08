#ifndef TIANOLE_BOOT_INFO_H
#define TIANOLE_BOOT_INFO_H

#include <stdint.h>

/*
 * Boot-time handoff data owned by Tianole rather than by a specific
 * firmware or architecture API. Fields can grow over time while the
 * kernel entry stays stable.
 */
typedef struct {
    uint32_t version;
    uint32_t boot_flags;
} tianole_boot_info_t;

#define TIANOLE_BOOT_INFO_VERSION 1u

#endif
