#include "keyboard.h"
#include "common.h"
#include "../cpu/isr.h"
#include "../kernel/shell.h"

// 声明 Shell 的按键处理函数
void shell_handle_key(uint16_t keycode);

static uint8_t is_shift_pressed = 0;

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

const char scancode_to_ascii_shift[] = {
    0,   27,  '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0,   'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0,   '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?',   0,
    '*', 0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};


// 键盘中断的回调函数
static void keyboard_callback(registers_t* regs) {
    uint8_t scancode = inb(0x60);

    // --- 识别方向键 ---
    // 方向键会发送两个字节的扫描码，第一个是 0xE0
    if (scancode == 0xE0) {
        // 我们暂时忽略这个字节，等待下一个字节
        return;
    }

    if (scancode & 0x80) { // 松开事件
        scancode -= 0x80;
        if (scancode == 0x2A || scancode == 0x36) { // Shift
            is_shift_pressed = 0;
        }
    } else { // 按下事件
        if (scancode == 0x2A || scancode == 0x36) { // Shift
            is_shift_pressed = 1;
            return;
        }

        // --- 处理方向键 ---
        if (scancode == 0x4B) { // 左方向键
            shell_handle_key(KEY_LEFT_ARROW);
            return;
        }
        if (scancode == 0x4D) { // 右方向键
            shell_handle_key(KEY_RIGHT_ARROW);
            return;
        }

        char c = 0;
        if (is_shift_pressed) {
            c = scancode_to_ascii_shift[scancode];
        } else {
            c = scancode_to_ascii[scancode];
        }

        if (c) {
            // 将按键码报告给 Shell
            shell_handle_key(c);
        }
    }
}
// 初始化键盘处理
void init_keyboard() {
    // 注册键盘中断 (IRQ 1 -> 中断号 33) 的处理函数
    register_interrupt_handler(33, &keyboard_callback);
}