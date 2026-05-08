SHELL := /bin/bash

CC := clang
LD := lld-link

BUILD_DIR := build
EFI_DIR := $(BUILD_DIR)/image/EFI/BOOT
BOOT_EFI := $(EFI_DIR)/BOOTX64.EFI
BOOT_OBJ := $(BUILD_DIR)/boot/main.obj
DEBUG_LOG := $(BUILD_DIR)/debug.log
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE_4M.fd
OVMF_VARS_TEMPLATE ?= /usr/share/OVMF/OVMF_VARS_4M.fd
OVMF_VARS := $(BUILD_DIR)/OVMF_VARS.fd

CFLAGS := \
	-target x86_64-unknown-windows \
	-ffreestanding \
	-fshort-wchar \
	-mno-red-zone \
	-fno-stack-protector \
	-fno-builtin \
	-Wall \
	-Wextra \
	-Werror \
	-Iinclude

LDFLAGS := \
	/subsystem:efi_application \
	/entry:efi_main \
	/nodefaultlib \
	/dll \
	/machine:x64 \
	/out:$(BOOT_EFI)

.PHONY: all clean run run-headless dirs

all: $(BOOT_EFI)

dirs:
	mkdir -p $(BUILD_DIR)/boot $(EFI_DIR)

$(BOOT_OBJ): boot/main.c include/efi.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BOOT_EFI): $(BOOT_OBJ) | dirs
	$(LD) $(LDFLAGS) $<

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
