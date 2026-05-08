#ifndef TIANOLE_ARCH_X86_EFI_H
#define TIANOLE_ARCH_X86_EFI_H

#include <stdint.h>

typedef uint64_t efi_status;
typedef void *efi_handle;
typedef uint16_t efi_char16_t;
typedef uint64_t efi_uintn_t;

#define EFI_SUCCESS 0

#if defined(__x86_64__)
#define EFIAPI __attribute__((ms_abi))
#else
#define EFIAPI
#endif

typedef struct {
    uint64_t signature;
    uint32_t revision;
    uint32_t header_size;
    uint32_t crc32;
    uint32_t reserved;
} efi_table_header_t;

typedef struct efi_simple_text_output_protocol efi_simple_text_output_protocol_t;
typedef struct efi_boot_services efi_boot_services_t;

struct efi_simple_text_output_protocol {
    void *reset;
    efi_status(EFIAPI *output_string)(
        efi_simple_text_output_protocol_t *self,
        efi_char16_t *string
    );
};

struct efi_boot_services {
    efi_table_header_t hdr;
    void *raise_tpl;
    void *restore_tpl;
    void *allocate_pages;
    void *free_pages;
    void *get_memory_map;
    void *allocate_pool;
    void *free_pool;
    void *create_event;
    void *set_timer;
    void *wait_for_event;
    void *signal_event;
    void *close_event;
    void *check_event;
    void *install_protocol_interface;
    void *reinstall_protocol_interface;
    void *uninstall_protocol_interface;
    void *handle_protocol;
    void *reserved;
    void *register_protocol_notify;
    void *locate_handle;
    void *locate_device_path;
    void *install_configuration_table;
    void *load_image;
    void *start_image;
    void *exit;
    void *unload_image;
    void *exit_boot_services;
    void *get_next_monotonic_count;
    void(EFIAPI *stall)(efi_uintn_t microseconds);
    void *set_watchdog_timer;
};

typedef struct {
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
} efi_system_table_t;

#endif
