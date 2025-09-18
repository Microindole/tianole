#include "keyboard.h"
#include "common.h"
#include "../cpu/isr.h"
#include "../kernel/shell.h"

// 定义命令缓冲区的最大长度
#define CMD_BUFFER_SIZE 256
static char cmd_buffer[CMD_BUFFER_SIZE];
static int cmd_index = 0;

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

    // 检查这是“按下”还是“松开”
    if (scancode & 0x80) { // 最高位为 1 表示是“松开”事件
        scancode -= 0x80; // 转换为“按下”码，以便识别是哪个键
        if (scancode == 0x2A || scancode == 0x36) { // 左 Shift 或 右 Shift
            is_shift_pressed = 0;
        }
    } else { // 这是“按下”事件
        if (scancode == 0x2A || scancode == 0x36) { // 左 Shift 或 右 Shift
            is_shift_pressed = 1;
            return; // 只设置状态，不产生字符
        }

        // 根据 Shift 状态选择正确的映射表
        char c = 0;
        if (is_shift_pressed) {
            c = scancode_to_ascii_shift[scancode];
        } else {
            c = scancode_to_ascii[scancode];
        }

        // --- 下面的命令处理逻辑和之前完全一样 ---
        if (c == '\n') {
            cmd_buffer[cmd_index] = '\0';
            process_command(cmd_buffer);
            cmd_index = 0;
        } else if (c == '\b') {
            if (cmd_index > 0) {
                cmd_index--;
                kputc('\b');
            }
        } else if (c != 0 && cmd_index < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_index++] = c;
            kputc(c);
        }
    }
}

// 初始化键盘处理
void init_keyboard() {
    // 注册键盘中断 (IRQ 1 -> 中断号 33) 的处理函数
    register_interrupt_handler(33, &keyboard_callback);
}