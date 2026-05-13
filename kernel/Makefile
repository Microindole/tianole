KERNEL_SRCS := \
	kernel/main.c \
	kernel/boot_report.c \
	kernel/early_log.c \
	kernel/workqueue.c \
	kernel/locking/spinlock.c \
	kernel/sched/core.c \
	kernel/sched/idle.c \
	kernel/sched/thread.c \
	kernel/sched/wait.c \
	kernel/selftest/page_table.c \
	kernel/selftest/sched.c \
	kernel/time/timer.c

KERNEL_OBJS := \
	$(ARCH_KERNEL_OBJS) \
	$(ARCH_MM_OBJS) \
	$(patsubst kernel/%.c,$(BUILD_DIR)/kernel/%.o,$(KERNEL_SRCS)) \
	$(MM_OBJS)
