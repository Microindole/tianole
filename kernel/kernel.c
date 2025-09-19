#include "common.h"
#include "shell.h"
#include "../fs/vfs.h"
#include "../mm/kheap.h"
#include "task.h"
#include "../mm/paging.h"
#include "syscall.h"


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

// --------------------
// --- 绝对安全的整数转字符串函数 ---
// --------------------
static void strrev(char *s, int len) {
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
    strrev(str, i);
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
void init_timer(uint32_t frequency);

// --- 为子进程创建独立的入口点 ---
void child_entry_point() {
    // 这里就是子进程真正的“第一行代码”
    asm volatile("sti");
    kprint("--- I am the CHILD process! My fork() returned 0. ---\n");
    while(1) {
        asm volatile("hlt");
    }
}

// --- 用于验证 kheap 的测试函数 ---
void heap_test() {
    kprint("\n--- Starting Heap Test ---\n");
    char hex_buf[12]; // 用于打印地址

    // 1. 连续申请 3 块内存
    kprint("Allocating a, b, c:\n");
    void* a = kmalloc(8);
    void* b = kmalloc(8);
    void* c = kmalloc(8);

    itoa_hex((uint32_t)a, hex_buf, 12);
    kprint("a: "); kprint(hex_buf); kprint("\n");

    itoa_hex((uint32_t)b, hex_buf, 12);
    kprint("b: "); kprint(hex_buf); kprint("\n");

    itoa_hex((uint32_t)c, hex_buf, 12);
    kprint("c: "); kprint(hex_buf); kprint("\n");

    // 2. 释放中间的块 'b'
    kprint("Freeing b...\n");
    kfree(b);

    // 3. 再次申请一块同样大小的内存 'd'
    kprint("Allocating d (should reuse b's space):\n");
    void* d = kmalloc(8);
    itoa_hex((uint32_t)d, hex_buf, 12);
    kprint("d: "); kprint(hex_buf); kprint("\n");

    kprint("--- Heap Test Finished ---\n\n");
}

void kernel_main(void) {
    // --- 1. 所有初始化照常进行 ---
    clear_screen();
    init_idt();
    init_kheap();
    init_paging();
    init_syscalls();
    init_tasking(); // 仍然需要，以创建第一个内核任务
    init_timer(50);   // 仍然需要，以保持系统心跳
    init_vfs();

    // heap_test(); 

    // --- 2. Shell 初始化并打印第一个提示符 ---
    init_shell();

    // --- 3. 内核进入主循环 ---
    // fork() 测试代码已移除。
    // 内核的主任务就是运行 Shell，它通过一个简单的
    // “等待中断”循环来实现。键盘输入会作为中断被处理。
    asm volatile ("sti"); // 开启中断
    while (1) {
        asm volatile ("hlt"); // 等待下一次中断 (键盘、时钟等)
    }
}

