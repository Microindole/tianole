ARCH_DIR := arch/x86
EFI_ARCH_TARGET := x86_64-unknown-windows
KERNEL_ARCH_TARGET := x86_64-unknown-none-elf
BOOT_EFI_NAME := BOOTX64.EFI

EFI_DIR := $(BUILD_DIR)/image/EFI/BOOT
BOOT_EFI := $(EFI_DIR)/$(BOOT_EFI_NAME)
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS_TEMPLATE ?= /usr/share/OVMF/OVMF_VARS_4M.fd
OVMF_VARS := $(BUILD_DIR)/OVMF_VARS.fd

CFLAGS := \
	-target $(EFI_ARCH_TARGET) \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone \
	-fno-stack-protector \
	-fno-builtin \
	-Wall \
	-Wextra \
	-Werror \
	-Iinclude \
	-I$(ARCH_DIR)/include

KERNEL_CFLAGS := \
	-target $(KERNEL_ARCH_TARGET) \
	-ffreestanding \
	-fno-stack-protector \
	-fno-builtin \
	-fno-pic \
	-mno-red-zone \
	-DKERNEL_TEST_TRAP=$(KERNEL_TEST_TRAP) \
	-DKERNEL_TEST_PAGE_FAULT=$(KERNEL_TEST_PAGE_FAULT) \
	-DKERNEL_TEST_DOUBLE_FAULT=$(KERNEL_TEST_DOUBLE_FAULT) \
	-DKERNEL_TEST_GENERAL_PROTECTION=$(KERNEL_TEST_GENERAL_PROTECTION) \
	-Wall \
	-Wextra \
	-Werror \
	-I. \
	-Iinclude \
	-Ikernel \
	-I$(ARCH_DIR)/include

KERNEL_ASFLAGS := \
	-target $(KERNEL_ARCH_TARGET) \
	-ffreestanding

LDFLAGS := \
	/subsystem:efi_application \
	/entry:efi_main \
	/nodefaultlib \
	/dll \
	/machine:x64 \
	/out:$(BOOT_EFI)

KERNEL_LDFLAGS := \
	-m elf_x86_64 \
	-nostdlib \
	-T $(ARCH_DIR)/kernel/linker.ld \
	-o $(KERNEL_ELF)
