#include "common.h"
#include "shell.h"
#include "../fs/vfs.h"
#include "../mm/kheap.h"
#include "task.h"
#include "../mm/paging.h"
#include "syscall.h"
#include "../drivers/serial.h"
#include "../drivers/ata.h"
#include "../fs/fat16.h"
#include "string.h"


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

    if (c == '\b') { // 退格键
        if (cursor_x > 0) {
            cursor_x--;
            VIDEO_MEMORY[cursor_y * VGA_WIDTH + cursor_x] = ' ' | (attribute_byte << 8);
        }
    } else if (c == '\t') { // Tab键
        cursor_x = (cursor_x + 8) & ~(8 - 1);
    } else if (c == '\r') { // 回车符
        cursor_x = 0;
    } else if (c == '\n') { // 换行符
        cursor_x = 0;
        cursor_y++;
    } else if (c >= ' ') { // 可打印字符
        int offset = cursor_y * VGA_WIDTH + cursor_x;
        VIDEO_MEMORY[offset] = c | (attribute_byte << 8);
        cursor_x++;
    }

    // 将光标检查和滚屏逻辑集中到函数末尾
    // 确保任何操作后都会执行
    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    // 每次调用 kputc 后都检查是否需要滚屏
    scroll();
    // 每次调用 kputc 后都更新硬件光标
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

// 安全的整数转字符串函数
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
void init_timer(uint32_t frequency);

// --- 为子进程创建独立的入口点 ---
void child_entry_point() {
    asm volatile("sti");
    kprint("--- I am the CHILD process! My fork() returned 0. ---\n");
    exit(); 
}

void heap_test() {
    kprint("\n--- Starting Heap Test ---\n");
    char hex_buf[12]; // 用于打印地址

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
    clear_screen();
    init_idt();

    init_serial();
    serial_print("Serial port initialized.\n");

    init_kheap();
    serial_print("Heap initialized.\n");

    init_paging();
    serial_print("Paging initialized.\n");

    init_syscalls();
    serial_print("Syscalls initialized.\n");

    init_tasking();
    serial_print("Tasking initialized.\n");

    init_timer(50);
    init_vfs();

    // 1. 先格式化硬盘 (仅用于开发阶段，每次启动都格式化)
    // fat16_format(); // 你可以选择是否每次都格式化，测试时建议开启

    // 2. 调用 init_fat16() 来读取引导扇区，填充 bpb 变量
    init_fat16();
    serial_print("FAT16 Initialized.\n");

    // 3. 现在 bpb 有了正确的值，再调用 find_free_cluster 就安全了
    uint16_t free_cluster = fat16_find_free_cluster();
    serial_print("First free cluster found at: ");
    char num_buf[10];
    itoa(free_cluster, num_buf, 10, 10);
    serial_print(num_buf);
    serial_print("\n\n");


    serial_print("All systems go. Starting shell.\n");

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

