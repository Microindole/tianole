#include "common.h"

// --- 我们之前实现的控制台功能 START ---

unsigned short* const VIDEO_MEMORY = (unsigned short*)0xB8000;
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

int cursor_x = 0;
int cursor_y = 0;

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
}

// --- 我们之前实现的控制台功能 END ---

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

// 内核的入口函数
void kernel_main(void) {
    clear_screen();
    init_idt();
    init_timer(50); // 设置定时器频率为 50 Hz

    kprint("Hello, Interrupt World!\n");
    kprint("Timer should be ticking now.\n");

    // 开启中断
    asm volatile ("sti");

    uint32_t last_tick = 0;
    extern uint32_t tick; // 从 timer.c 引用 tick 变量

    // 无限循环，等待中断的发生
    while(1) {
        if (tick != last_tick) {
            cursor_x = 0;
            cursor_y = 3;

            char tick_str[32];
            itoa(tick, tick_str);
            kprint("Tick:          "); // 用空格覆盖旧数字
            cursor_x = 6; // 回到冒号后面
            kprint(tick_str);

            last_tick = tick;
        }
    }
}