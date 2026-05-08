#include "elf_loader.h"

#include "tianole/elf.h"

static void *mem_copy(void *dst, const void *src, uint64_t size)
{
	uint8_t *out = (uint8_t *)dst;
	const uint8_t *in = (const uint8_t *)src;
	uint64_t i;

	for (i = 0; i < size; ++i) {
		out[i] = in[i];
	}

	return dst;
}

static void mem_zero(void *dst, uint64_t size)
{
	uint8_t *out = (uint8_t *)dst;
	uint64_t i;

	for (i = 0; i < size; ++i) {
		out[i] = 0;
	}
}

static efi_status validate_kernel_elf(const elf64_ehdr_t *ehdr)
{
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

efi_status boot_load_kernel_elf(efi_system_table_t *system_table,
	const void *kernel_image,
	uint64_t kernel_size,
	kernel_entry_fn_t *entry_out)
{
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

	phdrs = (const elf64_phdr_t *)((const uint8_t *)kernel_image +
		ehdr->phoff);

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
			&segment_base);
		if (status != EFI_SUCCESS) {
			return status;
		}

		mem_zero((void *)(uintptr_t)phdr->paddr, phdr->memsz);
		mem_copy((void *)(uintptr_t)phdr->paddr,
			(const uint8_t *)kernel_image + phdr->offset,
			phdr->filesz);
	}

	*entry_out = (kernel_entry_fn_t)(uintptr_t)ehdr->entry;
	return EFI_SUCCESS;
}
