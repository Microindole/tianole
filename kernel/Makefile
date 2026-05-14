kernel-y := \
	main.o \
	boot_report.o \
	early_log.o \
	workqueue.o \
	console/input_console.o \
	debug/kdb.o \
	locking/spinlock.o \
	sched/core.o \
	sched/idle.o \
	sched/thread.o \
	sched/wait.o \
	selftest/page_table.o \
	selftest/sched.o \
	time/timer.o

KERNEL_OBJS := \
	$(ARCH_KERNEL_OBJS) \
	$(ARCH_MM_OBJS) \
	$(addprefix $(BUILD_DIR)/kernel/,$(kernel-y)) \
	$(DRIVER_OBJS) \
	$(MM_OBJS)
