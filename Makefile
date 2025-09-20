# # --- 项目配置 ---
# BUILD_DIR = build
# SRC_DIRS = cpu drivers fs kernel lib mm

# # --- 工具链 ---
# CC = gcc
# AS = nasm
# LD = ld
# OBJCOPY = objcopy

# # --- 编译和链接标志 ---
# CFLAGS = -m32 -ffreestanding -c -g -O0 -Wall -Wextra -Wno-unused-parameter -fno-omit-frame-pointer -I. -Iinclude
# ASFLAGS = -f elf32
# LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# # --- 用户程序配置 ---
# USER_PROGRAM_ELF = $(BUILD_DIR)/program.elf
# USER_PROGRAM_BIN = $(BUILD_DIR)/program.bin
# USER_C_SOURCE = user/program.c
# USER_C_OBJ = $(BUILD_DIR)/program.o
# USER_LDFLAGS = -m elf_i386 -T user/link.ld -nostdlib

# # --- 源文件查找 ---
# # 自动查找所有 .c 和 .s 文件，但排除需要特殊处理的文件
# C_SOURCES = $(filter-out cpu/isr.c, $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.c)))
# S_SOURCES = $(filter-out cpu/boot.s cpu/isr.s cpu/paging.s kernel/switch.s cpu/fork_trampoline.s, $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.s)))

# # 将通用源文件映射到目标文件
# C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
# S_OBJS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(S_SOURCES))

# # --- 明确列出所有目标文件，避免任何冲突 ---
# OBJS = $(C_OBJS) $(S_OBJS) \
#        $(BUILD_DIR)/boot.o \
#        $(BUILD_DIR)/isr_s.o \
#        $(BUILD_DIR)/isr_c.o \
#        $(BUILD_DIR)/paging_s.o \
#        $(BUILD_DIR)/switch.o \
#        $(BUILD_DIR)/fork_trampoline.o \
#        $(BUILD_DIR)/gdt.o \
#        $(BUILD_DIR)/gdt_s.o

# # 最终目标
# KERNEL_BIN = $(BUILD_DIR)/kernel.bin
# OS_ISO = $(BUILD_DIR)/my-os.iso

# # 默认目标
# all: $(OS_ISO)

# # --- 核心编译链接规则 ---
# $(KERNEL_BIN): $(OBJS) linker.ld
# 	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# # --- 通用编译规则 ---
# # VPATH 告诉 make 在哪里寻找源文件
# VPATH = $(SRC_DIRS)
# $(BUILD_DIR)/%.o: %.c
# 	@mkdir -p $(dir $@)
# 	$(CC) $(CFLAGS) $< -o $@
# $(BUILD_DIR)/%.o: %.s
# 	@mkdir -p $(dir $@)
# 	$(AS) $(ASFLAGS) $< -o $@

# # --- 为特殊文件提供精确的编译规则 ---
# $(BUILD_DIR)/boot.o: cpu/boot.s
# 	@mkdir -p $(BUILD_DIR)
# 	$(AS) $(ASFLAGS) $< -o $@
# $(BUILD_DIR)/isr_s.o: cpu/isr.s
# 	@mkdir -p $(BUILD_DIR)
# 	$(AS) $(ASFLAGS) $< -o $@
# $(BUILD_DIR)/isr_c.o: cpu/isr.c
# 	@mkdir -p $(BUILD_DIR)
# 	$(CC) $(CFLAGS) $< -o $@
# $(BUILD_DIR)/paging_s.o: cpu/paging.s
# 	@mkdir -p $(BUILD_DIR)
# 	$(AS) $(ASFLAGS) $< -o $@
# $(BUILD_DIR)/switch.o: kernel/switch.s
# 	@mkdir -p $(BUILD_DIR)
# 	$(AS) $(ASFLAGS) $< -o $@
# $(BUILD_DIR)/fork_trampoline.o: cpu/fork_trampoline.s
# 	@mkdir -p $(BUILD_DIR)
# 	$(AS) $(ASFLAGS) $< -o $@
# $(BUILD_DIR)/task_utils.o: cpu/task_utils.s
# 	@mkdir -p $(BUILD_DIR)
# 	$(AS) $(ASFLAGS) $< -o $@
# $(BUILD_DIR)/gdt.o: cpu/gdt.c
# 	@mkdir -p $(BUILD_DIR)
# 	$(CC) $(CFLAGS) $< -o $@
# $(BUILD_DIR)/gdt_s.o: cpu/gdt.s
# 	@mkdir -p $(BUILD_DIR)
# 	$(AS) $(ASFLAGS) $< -o $@

