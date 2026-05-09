SHELL := /bin/bash

ARCH ?= x86_64

CC := clang
ELF_LD := ld.lld
LD := lld-link

BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/image/kernel.elf
DEBUG_LOG := $(BUILD_DIR)/debug.log
SERIAL_LOG := $(BUILD_DIR)/serial.log
KERNEL_TEST_TRAP ?= 0

ifeq ($(ARCH),x86_64)
ARCH_MAKEFILE := arch/x86/Makefile
else
$(error Unsupported ARCH '$(ARCH)')
endif

include $(ARCH_MAKEFILE)
include $(ARCH_DIR)/boot/Makefile
include $(ARCH_DIR)/kernel/Makefile
include $(ARCH_DIR)/mm/Makefile
include mm/Makefile
include kernel/Makefile

.PHONY: all clean dirs run run-headless

all: $(BOOT_EFI) $(KERNEL_ELF)

dirs:
	mkdir -p $(BUILD_DIR)/arch/boot $(BUILD_DIR)/arch/kernel $(BUILD_DIR)/arch/mm $(BUILD_DIR)/kernel $(BUILD_DIR)/mm $(EFI_DIR)

$(BUILD_DIR)/arch/boot/%.obj: $(ARCH_DIR)/boot/%.c $(ARCH_DIR)/include/efi.h $(ARCH_DIR)/boot/debug_log.h include/tianole/boot_info.h include/tianole/elf.h | dirs
	$(CC) $(CFLAGS) -c $< -o $@

$(BOOT_EFI): $(BOOT_OBJS) | dirs
	$(LD) $(LDFLAGS) $(BOOT_OBJS)

$(BUILD_DIR)/arch/kernel/entry.o: $(ARCH_DIR)/kernel/entry.S | dirs
	$(CC) $(KERNEL_ASFLAGS) -c $< -o $@

$(BUILD_DIR)/arch/kernel/%.o: $(ARCH_DIR)/kernel/%.S | dirs
	$(CC) $(KERNEL_ASFLAGS) -c $< -o $@

$(BUILD_DIR)/arch/kernel/%.o: $(ARCH_DIR)/kernel/%.c include/tianole/arch.h | dirs
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/arch/mm/%.o: $(ARCH_DIR)/mm/%.c include/tianole/early_log.h include/tianole/mm.h | dirs
	mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/kernel/%.o: kernel/%.c include/tianole/boot_info.h include/tianole/kernel_init.h include/tianole/arch.h include/tianole/early_log.h include/tianole/mm.h | dirs
	mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(BUILD_DIR)/mm/%.o: mm/%.c include/tianole/boot_info.h include/tianole/early_log.h include/tianole/mm.h | dirs
	mkdir -p $(@D)
	$(CC) $(KERNEL_CFLAGS) -c $< -o $@

$(KERNEL_ELF): $(KERNEL_OBJS) $(ARCH_DIR)/kernel/linker.ld | dirs
	$(ELF_LD) $(KERNEL_LDFLAGS) $(KERNEL_OBJS)

$(OVMF_VARS): | dirs
	cp $(OVMF_VARS_TEMPLATE) $@

run: $(BOOT_EFI) $(OVMF_VARS)
	qemu-system-x86_64 \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS) \
		-drive format=raw,file=fat:rw:$(BUILD_DIR)/image \
		-serial stdio

run-headless: $(BOOT_EFI) $(OVMF_VARS)
	rm -f $(DEBUG_LOG) $(SERIAL_LOG)
	qemu-system-x86_64 \
		-display none \
		-nodefaults \
		-no-reboot \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive if=pflash,format=raw,file=$(OVMF_VARS) \
		-drive format=raw,file=fat:rw:$(BUILD_DIR)/image \
		-debugcon file:$(DEBUG_LOG) \
		-global isa-debugcon.iobase=0xe9 \
		-serial file:$(SERIAL_LOG)

clean:
	rm -rf $(BUILD_DIR)
