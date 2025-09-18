// kernel.c - 一个简单的控制台驱动

// 使用一个真正的常量指针，而不是宏，来避免运算符优先级问题
unsigned short* const VIDEO_MEMORY = (unsigned short*)0xB8000;

// 定义VGA显存的起始地址和屏幕尺寸
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// 追踪光标位置的全局变量
int cursor_x = 0;
int cursor_y = 0;

// 屏幕向上滚动一行
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

// 在屏幕上打印一个字符
void kputc(char c) {
    unsigned char attribute_byte = 0x0F;

    if (c == 0x08 && cursor_x) {
        cursor_x--;
        unsigned short* location = VIDEO_MEMORY + (cursor_y * VGA_WIDTH + cursor_x);
        *location = ' ' | (attribute_byte << 8);
    } 
    else if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    }
    else if (c >= ' ') {
        unsigned short* location = VIDEO_MEMORY + (cursor_y * VGA_WIDTH + cursor_x);
        *location = c | (attribute_byte << 8);
        cursor_x++;
    }

    if (cursor_x >= VGA_WIDTH) {
        cursor_x = 0;
        cursor_y++;
    }

    scroll();
}

// 打印一个字符串
void kprint(const char* str) {
    int i = 0;
    while (str[i]) {
        kputc(str[i++]);
    }
}

// 清空屏幕函数
void clear_screen() {
    unsigned char attribute_byte = 0x0F;
    unsigned short blank = 0x20 | (attribute_byte << 8);

    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        VIDEO_MEMORY[i] = blank;
    }
    
    cursor_x = 0;
    cursor_y = 0;
}

// 内核的入口函数
void kernel_main(void) {
    clear_screen();

    kprint("Welcome to my tiny OS!\n");
    kprint("This is line 2.\n");
    kprint("This is a very long line that will wrap around to the next line automatically because it is longer than 80 characters.\n");
    
    // 打印足够多的行来测试滚动功能
    for(int i = 0; i < 25; i++) {
        kprint("This is a test line to demonstrate scrolling.\n");
    }

    kprint("You should see this line at the bottom after scrolling!");
}