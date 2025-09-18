#include "keyboard.h"
#include "common.h"
#include "isr.h"

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
    // 从键盘的 I/O 端口 0x60 读取按键的扫描码
    uint8_t scancode = inb(0x60);

    // 我们只处理按键按下的事件 (最高位为 0)
    if (scancode < sizeof(scancode_to_ascii)) {
        char c = scancode_to_ascii[scancode];
        if (c) { // 如果 c 不是 0, 说明是一个可打印字符
            kputc(c);
        }
    }
}

// 初始化键盘处理
void init_keyboard() {
    // 注册键盘中断 (IRQ 1 -> 中断号 33) 的处理函数
    register_interrupt_handler(33, &keyboard_callback);
}