#ifndef TIANOLE_BOOT_INFO_H
#define TIANOLE_BOOT_INFO_H

#include <stdint.h>

#define TIANOLE_EFI_MEMORY_TYPE_CONVENTIONAL 7u

typedef struct {
    uint32_t type;
    uint32_t pad;
    uint64_t physical_start;
    uint64_t virtual_start;
    uint64_t number_of_pages;
    uint64_t attribute;
} tianole_efi_memory_descriptor_t;

/*
 * Boot-time handoff data owned by Tianole rather than by a specific
 * firmware or architecture API. Fields can grow over time while the
 * kernel entry stays stable.
 */
typedef struct {
    uint32_t version;
    uint32_t boot_flags;
    uint64_t memory_map;
    uint64_t memory_map_size;
    uint64_t memory_map_key;
    uint64_t memory_descriptor_size;
    uint32_t memory_descriptor_version;
    uint32_t reserved0;
} tianole_boot_info_t;

#define TIANOLE_BOOT_INFO_VERSION 1u
#define TIANOLE_BOOT_FLAG_BOOT_SERVICES_ACTIVE (1u << 0)

#endif
