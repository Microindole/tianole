# --- 项目配置 ---
BUILD_DIR = build
SRC_DIRS = cpu drivers fs kernel lib mm

# --- 工具链 ---
CC = gcc
AS = nasm
LD = ld

# --- 编译和链接标志 ---
CFLAGS = -m32 -ffreestanding -c -g -O0 -Wall -Wextra -Wno-unused-parameter $(foreach D,$(SRC_DIRS),-I$(D)) -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# --- 源文件和目标文件 ---
# 自动查找所有 .c 和 .s 文件，但排除 boot.s, isr.s, 和 isr.c
C_SOURCES = $(filter-out %/isr.c, $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.c)))
S_SOURCES = $(filter-out %/boot.s %/isr.s, $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.s)))

C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(C_SOURCES)))
S_OBJS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(notdir $(S_SOURCES)))

# --- 明确列出所有对象文件，避免冲突 ---
OBJS = $(C_OBJS) $(S_OBJS) \
       $(BUILD_DIR)/boot.o \
       $(BUILD_DIR)/isr_s.o \
       $(BUILD_DIR)/isr_c.o

# 最终目标
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_ISO = $(BUILD_DIR)/my-os.iso

# 默认目标
all: $(OS_ISO)

# --- 核心编译链接规则 ---
# 链接内核
$(KERNEL_BIN): $(OBJS) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $^

# VPATH 告诉 make 在哪里寻找源文件
VPATH = $(SRC_DIRS)

# 通用 C/S 编译规则
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# --- 为特殊文件提供精确规则 ---
$(BUILD_DIR)/boot.o: cpu/boot.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/isr_s.o: cpu/isr.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/isr_c.o: cpu/isr.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@


# --- 运行和诊断 ---
run: $(OS_ISO)
	qemu-system-i386 -cdrom $(OS_ISO)

qemu-direct: $(KERNEL_BIN)
	qemu-system-i386 -kernel $(KERNEL_BIN)

# 创建 ISO 镜像
$(OS_ISO): $(KERNEL_BIN) grub.cfg
	@mkdir -p $(BUILD_DIR)/isodir/boot/grub
	cp $(KERNEL_BIN) $(BUILD_DIR)/isodir/boot/
	cp grub.cfg $(BUILD_DIR)/isodir/boot/grub/
	grub-mkrescue -o $@ $(BUILD_DIR)/isodir
	@echo "ISO image created: $@"

# 清理生成的文件
clean:
	rm -rf $(BUILD_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)