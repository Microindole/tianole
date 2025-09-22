#include "common.h"
#include "shell.h"
#include "../fs/vfs.h"
#include "../mm/kheap.h"
#include "task.h"
#include "../mm/paging.h"
#include "syscall.h"
#include "string.h"
#include "../cpu/gdt.h"


unsigned short* const VIDEO_MEMORY = (unsigned short*)0xB8000;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

int cursor_x = 0;
int cursor_y = 0;

void move_cursor() {
    uint16_t cursorLocation = cursor_y * VGA_WIDTH + cursor_x;
    outb(0x3D4, 14);
    outb(0x3D5, cursorLocation >> 8);
    outb(0x3D4, 15);
    outb(0x3D5, cursorLocation);
}

void scroll() {
    unsigned char attribute_byte = 0x0F;
    unsigned short blank = 0x20 | (attribute_byte << 8);

    if (cursor_y >= VGA_HEIGHT) {
        int i;
        for (i = 0; i < (VGA_HEIGHT - 1) * VGA_WIDTH; i++) {
            VIDEO_MEMORY[i] = VIDEO_MEMORY[i + VGA_WIDTH];
        }
        for (i = (VGA_HEIGHT - 1) * VGA_WIDTH; i < VGA_HEIGHT * VGA_WIDTH; i++) {
            VIDEO_MEMORY[i] = blank;
        }
        cursor_y = VGA_HEIGHT - 1;
    }
}

void kputc(char c) {
    unsigned char attribute_byte = 0x0F;
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else if (c == '\b') {
        if (cursor_x > 0) {
            cursor_x--;
            VIDEO_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = ' ' | (attribute_byte << 8);
        }
    } else if (c >= ' ') {
        int offset = cursor_y * VGA_WIDTH + cursor_x;
        VIDEO_MEMORY[offset] = c | (attribute_byte << 8);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }
    scroll();
    move_cursor();
}

void kprint(const char* str) {
    int i = 0;
    while (str[i]) {
        kputc(str[i++]);
    }
}

void clear_screen() {
    unsigned char attribute_byte = 0x0F;
    unsigned short blank = 0x20 | (attribute_byte << 8);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VIDEO_MEMORY[i] = blank;
    }
    cursor_x = 0;
    cursor_y = 0;
    move_cursor();
}

static void strrev_kernel(char *s, int len) {
    char *e = s + len - 1;
    while (s < e) {
        char tmp = *s;
        *s = *e;
        *e = tmp;
        s++;
        e--;
    }
}

void itoa(int n, char* str, int len, int base) {
    int i = 0;
    _Bool is_negative = 0;
    if (n == 0) {
        if (len > 1) {
            str[i++] = '0';
            str[i] = '\0';
        }
        return;
    }
    if (n < 0 && base == 10) {
        is_negative = 1;
        n = -n;
    }
    while (n != 0 && i < len - 2) {
        int rem = n % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        n /= base;
    }
    if (is_negative && i < len - 2) {
        str[i++] = '-';
    }
    str[i] = '\0';
    strrev_kernel(str, i);
}

void itoa_hex(uint32_t n, char* str, int len) {
    if (len < 3) return;
    str[0] = '0';
    str[1] = 'x';
    if (n == 0) {
        if (len > 3) {
            str[2] = '0';
            str[3] = '\0';
        }
        return;
    }
    itoa(n, str + 2, len - 2, 16);
}


// --- 内核入口函数 ---

void init_idt();
void init_gdt_tss();
void init_timer(uint32_t frequency);


// ld 会自动创建这些符号
extern uint8_t _binary_build_user_bin_start[];
extern uint8_t _binary_build_user_bin_end[];

extern page_directory_t* current_directory;


// --- 【关键修正】添加一个空的 child_entry_point 来满足链接器 ---
void child_entry_point() {
    // 这个函数在当前执行路径下不会被调用，但它的存在可以解决链接错误。
    return;
}


void kernel_main(void) {
    // --- 1. 初始化 (GDT/IDT/Paging等) ---
    clear_screen();
    init_idt();
    init_gdt_tss(); 
    init_kheap();
    init_paging();
    
    kprint("Kernel initialized successfully.\n");

    // --- 2. 加载用户程序 (和之前一样) ---
    uint8_t* user_code_start = _binary_build_user_bin_start;
    uint32_t user_code_size = _binary_build_user_bin_end - _binary_build_user_bin_start;

    kprint("User program found, size: ");
    char size_buf[12];
    itoa(user_code_size, size_buf, 12, 10);
    kprint(size_buf);
    kprint(" bytes.\n");

    uint32_t num_pages = (user_code_size + 4095) / 4096;
    for (uint32_t i = 0; i < num_pages; i++) {
        void* phys_page = kmalloc(4096);
        if (!phys_page) {
            kprint("Error: kmalloc failed for user program page!");
            return;
        }
        uint32_t virt_addr = 0x40000000 + i * 4096;
        uint32_t copy_size = (i == num_pages - 1) ? (user_code_size % 4096) : 4096;
        if (copy_size == 0 && user_code_size > 0) copy_size = 4096;
        memcpy(phys_page, user_code_start + i * 4096, copy_size);
        map_page(current_directory, virt_addr, (uint32_t)phys_page, 1, 1);
    }
    kprint("User program mapped to virtual address 0x40000000.\n");


    // --- 3. 调用用户代码，并传入 kprint 的地址 ---
    void (*user_program_entry)(void (*)(const char*)) = (void (*)(void (*)(const char*)))0x40000000;
    kprint("Calling user code in Ring 0 and passing kprint pointer...\n\n");
    user_program_entry(kprint);


    kprint("\nUser program finished and returned to kernel.\n");

    // --- 4. 用户代码结束后，内核停机 ---
    kprint("Kernel halting.\n");
    asm volatile ("cli; hlt");
}
