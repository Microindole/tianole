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
        // --- 关键修正 ---
        // 使用我们实际映射的栈地址。栈是从高地址向低地址增长的，
        // 所以我们将指针设置在分配的 4KB 区域的顶部。
        "pushl $0xBFFFF000 + 4096;" 
        "pushf;"             // EFLAGS
        "popl %%eax;"        //
        "or $0x200, %%eax;"  // 打开中断
        "pushl %%eax;"       //
        "pushl $0x1B;"       // CS (0x18 | 3)
        "pushl $0x40000000;" // EIP (用户程序入口点)
        "iret;"              // <<<<<< THE BIG SWITCH
        : : : "eax"
    );
}


extern page_directory_t* current_directory;

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

    // 1. 计算需要多少个 4KB 的页来存放用户程序
    //    (size + 4095) / 4096 是一种计算向上取整的常用技巧
    uint32_t num_pages = (user_code_size + 4095) / 4096;

    // 2. 为用户程序分配并映射所需的全部页面
    for (uint32_t i = 0; i < num_pages; i++) {
        // a. 为当前页分配一个物理内存页
        void* phys_page = kmalloc(4096);
        if (!phys_page) {
            kprint("Error: kmalloc failed for user program page!");
            return;
        }

        // b. 计算当前页对应的虚拟地址
        uint32_t virt_addr = 0x40000000 + i * 4096;

        // c. 将这一页从用户程序的二进制数据中复制过去
        //    注意：最后一个页可能不需要复制完整的 4096 字节
        uint32_t copy_size = (i == num_pages - 1) ? (user_code_size % 4096) : 4096;
        if (copy_size == 0 && user_code_size > 0) copy_size = 4096; // 处理刚好是页大小整数倍的情况
        memcpy(phys_page, user_code_start + i * 4096, copy_size);

        // d. 映射虚拟地址到我们刚分配并填充的物理地址
        map_page(current_directory, virt_addr, (uint32_t)phys_page, 0, 1);
    }

    // 3. 为用户程序分配并映射栈空间 (保持不变)
    void* user_stack_mem = kmalloc(4096);
    map_page(current_directory, 0xBFFFF000, (uint32_t)user_stack_mem, 0, 1);

    kprint("Jumping to user mode...\n");
    switch_to_user_mode();



    init_shell();

    asm volatile ("sti"); // 开启中断
    while (1) {
        asm volatile ("hlt"); // 等待下一次中断 (键盘、时钟等)
    }
}

