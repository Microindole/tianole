#ifndef X86_EFI_H
#define X86_EFI_H

#include <stdint.h>

typedef uint64_t efi_status;
typedef void *efi_handle;
typedef uint16_t efi_char16_t;
typedef uint64_t efi_uintn_t;
typedef uint64_t efi_physical_address_t;

#define EFI_SUCCESS 0
#define EFI_LOAD_ERROR 0x8000000000000001ULL
#define EFI_BUFFER_TOO_SMALL 0x8000000000000005ULL
#define EFI_OUT_OF_RESOURCES 0x8000000000000009ULL
#define EFI_NOT_FOUND 0x800000000000000eULL
#define EFI_INVALID_PARAMETER 0x8000000000000002ULL

#define EFI_ALLOCATE_ADDRESS 2
#define EFI_LOADER_DATA 4
#define EFI_OPEN_MODE_READ 0x0000000000000001ULL

#define EFI_FILE_INFO_ID 0x09576e92
#define EFI_FILE_INFO_ID_A 0x6d3f
#define EFI_FILE_INFO_ID_B 0x11d2
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_A 0x964e5b22
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_B 0x6459
#define EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_C 0x11d2
#define EFI_LOADED_IMAGE_PROTOCOL_GUID_A 0x5b1b31a1
#define EFI_LOADED_IMAGE_PROTOCOL_GUID_B 0x9562
#define EFI_LOADED_IMAGE_PROTOCOL_GUID_C 0x11d2
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_A 0x9042a9de
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_B 0x23dc
#define EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_C 0x4a38

#if defined(__x86_64__)
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

/**
 * struct efi_guid_t - UEFI globally unique identifier layout.
 *
 * The field order matches the UEFI ABI exactly and is used when locating
 * firmware protocols such as Simple File System, Loaded Image and GOP.
 */
typedef struct {
	uint32_t data1;
	uint16_t data2;
	uint16_t data3;
	uint8_t data4[8];
} efi_guid_t;

/**
 * struct efi_table_header_t - Header prefix shared by UEFI tables.
 */
typedef struct {
	uint64_t signature;
	uint32_t revision;
	uint32_t header_size;
	uint32_t crc32;
	uint32_t reserved;
} efi_table_header_t;

typedef struct efi_simple_text_output_protocol
	efi_simple_text_output_protocol_t;
typedef struct efi_boot_services efi_boot_services_t;
typedef struct efi_simple_file_system_protocol
	efi_simple_file_system_protocol_t;
typedef struct efi_file_protocol efi_file_protocol_t;
typedef struct efi_loaded_image_protocol efi_loaded_image_protocol_t;
typedef struct efi_graphics_output_protocol efi_graphics_output_protocol_t;
typedef struct efi_system_table efi_system_table_t;

struct efi_simple_text_output_protocol {
	void *reset;
	efi_status(EFIAPI *output_string)(
		efi_simple_text_output_protocol_t *self, efi_char16_t *string);
};

typedef struct {
	uint64_t size;
	uint64_t file_size;
	uint64_t physical_size;
	uint64_t create_time[2];
	uint64_t last_access_time[2];
	uint64_t modification_time[2];
	uint64_t attribute;
	efi_char16_t file_name[1];
} efi_file_info_t;

/**
 * struct efi_file_protocol - Minimal UEFI file protocol used by boot I/O.
 *
 * Only the methods needed to open, size and read the kernel image are typed.
 * Unused callbacks stay as opaque pointers to preserve table layout.
 */
struct efi_file_protocol {
	uint64_t revision;
	efi_status(EFIAPI *open)(efi_file_protocol_t *self,
		efi_file_protocol_t **new_handle,
		efi_char16_t *file_name,
		uint64_t open_mode,
		uint64_t attributes);
	efi_status(EFIAPI *close)(efi_file_protocol_t *self);
	void *delete_;
	efi_status(EFIAPI *read)(efi_file_protocol_t *self,
		efi_uintn_t *buffer_size,
		void *buffer);
	void *write;
	void *get_position;
	void *set_position;
	efi_status(EFIAPI *get_info)(efi_file_protocol_t *self,
		efi_guid_t *information_type,
		efi_uintn_t *buffer_size,
		void *buffer);
	void *set_info;
	void *flush;
};

struct efi_simple_file_system_protocol {
	uint64_t revision;
	efi_status(EFIAPI *open_volume)(efi_simple_file_system_protocol_t *self,
		efi_file_protocol_t **root);
};

/**
 * struct efi_loaded_image_protocol - UEFI metadata for the running loader.
 *
 * Tianole uses the device handle from this protocol to find the filesystem that
 * contains `kernel.elf`.
 */
struct efi_loaded_image_protocol {
	uint32_t revision;
	efi_handle parent_handle;
	efi_system_table_t *system_table;
	efi_handle device_handle;
	void *file_path;
	void *reserved;
	uint32_t load_options_size;
	void *load_options;
	void *image_base;
	uint64_t image_size;
	uint32_t image_code_type;
	uint32_t image_data_type;
	void *unload;
};

/**
 * struct efi_graphics_output_mode_information_t - GOP visible mode data.
 *
 * The bootloader copies these fields into boot_info so the kernel can attach a
 * framebuffer early console after ExitBootServices().
 */
