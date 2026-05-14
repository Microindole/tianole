SHELL := /bin/bash

ARCH ?= x86_64

BUILD_DIR := build
KERNEL_ELF := $(BUILD_DIR)/image/kernel.elf
DEBUG_LOG := $(BUILD_DIR)/debug.log
SERIAL_LOG := $(BUILD_DIR)/serial.log
KERNEL_TEST_TRAP ?= 0
KERNEL_TEST_PAGE_FAULT ?= 0
KERNEL_TEST_DOUBLE_FAULT ?= 0
KERNEL_TEST_GENERAL_PROTECTION ?= 0

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
include drivers/Makefile
include kernel/Makefile

include scripts/Makefile.toolchain
include scripts/Makefile.build
include scripts/Makefile.qemu
