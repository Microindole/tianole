#include "efi.h"
#include "tianole/elf.h"
#include "tianole/boot_info.h"

typedef void __attribute__((sysv_abi)) (*kernel_entry_fn_t)(const tianole_boot_info_t *);

static efi_char16_t boot_banner_text[] = {
    'T', 'i', 'a', 'n', 'o', 'l', 'e', ' ',
    'x', '8', '6', ' ', 'b', 'o', 'o', 't',
    'l', 'o', 'a', 'd', 'e', 'r', '.', '\r', '\n', 0
};

static efi_char16_t kernel_path_text[] = {
    '\\', 'k', 'e', 'r', 'n', 'e', 'l', '.',
    'e', 'l', 'f', 0
};

static inline void debug_putc(char ch) {
    __asm__ volatile("outb %0, $0xe9" : : "a"(ch));
}

static void debug_puts(const char *text) {
    while (*text != '\0') {
        if (*text == '\n') {
            debug_putc('\r');
        }
        debug_putc(*text++);
    }
}

static void *mem_copy(void *dst, const void *src, uint64_t size) {
    uint8_t *out = (uint8_t *)dst;
    const uint8_t *in = (const uint8_t *)src;
    uint64_t i;

    for (i = 0; i < size; ++i) {
        out[i] = in[i];
    }

    return dst;
}

static void mem_zero(void *dst, uint64_t size) {
    uint8_t *out = (uint8_t *)dst;
    uint64_t i;

    for (i = 0; i < size; ++i) {
        out[i] = 0;
    }
}

void *memset(void *dst, int value, __SIZE_TYPE__ size) {
    uint8_t *out = (uint8_t *)dst;
    __SIZE_TYPE__ i;

    for (i = 0; i < size; ++i) {
        out[i] = (uint8_t)value;
    }

    return dst;
}

static void debug_put_hex64(uint64_t value) {
    static const char digits[] = "0123456789abcdef";
    int shift;

    debug_puts("0x");
    for (shift = 60; shift >= 0; shift -= 4) {
        debug_putc(digits[(value >> shift) & 0xf]);
    }
}

static efi_status open_root(
    efi_handle image_handle,
    efi_system_table_t *system_table,
    efi_file_protocol_t **root
) {
    efi_guid_t loaded_image_guid = efi_loaded_image_protocol_guid();
    efi_guid_t simple_fs_guid = efi_simple_file_system_protocol_guid();
    efi_loaded_image_protocol_t *loaded_image;
    efi_simple_file_system_protocol_t *fs;
    efi_status status;

    status = system_table->boot_services->handle_protocol(
        image_handle,
        &loaded_image_guid,
        (void **)&loaded_image
    );
    if (status != EFI_SUCCESS) {
        return status;
    }

    status = system_table->boot_services->handle_protocol(
        loaded_image->device_handle,
        &simple_fs_guid,
        (void **)&fs
    );
    if (status != EFI_SUCCESS) {
        return status;
    }

    return fs->open_volume(fs, root);
}

static efi_status read_entire_file(
    efi_system_table_t *system_table,
    efi_file_protocol_t *root,
    efi_char16_t *path,
    void **buffer,
    uint64_t *size
) {
    efi_guid_t file_info_guid = efi_file_info_guid();
    efi_file_protocol_t *file;
    efi_file_info_t *file_info;
    efi_uintn_t info_size;
    efi_uintn_t read_size;
    efi_status status;

    status = root->open(root, &file, path, EFI_OPEN_MODE_READ, 0);
    if (status != EFI_SUCCESS) {
        return status;
    }

    info_size = 0;
    status = file->get_info(file, &file_info_guid, &info_size, 0);
    if (status != EFI_BUFFER_TOO_SMALL) {
        file->close(file);
        return status;
    }

    status = system_table->boot_services->allocate_pool(EFI_LOADER_DATA, info_size, (void **)&file_info);
    if (status != EFI_SUCCESS) {
        file->close(file);
        return status;
    }

    status = file->get_info(file, &file_info_guid, &info_size, file_info);
    if (status != EFI_SUCCESS) {
        file->close(file);
        return status;
    }

    *size = file_info->file_size;
    status = system_table->boot_services->allocate_pool(EFI_LOADER_DATA, *size, buffer);
    if (status != EFI_SUCCESS) {
        file->close(file);
        return status;
    }

    read_size = *size;
    status = file->read(file, &read_size, *buffer);
    file->close(file);
    if (status != EFI_SUCCESS) {
        return status;
    }

    if (read_size != *size) {
        return EFI_LOAD_ERROR;
    }

    return EFI_SUCCESS;
}

static efi_status validate_kernel_elf(const elf64_ehdr_t *ehdr) {
    if (*(const uint32_t *)ehdr->ident != ELF_MAGIC) {
        return EFI_LOAD_ERROR;
    }
    if (ehdr->ident[4] != ELFCLASS64 || ehdr->ident[5] != ELFDATA2LSB) {
        return EFI_LOAD_ERROR;
    }
    if (ehdr->ident[6] != EV_CURRENT || ehdr->version != EV_CURRENT) {
        return EFI_LOAD_ERROR;
    }
    if (ehdr->type != ET_EXEC || ehdr->machine != EM_X86_64) {
        return EFI_LOAD_ERROR;
    }
    if (ehdr->phentsize != sizeof(elf64_phdr_t)) {
        return EFI_LOAD_ERROR;
    }

    return EFI_SUCCESS;
}

