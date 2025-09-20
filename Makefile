# --- 项目配置 ---
BUILD_DIR = build
SRC_DIRS = cpu drivers fs kernel lib mm

# --- 工具链 ---
CC = gcc
AS = nasm
LD = ld
OBJCOPY = objcopy

# --- 编译和链接标志 ---
CFLAGS = -m32 -ffreestanding -c -g -O0 -Wall -Wextra -Wno-unused-parameter -fno-omit-frame-pointer -I. -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# --- 用户程序配置 ---
USER_PROGRAM_ELF = $(BUILD_DIR)/user/program.elf
USER_PROGRAM_BIN = $(BUILD_DIR)/user/program.bin
USER_PROGRAM_OBJ = $(BUILD_DIR)/user_program_bin.o
USER_C_SOURCES = $(wildcard user/*.c)
USER_C_OBJS = $(patsubst %.c,$(BUILD_DIR)/c/%.o,$(USER_C_SOURCES))

# --- 源文件查找 ---
C_SOURCES = $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.c))
S_SOURCES = $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.s))
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/c/%.o,$(C_SOURCES))
S_OBJS = $(patsubst %.s,$(BUILD_DIR)/s/%.o,$(S_SOURCES))

# --- 组合所有目标文件 ---
OBJS = $(C_OBJS) $(S_OBJS) $(USER_PROGRAM_OBJ)

# 最终目标
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_ISO = $(BUILD_DIR)/my-os.iso

# 默认目标
all: $(OS_ISO)

# --- 核心编译链接规则 ---
$(KERNEL_BIN): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# --- 通用编译规则 ---
# For C files
$(BUILD_DIR)/c/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

# For Assembly files
$(BUILD_DIR)/s/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# --- 用户程序编译规则 (四步走) ---
# 1. 编译用户 C -> .o
$(USER_C_OBJS): $(USER_C_SOURCES) user/syscall.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $(USER_C_SOURCES) -o $(USER_C_OBJS)

# 2. 链接 .o -> .elf
$(USER_PROGRAM_ELF): $(USER_C_OBJS) user/link.ld
	@mkdir -p $(dir $@)
	$(LD) -m elf_i386 -T user/link.ld -nostdlib -o $@ $(USER_C_OBJS)

# 3. 提取 .elf -> .bin
$(USER_PROGRAM_BIN): $(USER_PROGRAM_ELF)
	@mkdir -p $(dir $@)
	$(OBJCOPY) -O binary $< $@

# 4. 将 .bin 转换为可链接的 .o 文件
$(USER_PROGRAM_OBJ): $(USER_PROGRAM_BIN)
	@mkdir -p $(dir $@)
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

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