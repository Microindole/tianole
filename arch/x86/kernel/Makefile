ARCH_KERNEL_SRCS := \
	$(ARCH_DIR)/kernel/early_log.c \
	$(ARCH_DIR)/kernel/gdt.c \
	$(ARCH_DIR)/kernel/idt.c \
	$(ARCH_DIR)/kernel/traps.c

ARCH_KERNEL_OBJS := \
	$(BUILD_DIR)/arch/kernel/entry.o \
	$(BUILD_DIR)/arch/kernel/exception_entry.o \
	$(patsubst $(ARCH_DIR)/kernel/%.c,$(BUILD_DIR)/arch/kernel/%.o,$(ARCH_KERNEL_SRCS))
