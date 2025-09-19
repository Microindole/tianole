# --- 项目配置 ---
BUILD_DIR = build
SRC_DIRS = cpu drivers fs kernel lib mm

# --- 工具链 ---
CC = gcc
AS = nasm
LD = ld

# --- 编译和链接标志 ---
CFLAGS = -m32 -ffreestanding -c -g -O0 -Wall -Wextra -Wno-unused-parameter -fno-omit-frame-pointer -I. -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# --- 源文件查找 ---
# 自动查找所有 .c 和 .s 文件，但排除需要特殊处理的文件
C_SOURCES = $(filter-out cpu/isr.c, $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.c)))
S_SOURCES = $(filter-out cpu/boot.s cpu/isr.s cpu/paging.s kernel/switch.s cpu/fork_trampoline.s cpu/task_utils.s, $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.s)))

# 将通用源文件映射到目标文件
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
S_OBJS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(S_SOURCES))

# --- 明确列出所有目标文件，避免任何冲突 ---
OBJS = $(C_OBJS) $(S_OBJS) \
       $(BUILD_DIR)/boot.o \
       $(BUILD_DIR)/isr_s.o \
       $(BUILD_DIR)/isr_c.o \
       $(BUILD_DIR)/paging_s.o \
       $(BUILD_DIR)/switch.o \
       $(BUILD_DIR)/fork_trampoline.o

# 最终目标
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_ISO = $(BUILD_DIR)/my-os.iso

# 默认目标
all: $(OS_ISO)

# --- 核心编译链接规则 ---
$(KERNEL_BIN): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# --- 通用编译规则 ---
# VPATH 告诉 make 在哪里寻找源文件
VPATH = $(SRC_DIRS)
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# --- 为特殊文件提供精确的编译规则 ---
$(BUILD_DIR)/boot.o: cpu/boot.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/isr_s.o: cpu/isr.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/isr_c.o: cpu/isr.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@
$(BUILD_DIR)/paging_s.o: cpu/paging.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/switch.o: kernel/switch.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/fork_trampoline.o: cpu/fork_trampoline.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@
$(BUILD_DIR)/task_utils.o: cpu/task_utils.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# --- 运行和诊断 ---
run: $(OS_ISO)
	qemu-system-i386 -cdrom $(OS_ISO)
qemu-direct: $(KERNEL_BIN)
	qemu-system-i386 -kernel $(KERNEL_BIN)

# --- ISO 创建和清理 ---
$(OS_ISO): $(KERNEL_BIN) grub.cfg
	@mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(KERNEL_BIN) $(BUILD_DIR)/isodir/boot/
	cp grub.cfg $(BUILD_DIR)/isodir/boot/grub/
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir
	@echo "ISO image created: $@"
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all run qemu-direct clean