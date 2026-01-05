# 定义内核路径
KERNEL := target/x86_64-unknown-none/debug/kernel
ISO := tianole.iso

.PHONY: all run clean kernel

all: $(ISO)

# 1. 编译内核 (使用 cargo)
kernel:
	@echo "Building Kernel..."
	@cargo build -p kernel

# 2. 制作 ISO
$(ISO): kernel
	@echo "Generating ISO..."
	@mkdir -p build/iso_root
	# 复制内核
	@cp $(KERNEL) build/iso_root/kernel.elf
	# 复制配置文件
	@cp limine.cfg build/iso_root/
	# 下载并安装 Limine (如果不存在)
	@if [ ! -d "limine" ]; then \
		git clone https://github.com/limine-bootloader/limine.git --branch=v5.x-branch-binary --depth=1; \
		make -C limine; \
	fi
	# 复制 Limine 引导文件到 ISO 目录
	@cp limine/limine-bios.sys limine/limine-bios-cd.bin limine/limine-uefi-cd.bin build/iso_root/
	# 打包 ISO
	@xorriso -as mkisofs -b limine-bios-cd.bin \
		-no-emul-boot -boot-load-size 4 -boot-info-table \
		--efi-boot limine-uefi-cd.bin \
		-efi-boot-part --efi-boot-image --protective-msdos-label \
		build/iso_root -o $(ISO)
	# 将 Limine 部署到 ISO 头部 (使 BIOS 可启动)
	@./limine/limine bios-install $(ISO)

# 3. 运行 QEMU
run: $(ISO)
	@echo "Running QEMU..."
	@qemu-system-x86_64 -cdrom $(ISO) -serial stdio

clean:
	@cargo clean
	@rm -rf build $(ISO) limine