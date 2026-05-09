KERNEL_SRCS := \
	kernel/main.c \
	kernel/boot_report.c \
	kernel/early_log.c

KERNEL_OBJS := \
	$(ARCH_KERNEL_OBJS) \
	$(ARCH_MM_OBJS) \
	$(patsubst kernel/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_SRCS)) \
	$(MM_OBJS)