typedef struct {
	uint32_t version;
	uint32_t horizontal_resolution;
	uint32_t vertical_resolution;
	uint32_t pixel_format;
	uint32_t pixel_information[4];
	uint32_t pixels_per_scan_line;
} efi_graphics_output_mode_information_t;

typedef struct {
	uint32_t max_mode;
	uint32_t mode;
	efi_graphics_output_mode_information_t *info;
	efi_uintn_t size_of_info;
	efi_physical_address_t frame_buffer_base;
	efi_uintn_t frame_buffer_size;
} efi_graphics_output_protocol_mode_t;

struct efi_graphics_output_protocol {
	void *query_mode;
	void *set_mode;
	void *blt;
	efi_graphics_output_protocol_mode_t *mode;
};

/**
 * struct efi_boot_services - UEFI boot-services table subset.
 *
 * Function pointer order must match the firmware ABI. Only services used by
 * the bootloader are typed; other entries remain opaque placeholders so later
 * fields keep their specified offsets.
 */
struct efi_boot_services {
	efi_table_header_t hdr;
	void *raise_tpl;
	void *restore_tpl;
	efi_status(EFIAPI *allocate_pages)(int type,
		int memory_type,
		efi_uintn_t pages,
		efi_physical_address_t *memory);
	void *free_pages;
	efi_status(EFIAPI *get_memory_map)(efi_uintn_t *memory_map_size,
		void *memory_map,
		efi_uintn_t *map_key,
		efi_uintn_t *descriptor_size,
		uint32_t *descriptor_version);
	efi_status(EFIAPI *allocate_pool)(
		int pool_type, efi_uintn_t size, void **buffer);
	efi_status(EFIAPI *free_pool)(void *buffer);
	void *create_event;
	void *set_timer;
	void *wait_for_event;
	void *signal_event;
	void *close_event;
	void *check_event;
	void *install_protocol_interface;
	void *reinstall_protocol_interface;
	void *uninstall_protocol_interface;
	efi_status(EFIAPI *handle_protocol)(
		efi_handle handle, efi_guid_t *protocol, void **interface);
	void *reserved;
	void *register_protocol_notify;
	void *locate_handle;
	void *locate_device_path;
	void *install_configuration_table;
	void *load_image;
	void *start_image;
	void *exit;
	void *unload_image;
	efi_status(EFIAPI *exit_boot_services)(
		efi_handle image_handle, efi_uintn_t map_key);
	void *get_next_monotonic_count;
	void(EFIAPI *stall)(efi_uintn_t microseconds);
	void *set_watchdog_timer;
	void *connect_controller;
	void *disconnect_controller;
	void *open_protocol;
	void *close_protocol;
	void *open_protocol_information;
	void *protocols_per_handle;
	void *locate_handle_buffer;
	efi_status(EFIAPI *locate_protocol)(
		efi_guid_t *protocol, void *registration, void **interface);
};

/**
 * struct efi_system_table - UEFI system table subset used by Tianole boot.
 */
struct efi_system_table {
	efi_table_header_t hdr;
	efi_char16_t *firmware_vendor;
	uint32_t firmware_revision;
	efi_handle console_in_handle;
	void *con_in;
	efi_handle console_out_handle;
	efi_simple_text_output_protocol_t *con_out;
	efi_handle standard_error_handle;
	efi_simple_text_output_protocol_t *std_err;
	void *runtime_services;
	efi_boot_services_t *boot_services;
	uint64_t number_of_table_entries;
	void *configuration_table;
};

/**
 * efi_file_info_guid() - Return the EFI_FILE_INFO protocol GUID.
 */
static inline efi_guid_t efi_file_info_guid(void)
{
	return (efi_guid_t){
		.data1 = EFI_FILE_INFO_ID,
		.data2 = EFI_FILE_INFO_ID_A,
		.data3 = EFI_FILE_INFO_ID_B,
		.data4 = {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b},
	};
}

/**
 * efi_simple_file_system_protocol_guid() - Return Simple File System GUID.
 */
static inline efi_guid_t efi_simple_file_system_protocol_guid(void)
{
	return (efi_guid_t){
		.data1 = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_A,
		.data2 = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_B,
		.data3 = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID_C,
		.data4 = {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b},
	};
}

/**
 * efi_loaded_image_protocol_guid() - Return Loaded Image Protocol GUID.
 */
static inline efi_guid_t efi_loaded_image_protocol_guid(void)
{
	return (efi_guid_t){
		.data1 = EFI_LOADED_IMAGE_PROTOCOL_GUID_A,
		.data2 = EFI_LOADED_IMAGE_PROTOCOL_GUID_B,
		.data3 = EFI_LOADED_IMAGE_PROTOCOL_GUID_C,
		.data4 = {0x8e, 0x3f, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b},
	};
}

/**
 * efi_graphics_output_protocol_guid() - Return Graphics Output Protocol GUID.
 */
static inline efi_guid_t efi_graphics_output_protocol_guid(void)
{
	return (efi_guid_t){
		.data1 = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_A,
		.data2 = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_B,
		.data3 = EFI_GRAPHICS_OUTPUT_PROTOCOL_GUID_C,
		.data4 = {0x96, 0xfb, 0x7a, 0xde, 0xd0, 0x80, 0x51, 0x6a},
	};
}

#endif
