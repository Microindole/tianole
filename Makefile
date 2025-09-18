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
# 关键修复：从自动查找中排除 isr.c 和 isr.s
C_SOURCES = $(filter-out %/isr.c, $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.c)))
S_SOURCES = $(filter-out %/isr.s, $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.s)))

# 根据源文件生成对象文件路径
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(C_SOURCES)))
S_OBJS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(notdir $(S_SOURCES)))

# 将所有对象文件汇总，并手动添加特殊情况
OBJS = $(S_OBJS) $(C_OBJS) $(BUILD_DIR)/isr.o $(BUILD_DIR)/isr_c.o

# 最终目标
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_ISO = $(BUILD_DIR)/my-os.iso

# 默认目标
all: $(OS_ISO)

# --- 核心编译链接规则 ---

# 链接内核
$(KERNEL_BIN): $(filter %/boot.o,$(OBJS)) $(filter-out %/boot.o,$(OBJS)) | $(BUILD_DIR)
	$(LD) $(LDFLAGS) -o $@ $^

# 通用 C 编译规则
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

# 通用汇编编译规则
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# VPATH 告诉 make 在哪里寻找源文件
VPATH = $(SRC_DIRS)

# --- 关键修复：为 isr.o 和 isr_c.o 提供精确规则 ---
$(BUILD_DIR)/isr.o: cpu/isr.s
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
# ... (ISO 创建规则不变) ...
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