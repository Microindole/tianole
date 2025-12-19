# Userland程序构建配置
CC = gcc
AS = nasm
LD = ld

# 构建目录
BUILD_DIR = ../build/userland

# 编译标志
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -ffreestanding -c -O0 -I.

# 汇编标志
ASFLAGS = -f elf32

# 链接标志
# -Ttext指定代码段加载地址（必须与内核中USER_PROGRAM_BASE一致）
# -e指定入口点
LDFLAGS = -m elf_i386 -Ttext=0x08048000 -e main

# 源文件
C_SOURCES = userlib.c hello.c
S_SOURCES = syscall.s

# 目标文件（在build目录中）
C_OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
S_OBJS = $(patsubst %.s,$(BUILD_DIR)/%.o,$(S_SOURCES))
OBJS = $(S_OBJS) $(C_OBJS)

# 可执行文件
BIN = $(BUILD_DIR)/hello.bin

all: $(BIN)

# 编译C文件
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $< -o $@

# 编译汇编文件
$(BUILD_DIR)/%.o: %.s
	@mkdir -p $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# 链接生成可执行文件
$(BIN): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(LD) $(LDFLAGS) $(OBJS) -o $(BIN)
	@echo "User program built: $(BIN)"

# 清理
clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
