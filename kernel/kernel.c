#include "common.h"
#include "shell.h"
#include "../fs/vfs.h"
#include "../mm/kheap.h"
#include "task.h"
#include "../mm/paging.h"


unsigned short* const VIDEO_MEMORY = (unsigned short*)0xB8000;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

int cursor_x = 0;
int cursor_y = 0;

void move_cursor() {
    // 计算光标的一维线性位置
    uint16_t cursorLocation = cursor_y * VGA_WIDTH + cursor_x;

    // 向 VGA 控制器发送命令，告诉它我们要设置光标的高位字节
    outb(0x3D4, 14);
    // 发送高8位
    outb(0x3D5, cursorLocation >> 8);
    // 向 VGA 控制器发送命令，告诉它我们要设置光标的低位字节
    outb(0x3D4, 15);
    // 发送低8位
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
            // 用带属性的空格符覆盖前一个字符
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

// 声明外部的初始化函数
void init_idt();
void init_timer(uint32_t frequency);

// 简单的整数转字符串函数
void itoa(int n, char str[]) {
    int i = 0;
    if (n == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }
    while(n != 0) {
        int rem = n % 10;
        str[i++] = rem + '0';
        n = n / 10;
    }
    str[i] = '\0';
    int start = 0, end = i - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        start++;
        end--;
    }
}

// 简单的十六进制整数转字符串函数
void itoa_hex(uint32_t n, char str[]) {
    int i = 0;
    int temp;

    // 先加上 '0x' 前缀
    str[i++] = '0';
    str[i++] = 'x';

    if (n == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    char hex_chars[] = "0123456789ABCDEF";
    int start_index = i;

    while (n > 0) {
        temp = n % 16;
        str[i++] = hex_chars[temp];
        n /= 16;
    }
    str[i] = '\0';

    // 反转十六进制部分
    int start = start_index;
    int end = i - 1;
    while (start < end) {
        char t = str[start];
        str[start] = str[end];
        str[end] = t;
        start++;
        end--;
    }
}

void init_idt();
void init_timer(uint32_t frequency);

// 内核的入口函数
void kernel_main(void) {
    clear_screen();
    init_idt();
    
    // 1. 初始化内存管理 (Heap)
    init_kheap();
    
    // 2. 初始化分页！！！
    init_paging();

    // 3. 初始化任务系统
    init_tasking();
    
    init_timer(50); // 设置定时器频率为 50 Hz
    init_vfs();
    init_shell();

    kprint("Hello from a Paged World!\n"); // <--- 可以改个欢迎语
    kprint("Timer should be ticking now.\n");

    // 开启中断
    asm volatile ("sti");

    // 触发一个页错误来测试！
    // uint32_t* ptr = (uint32_t*)0xA0000000;
    // uint32_t test = *ptr;

    while(1) {
        asm volatile ("hlt");
    }
}