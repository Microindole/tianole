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

// --------------------
// --- 绝对安全的整数转字符串函数 ---
// --------------------
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

// ld 会自动创建这些符号
extern uint8_t _binary_build_user_bin_start[];
extern uint8_t _binary_build_user_bin_end[];

void switch_to_user_mode() {
    // 在跳转前，设置 TSS 的内核栈
    // 我们把当前的 esp 作为内核栈顶传给 TSS
    uint32_t esp;
    asm volatile("mov %%esp, %0" : "=r"(esp));
    tss_set_stack(esp);

    asm volatile (
        "cli;"
        "mov $0x23, %%ax;" // 用户数据段选择子 (0x20 | 3)
        "mov %%ax, %%ds;"
        "mov %%ax, %%es;"
        "mov %%ax, %%fs;"
        "mov %%ax, %%gs;"

        "pushl $0x23;"
        "pushl $0xC0000000;"
        "pushl %%eax;"      // ESP
        "pushf;"            // EFLAGS
        "popl %%eax;"       //
        "or $0x200, %%eax;" // 打开中断
        "pushl %%eax;"      //
        "pushl $0x1B;"      // CS (0x18 | 3)
        "pushl $0x40000000;"// EIP (用户程序入口点)
        "iret;"             // <<<<<< THE BIG SWITCH
        : : : "eax"
    );
}

void kernel_main(void) {
    // --- 1. 所有初始化照常进行 ---
    clear_screen();
    init_idt();
    init_gdt_tss();
    init_kheap();
    init_paging();
    init_syscalls();
    // init_tasking();
    // init_timer(50);
    // init_vfs();

    // --- 加载用户程序 ---
    uint8_t* user_code_start = _binary_build_user_bin_start;
    uint32_t user_code_size = _binary_build_user_bin_end - _binary_build_user_bin_start;

    // 1. 分配物理内存页
    // 为了简单，我们分配 4KB (一页)
    void* prog_phys_mem = kmalloc(4096);
    void* user_stack_mem = kmalloc(4096);

    // 2. 将用户程序代码复制过去
    memcpy(prog_phys_mem, user_code_start, user_code_size);

    // 3. 映射虚拟地址到物理地址！
    extern page_directory_t* current_directory; // 确保能访问当前页目录
    map_page(current_directory, 0x40000000, (uint32_t)prog_phys_mem, 0, 1);
    map_page(current_directory, 0xBFFFF000, (uint32_t)user_stack_mem, 0, 1);

    kprint("Jumping to user mode...\n");
    switch_to_user_mode();



    // init_shell();

    // asm volatile ("sti"); // 开启中断
    // while (1) {
    //     asm volatile ("hlt"); // 等待下一次中断 (键盘、时钟等)
    // }
}

