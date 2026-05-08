KERNEL_SRCS := \
	kernel/main.c \
	kernel/boot_report.c

KERNEL_OBJS := \
	$(ARCH_KERNEL_OBJS) \
	$(patsubst kernel/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_SRCS))
