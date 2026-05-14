boot-y := \
	main.o \
	file.o \
	elf_loader.o \
	framebuffer.o \
	memory_map.o \
	support.o

BOOT_OBJS := $(patsubst %.o,$(BUILD_DIR)/arch/boot/%.obj,$(boot-y))