# # --- 用户程序编译规则 ---
# $(USER_PROGRAM_BIN): $(USER_PROGRAM_ELF)
# 	$(OBJCOPY) -O binary $< $@

# $(USER_PROGRAM_ELF): $(USER_C_OBJ) user/link.ld
# 	$(LD) $(USER_LDFLAGS) -o $@ $(USER_C_OBJ)

# $(USER_C_OBJ): $(USER_C_SOURCE)
# 	@mkdir -p $(dir $@)
# 	$(CC) $(CFLAGS) $< -o $@


# # --- 运行和诊断 ---
# run: $(OS_ISO)
# 	qemu-system-i386 -cdrom $(OS_ISO)
# qemu-direct: $(KERNEL_BIN)
# 	qemu-system-i386 -kernel $(KERNEL_BIN)

# # --- ISO 创建和清理 ---
# $(OS_ISO): $(KERNEL_BIN) grub.cfg
# 	@mkdir -p $(BUILD_DIR)/isodir/boot/grub
# 	cp $(KERNEL_BIN) $(BUILD_DIR)/isodir/boot/
# 	cp grub.cfg $(BUILD_DIR)/isodir/boot/grub/
# 	grub-mkrescue -o $@ $(BUILD_DIR)/isodir
# 	@echo "ISO image created: $@"
# clean:
# 	rm -rf $(BUILD_DIR)

# .PHONY: all run qemu-direct clean



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
USER_PROGRAM_OBJ = $(BUILD_DIR)/user_program_bin.o # <-- 新增：转换后的目标文件
USER_C_SOURCES = $(wildcard user/*.c)
USER_C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(USER_C_SOURCES))

# --- 源文件查找 ---
C_SOURCES = $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.c))
S_SOURCES = $(foreach D,$(SRC_DIRS),$(wildcard $(D)/*.s))
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
S_OBJS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(S_SOURCES))

# --- 组合所有目标文件 ---
# VVV 修正 VVV：将用户程序的目标文件也加入链接列表
OBJS = $(C_OBJS) $(S_OBJS) $(USER_PROGRAM_OBJ)

# 最终目标
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
OS_ISO = $(BUILD_DIR)/my-os.iso

# 默认目标
all: $(OS_ISO)

# --- 核心编译链接规则 ---
# VVV 修正 VVV：内核链接依赖于 user_program_obj
$(KERNEL_BIN): $(OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# --- 用户程序编译规则 (三步走) ---
# 1. 编译 C -> .o
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

# 2. 链接 .o -> .elf
$(USER_PROGRAM_ELF): $(USER_C_OBJS) user/link.ld
	$(LD) -m elf_i386 -T user/link.ld -nostdlib -o $@ $(USER_C_OBJS)

# 3. 提取 .elf -> .bin
$(USER_PROGRAM_BIN): $(USER_PROGRAM_ELF)
	$(OBJCOPY) -O binary $< $@

# VVV 新增 VVV：将 .bin 转换为可链接的 .o 文件
$(USER_PROGRAM_OBJ): $(USER_PROGRAM_BIN)
	$(OBJCOPY) -I binary -O elf32-i386 -B i386 $< $@

# --- 通用汇编编译规则 ---
VPATH = $(SRC_DIRS) user
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

# --- 运行和诊断 ---
run: $(OS_ISO)
	qemu-system-i386 -cdrom $(OS_ISO)

# VVV 修正 VVV：恢复 qemu-direct
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