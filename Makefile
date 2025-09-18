# 编译器和链接器设置
CC = gcc
AS = nasm
LD = ld

# 编译和链接参数
CFLAGS = -m32 -ffreestanding -c -g -O0 -Wall -Wextra -Wno-unused-parameter
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# 源文件对象列表
OBJS = kernel.o idt.o timer.o isr.o isr_c.o keyboard.o

# 默认目标
all: my-os.iso

# 链接内核
kernel.bin: boot.o $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.bin boot.o $(OBJS)

# --- 编译规则 ---
isr_c.o: isr.c
	$(CC) $(CFLAGS) isr.c -o isr_c.o
keyboard.o: keyboard.c
	$(CC) $(CFLAGS) keyboard.c -o keyboard.o
kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o
idt.o: idt.c
	$(CC) $(CFLAGS) idt.c -o idt.o
timer.o: timer.c
	$(CC) $(CFLAGS) timer.c -o timer.o
boot.o: boot.s
	$(AS) $(ASFLAGS) boot.s -o boot.o
isr.o: isr.s
	$(AS) $(ASFLAGS) isr.s -o isr.o

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