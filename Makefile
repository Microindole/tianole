SHELL := /bin/bash

CC := clang
ELF_LD := ld.lld
LD := lld-link
ARCH ?= x86_64

ifeq ($(ARCH),x86_64)
ARCH_DIR := arch/x86
EFI_ARCH_TARGET := x86_64-unknown-windows
KERNEL_ARCH_TARGET := x86_64-unknown-none-elf
BOOT_EFI_NAME := BOOTX64.EFI
else
$(error Unsupported ARCH '$(ARCH)')
endif

BUILD_DIR := build
EFI_DIR := $(BUILD_DIR)/image/EFI/BOOT
BOOT_EFI := $(EFI_DIR)/$(BOOT_EFI_NAME)
BOOT_OBJ := $(BUILD_DIR)/arch/boot/main.obj
KERNEL_ELF := $(BUILD_DIR)/image/kernel.elf
KERNEL_OBJS := $(BUILD_DIR)/arch/kernel/entry.o $(BUILD_DIR)/kernel/main.o
DEBUG_LOG := $(BUILD_DIR)/debug.log
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
	-Wall \
	-Wextra \
	-Werror \
	-Iinclude \
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

.PHONY: all clean run run-headless dirs

all: $(BOOT_EFI) $(KERNEL_ELF)

dirs:
	mkdir -p $(BUILD_DIR)/arch/boot $(BUILD_DIR)/arch/kernel $(BUILD_DIR)/kernel $(EFI_DIR)

$(BOOT_OBJ): $(ARCH_DIR)/boot/main.c $(ARCH_DIR)/include/efi.h include/tianole/boot_info.h include/tianole/elf.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BOOT_EFI): $(BOOT_OBJ) | dirs
	$(LD) $(LDFLAGS) $<

$(BUILD_DIR)/arch/kernel/entry.o: $(ARCH_DIR)/kernel/entry.S | dirs
	$(CC) $(KERNEL_ASFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/main.o: kernel/main.c include/tianole/boot_info.h | dirs
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(ARCH_DIR)/kernel/linker.ld | dirs
	$(ELF_LD) $(KERNEL_LDFLAGS) $(KERNEL_OBJS)

$(OVMF_VARS): | dirs
	cp $(OVMF_VARS_TEMPLATE) $@

run: $(BOOT_EFI) $(OVMF_VARS)
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS) \
		-drive format=raw,file=fat:rw:$(BUILD_DIR)/image

run-headless: $(BOOT_EFI) $(OVMF_VARS)
	rm -f $(DEBUG_LOG)
	qemu-system-x86_64 \
		-display none \
		-nodefaults \
		-no-reboot \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS) \
		-drive format=raw,file=fat:rw:$(BUILD_DIR)/image \
		-debugcon file:$(DEBUG_LOG) \
		-global isa-debugcon.iobase=0xe9

clean:
	rm -rf $(BUILD_DIR)
