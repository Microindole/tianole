ARCH_KERNEL_SRCS := \
	$(ARCH_DIR)/kernel/early_log.c

ARCH_KERNEL_OBJS := \
	$(BUILD_DIR)/arch/kernel/entry.o \
	$(patsubst $(ARCH_DIR)/kernel/%.c,$(BUILD_DIR)/arch/kernel/%.o,$(ARCH_KERNEL_SRCS))
