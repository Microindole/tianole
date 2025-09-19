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

void kernel_main(void) {
    clear_screen();
    init_idt();
    
    // 请确保你使用的是简化的、用于调试的 kheap.c
    init_kheap();
    
    init_paging();
    init_syscalls();
    init_tasking();
    init_timer(50);
    init_vfs();
    init_shell();

    kprint("Kernel initialized. Starting fork() test...\n");

    int pid = fork();

    if (pid == 0) {
        // 子进程现在有了自己的入口点 (child_entry_point)，
        // 它永远不会执行到这里的代码。
        // 我们可以把这里清空。
    } else {
        // 父进程代码保持不变
        kprint("--- I am the PARENT process! My fork() returned PID: ");
        char pid_str[12];
        itoa(pid, pid_str, 12, 10);
        kprint(pid_str);
        kprint(" ---\n");
    }

    kprint("Parent process is returning to shell.\n");
    
    // 父进程的 idle 循环保持不变
    asm volatile ("sti");
    while(1) {
        asm volatile ("hlt");
    }
}

