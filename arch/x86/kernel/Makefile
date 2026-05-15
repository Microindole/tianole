arch-kernel-asm-y := \
	entry.o \
	exception_entry.o \
	switch.o

arch-kernel-y := \
	early_log.o \
	gdt.o \
	idt.o \
	irq.o \
	screen.o \
	trap_policy.o \
	traps.o

ARCH_KERNEL_OBJS := \
	$(addprefix $(BUILD_DIR)/arch/kernel/,$(arch-kernel-asm-y)) \
	$(addprefix $(BUILD_DIR)/arch/kernel/,$(arch-kernel-y))
