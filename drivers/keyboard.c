#include "keyboard.h"
#include "common.h"
#include "../cpu/isr.h"
#include "../kernel/shell.h"

// 定义命令缓冲区的最大长度
#define CMD_BUFFER_SIZE 256
static char cmd_buffer[CMD_BUFFER_SIZE];
static int cmd_index = 0;

// 美式键盘的扫描码到 ASCII 字符的映射表 (只处理部分按键以便演示)
// 0 表示该键位未被映射
const char scancode_to_ascii[] = {
    0,   27,  '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0,   'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0,   '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',   0,
    '*',
    0,   ' ', // Alt, Space
    0,   // Caps Lock
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // F1-F10
    0,   // Num Lock
    0,   // Scroll Lock
};


// 键盘中断的回调函数
static void keyboard_callback(registers_t* regs) {
    uint8_t scancode = inb(0x60);

    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];

        // 如果是回车键
        if (c == '\n') {
            cmd_buffer[cmd_index] = '\0'; // 字符串结束符
            process_command(cmd_buffer);  // 处理命令
            cmd_index = 0;                // 重置缓冲区索引
        }
        // 如果是退格键
        else if (c == '\b') {
            if (cmd_index > 0) {
                cmd_index--;
                kputc('\b'); // 在屏幕上模拟退格
            }
        }
        // 如果是可打印字符，并且缓冲区没满
        else if (c >= ' ' && cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = c;
            kputc(c); // 将字符回显到屏幕
        }
    }
}

// 初始化键盘处理
void init_keyboard() {
    // 注册键盘中断 (IRQ 1 -> 中断号 33) 的处理函数
    register_interrupt_handler(33, &keyboard_callback);
}