# 编译器和链接器设置
CC = gcc
AS = nasm
LD = ld

# 编译和链接参数
CFLAGS = -m32 -ffreestanding -c
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib --nmagic

# 源文件对象
OBJS = boot.o kernel.o

# 默认目标
all: my-os.iso

# 创建 ISO 镜像
my-os.iso: kernel.bin grub.cfg
	mkdir -p isodir/boot/grub
	cp kernel.bin isodir/boot/
	cp grub.cfg isodir/boot/grub/
	grub-mkrescue -o my-os.iso isodir
	@echo "ISO image created: my-os.iso"

# 链接内核
kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o kernel.bin $(OBJS)

# 编译 C 代码
kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o

# 编译汇编代码
boot.o: boot.s
	$(AS) $(ASFLAGS) boot.s -o boot.o

# 运行 QEMU
run: my-os.iso
	qemu-system-x86_64 -cdrom my-os.iso

# 清理生成的文件
clean:
	rm -rf *.o *.bin *.iso isodir