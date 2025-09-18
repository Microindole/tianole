# 编译器和链接器设置
CC = gcc
AS = nasm
LD = ld

# 编译和链接参数
CFLAGS = -m32 -ffreestanding -c -g -O0 -Wall -Wextra -Wno-unused-parameter
ASFLAGS = -f elf32
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib --nmagic

# 源文件对象列表
# 关键修正：我们明确地将 isr.c 编译为 isr_c.o，并同时包含 isr.o 和 isr_c.o
OBJS = boot.o kernel.o idt.o timer.o isr.o isr_c.o

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

# --- 明确的编译规则 ---

# 关键修正：为 isr.c 添加一个特殊的编译规则，生成 isr_c.o
isr_c.o: isr.c
	$(CC) $(CFLAGS) isr.c -o isr_c.o

# 编译其他 C 文件
kernel.o: kernel.c
	$(CC) $(CFLAGS) kernel.c -o kernel.o
idt.o: idt.c
	$(CC) $(CFLAGS) idt.c -o idt.o
timer.o: timer.c
	$(CC) $(CFLAGS) timer.c -o timer.o

# 编译汇编文件
boot.o: boot.s
	$(AS) $(ASFLAGS) boot.s -o boot.o
isr.o: isr.s
	$(AS) $(ASFLAGS) isr.s -o isr.o


# 运行 QEMU
run: my-os.iso
	qemu-system-x86_64 -cdrom my-os.iso

# 清理生成的文件
clean:
	rm -rf *.o *.bin *.iso isodir