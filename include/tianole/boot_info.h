#ifndef BOOT_INFO_H
#define BOOT_INFO_H

#include <stdint.h>

/**
 * BOOT_MEMORY_TYPE_CONVENTIONAL - Firmware memory type for usable RAM.
 *
 * Mirrors the UEFI conventional memory type used by the bootloader handoff.
 * The kernel converts this into its own long-term memory model during MM init.
 */
#define BOOT_MEMORY_TYPE_CONVENTIONAL 7u

/**
 * typedef boot_memory_descriptor_t - Boot memory map descriptor.
 * @type: Firmware memory type.
 * @pad: Reserved padding used to keep the handoff layout stable.
 * @physical_start: Physical base address of this region.
 * @virtual_start: Firmware virtual address field, currently preserved only.
 * @number_of_pages: Region size in 4 KiB pages.
 * @attribute: Firmware attributes for the region.
 *
 * Describes one memory map entry copied from firmware-owned data into the
 * Tianole boot handoff format.
 */
typedef struct {
	uint32_t type;
	uint32_t pad;
	uint64_t physical_start;
	uint64_t virtual_start;
	uint64_t number_of_pages;
	uint64_t attribute;
} boot_memory_descriptor_t;

/**
 * typedef boot_info_t - Bootloader-to-kernel handoff data.
 * @version: Handoff structure version.
 * @boot_flags: Flags describing the firmware handoff state.
 * @memory_map: Physical address of boot_memory_descriptor_t entries.
 * @memory_map_size: Total memory map size in bytes.
 * @memory_map_key: Firmware key captured before ExitBootServices().
 * @memory_descriptor_size: Size of each memory descriptor entry.
 * @memory_descriptor_version: Firmware descriptor version.
 * @reserved0: Reserved field for alignment and future expansion.
 * @framebuffer_base: Physical base address of the boot framebuffer.
 * @framebuffer_size: Framebuffer size in bytes.
 * @framebuffer_width: Visible framebuffer width in pixels.
 * @framebuffer_height: Visible framebuffer height in pixels.
 * @framebuffer_pixels_per_scan_line: Physical pixels per scan line.
 * @framebuffer_pixel_format: Bootloader-provided pixel format identifier.
 *
 * Boot-time handoff data owned by Tianole rather than by a specific firmware
 * or architecture API. Fields can grow while the kernel entry stays stable.
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
	uint64_t framebuffer_base;
	uint64_t framebuffer_size;
	uint32_t framebuffer_width;
	uint32_t framebuffer_height;
	uint32_t framebuffer_pixels_per_scan_line;
	uint32_t framebuffer_pixel_format;
} boot_info_t;

/**
 * BOOT_INFO_VERSION - Current boot_info_t layout version.
 */
#define BOOT_INFO_VERSION 2u

/**
 * BOOT_FLAG_SERVICES_ACTIVE - Firmware boot services were active at handoff.
 */
#define BOOT_FLAG_SERVICES_ACTIVE (1u << 0)

#endif