static efi_status load_kernel_image(
    efi_system_table_t *system_table,
    const void *kernel_image,
    uint64_t kernel_size,
    kernel_entry_fn_t *entry_out
) {
    const elf64_ehdr_t *ehdr = (const elf64_ehdr_t *)kernel_image;
    const elf64_phdr_t *phdrs;
    uint16_t index;
    efi_status status;

    if (kernel_size < sizeof(*ehdr)) {
        return EFI_LOAD_ERROR;
    }

    status = validate_kernel_elf(ehdr);
    if (status != EFI_SUCCESS) {
        return status;
    }

    phdrs = (const elf64_phdr_t *)((const uint8_t *)kernel_image + ehdr->phoff);

    for (index = 0; index < ehdr->phnum; ++index) {
        const elf64_phdr_t *phdr = &phdrs[index];
        efi_physical_address_t segment_base;
        uint64_t page_count;

        if (phdr->type != PT_LOAD) {
            continue;
        }

        if (phdr->memsz < phdr->filesz) {
            return EFI_LOAD_ERROR;
        }
        if (phdr->offset + phdr->filesz > kernel_size) {
            return EFI_LOAD_ERROR;
        }

        segment_base = phdr->paddr;
        page_count = (phdr->memsz + 0xfffULL) >> 12;
        status = system_table->boot_services->allocate_pages(
            EFI_ALLOCATE_ADDRESS,
            EFI_LOADER_DATA,
            page_count,
            &segment_base
        );
        if (status != EFI_SUCCESS) {
            return status;
        }

        mem_zero((void *)(uintptr_t)phdr->paddr, phdr->memsz);
        mem_copy(
            (void *)(uintptr_t)phdr->paddr,
            (const uint8_t *)kernel_image + phdr->offset,
            phdr->filesz
        );
    }

    *entry_out = (kernel_entry_fn_t)(uintptr_t)ehdr->entry;
    return EFI_SUCCESS;
}

static efi_status fetch_memory_map(
    efi_system_table_t *system_table,
    tianole_boot_info_t *boot_info
) {
    tianole_efi_memory_descriptor_t *memory_map;
    efi_uintn_t memory_map_size;
    efi_uintn_t map_key;
    efi_uintn_t descriptor_size;
    uint32_t descriptor_version;
    efi_status status;

    memory_map_size = 0;
    map_key = 0;
    descriptor_size = 0;
    descriptor_version = 0;
    status = system_table->boot_services->get_memory_map(
        &memory_map_size,
        0,
        &map_key,
        &descriptor_size,
        &descriptor_version
    );
    if (status != EFI_BUFFER_TOO_SMALL) {
        return status;
    }

    memory_map_size += descriptor_size * 8;
    status = system_table->boot_services->allocate_pool(
        EFI_LOADER_DATA,
        memory_map_size,
        (void **)&memory_map
    );
    if (status != EFI_SUCCESS) {
        return status;
    }

    status = system_table->boot_services->get_memory_map(
        &memory_map_size,
        memory_map,
        &map_key,
        &descriptor_size,
        &descriptor_version
    );
    if (status != EFI_SUCCESS) {
        system_table->boot_services->free_pool(memory_map);
        return status;
    }

    boot_info->memory_map = (uint64_t)(uintptr_t)memory_map;
    boot_info->memory_map_size = memory_map_size;
    boot_info->memory_map_key = map_key;
    boot_info->memory_descriptor_size = descriptor_size;
    boot_info->memory_descriptor_version = descriptor_version;

    return EFI_SUCCESS;
}

efi_status EFIAPI efi_main(efi_handle image_handle, efi_system_table_t *system_table) {
    efi_file_protocol_t *root;
    void *kernel_image;
    uint64_t kernel_size;
    kernel_entry_fn_t kernel_entry;
    tianole_boot_info_t boot_info = {
        .version = TIANOLE_BOOT_INFO_VERSION,
        .boot_flags = TIANOLE_BOOT_FLAG_BOOT_SERVICES_ACTIVE,
    };
    efi_status status;

    system_table->con_out->output_string(system_table->con_out, boot_banner_text);
    debug_puts("Tianole x86 bootloader loaded.\n");

    status = open_root(image_handle, system_table, &root);
    if (status != EFI_SUCCESS) {
        debug_puts("failed: open_root\n");
        return status;
    }

    status = read_entire_file(system_table, root, kernel_path_text, &kernel_image, &kernel_size);
    root->close(root);
    if (status != EFI_SUCCESS) {
        debug_puts("failed: read kernel.elf\n");
        return status;
    }

    status = load_kernel_image(system_table, kernel_image, kernel_size, &kernel_entry);
    if (status != EFI_SUCCESS) {
        debug_puts("failed: load kernel image\n");
        return status;
    }

    status = fetch_memory_map(system_table, &boot_info);
    if (status != EFI_SUCCESS) {
        debug_puts("failed: fetch memory map\n");
        return status;
    }

    debug_puts("memory_map descriptors_size=");
    debug_put_hex64(boot_info.memory_descriptor_size);
    debug_puts("\n");
    debug_puts("memory_map size=");
    debug_put_hex64(boot_info.memory_map_size);
    debug_puts("\n");

    debug_puts("jumping to kernel entry\n");
    kernel_entry(&boot_info);

    debug_puts("kernel returned unexpectedly\n");
    for (;;) {
        system_table->boot_services->stall(1000 * 1000);
    }

    return EFI_SUCCESS;
}
