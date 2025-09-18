# 编译器和链接器设置
CC = gcc
AS = nasm
LD = ld

# --- 使用 VPATH 告诉 make 在哪里寻找源文件 ---
VPATH = cpu drivers fs kernel lib

# 编译和链接参数
# --- 只包含公共的 'include' 目录，这是最规范的做法 ---
CFLAGS = -m32 -ffreestanding -c -g -O0 -Wall -Wextra -Wno-unused-parameter -Iinclude
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# 源文件对象列表
# --- 按模块定义对象文件 ---
CPU_OBJS = boot.o idt.o isr.o isr_c.o
DRIVERS_OBJS = timer.o keyboard.o
FS_OBJS = vfs.o
KERNEL_OBJS = kernel.o shell.o
LIB_OBJS = string.o

# 将所有对象文件汇总
OBJS = $(CPU_OBJS) $(DRIVERS_OBJS) $(FS_OBJS) $(KERNEL_OBJS) $(LIB_OBJS)

# 默认目标
all: my-os.iso

# 链接内核
# 关键：boot.o 必须是第一个！
kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.bin boot.o $(filter-out boot.o,$(OBJS))

# --- 编译规则 ---

# 通用 C 编译规则
%.o: %.c
	$(CC) $(CFLAGS) $< -o $@

# 通用汇编编译规则
%.o: %.s
	$(AS) $(ASFLAGS) $< -o $@

# 这会覆盖上面的通用规则，确保正确性
isr.o: cpu/isr.s
	$(AS) $(ASFLAGS) $< -o $@

isr_c.o: cpu/isr.c
	$(CC) $(CFLAGS) $< -o $@



# --- 运行和诊断 ---

# 标准的 ISO 运行方式
run: my-os.iso
	qemu-system-i386 -cdrom my-os.iso

# 新增的诊断目标：让 QEMU 直接加载内核！
qemu-direct: kernel.bin
	qemu-system-i386 -kernel kernel.bin

# 创建 ISO 镜像
my-os.iso: kernel.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o my-os.iso isodir
	@echo "ISO image created: my-os.iso"

# 清理生成的文件
clean:
	rm -rf *.o *.bin *.iso isodir