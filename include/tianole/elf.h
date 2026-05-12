#ifndef ELF_H
#define ELF_H

#include <stdint.h>

/**
 * ELF_MAGIC - ELF file magic value in little-endian word form.
 */
#define ELF_MAGIC 0x464c457fu

/**
 * ELFCLASS64 - ELF identification value for 64-bit objects.
 */
#define ELFCLASS64 2

/**
 * ELFDATA2LSB - ELF identification value for little-endian objects.
 */
#define ELFDATA2LSB 1

/**
 * EV_CURRENT - Current ELF object format version.
 */
#define EV_CURRENT 1

/**
 * ET_EXEC - ELF object type for executable files.
 */
#define ET_EXEC 2

/**
 * EM_X86_64 - ELF machine type for x86_64.
 */
#define EM_X86_64 62

/**
 * PT_LOAD - ELF program header type for loadable segments.
 */
#define PT_LOAD 1

/**
 * typedef elf64_ehdr_t - ELF64 file header.
 * @ident: ELF identification bytes.
 * @type: Object file type.
 * @machine: Target machine architecture.
 * @version: ELF object version.
 * @entry: Entry point virtual address.
 * @phoff: Program header table file offset.
 * @shoff: Section header table file offset.
 * @flags: Processor-specific flags.
 * @ehsize: ELF header size.
 * @phentsize: Size of one program header entry.
 * @phnum: Number of program header entries.
 * @shentsize: Size of one section header entry.
 * @shnum: Number of section header entries.
 * @shstrndx: Section name string table index.
 *
 * Used by the bootloader and future ELF loader to validate and locate image
 * segments without depending on host libc headers.
 */
typedef struct {
	unsigned char ident[16];
	uint16_t type;
	uint16_t machine;
	uint32_t version;
	uint64_t entry;
	uint64_t phoff;
	uint64_t shoff;
	uint32_t flags;
	uint16_t ehsize;
	uint16_t phentsize;
	uint16_t phnum;
	uint16_t shentsize;
	uint16_t shnum;
	uint16_t shstrndx;
} elf64_ehdr_t;

/**
 * typedef elf64_phdr_t - ELF64 program header.
 * @type: Segment type.
 * @flags: Segment permissions.
 * @offset: Segment file offset.
 * @vaddr: Segment virtual address.
 * @paddr: Segment physical address, if meaningful for the image.
 * @filesz: Number of bytes present in the file.
 * @memsz: Number of bytes required in memory.
 * @align: Segment alignment requirement.
 *
 * Describes one loadable or metadata segment consumed by boot and future
 * user-mode ELF loading paths.
 */
typedef struct {
	uint32_t type;
	uint32_t flags;
	uint64_t offset;
	uint64_t vaddr;
	uint64_t paddr;
	uint64_t filesz;
	uint64_t memsz;
	uint64_t align;
} elf64_phdr_t;

#endif
