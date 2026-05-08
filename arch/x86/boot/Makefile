BOOT_SRCS := \
	$(ARCH_DIR)/boot/main.c \
	$(ARCH_DIR)/boot/file.c \
	$(ARCH_DIR)/boot/elf_loader.c \
	$(ARCH_DIR)/boot/memory_map.c \
	$(ARCH_DIR)/boot/support.c

BOOT_OBJS := $(patsubst $(ARCH_DIR)/boot/%.c,$(BUILD_DIR)/arch/boot/%.obj,$(BOOT_SRCS))
