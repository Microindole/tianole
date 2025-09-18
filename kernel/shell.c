#include "shell.h"
#include "common.h"
#include "../drivers/keyboard.h" // shell 需要初始化 keyboard
#include "../fs/vfs.h"
#include <stddef.h>  // for NULL
#include "string.h"  // for strcmp

// --- 缓冲区和光标状态 ---
#define CMD_BUFFER_SIZE 256
static char cmd_buffer[CMD_BUFFER_SIZE];
static int buffer_len = 0;
static int cursor_pos = 0;

// 外部变量，来自 kernel.c
extern int cursor_x, cursor_y;

static void redraw_line() {
    // 1. 将硬件光标移动到行首（提示符后面）
    cursor_x = 2; // "> " 占两个字符
    move_cursor();

    // 2. 打印缓冲区中的所有内容
    for (int i = 0; i < buffer_len; i++) {
        kputc(cmd_buffer[i]);
    }

    // 3. 打印空格，覆盖掉可能残留的旧字符
    kputc(' ');

    // 4. 将硬件光标移动到逻辑光标的正确位置
    cursor_x = 2 + cursor_pos;
    move_cursor();
}


// --- 按键处理函数 ---
void shell_handle_key(uint16_t keycode) {
    switch (keycode) {
        case KEY_LEFT_ARROW:
            if (cursor_pos > 0) {
                cursor_pos--;
                redraw_line(); // 重新绘制以更新光标
            }
            break;

        case KEY_RIGHT_ARROW:
            if (cursor_pos < buffer_len) {
                cursor_pos++;
                redraw_line(); // 重新绘制以更新光标
            }
            break;

        case '\n': // 回车键
            cmd_buffer[buffer_len] = '\0';
            process_command(cmd_buffer); // 处理命令
            // 重置缓冲区和光标位置，为下一条命令做准备
            buffer_len = 0;
            cursor_pos = 0;
            break;

        case '\b': // 退格键
            // 只有当光标不在行首时，退格才有效
            if (cursor_pos > 0) {
                // 从光标位置开始，将后面的所有字符向前移动一位
                for (int i = cursor_pos - 1; i < buffer_len; i++) {
                    cmd_buffer[i] = cmd_buffer[i + 1]; // cmd_buffer[buffer_len] 是 '\0'，正好可以结束循环
                }
                buffer_len--;
                cursor_pos--;
                redraw_line(); // 重新绘制整行以反映删除
            }
            break;

        default: // 可打印字符
            // 确保是可打印字符，并且缓冲区没有满
            if (keycode >= ' ' && buffer_len < CMD_BUFFER_SIZE - 1) {
                // 为新字符腾出空间，将光标位置及之后的所有字符向后移动一位
                for (int i = buffer_len; i > cursor_pos; i--) {
                    cmd_buffer[i] = cmd_buffer[i - 1];
                }
                cmd_buffer[cursor_pos] = (char)keycode; // 在光标处插入新字符
                buffer_len++;
                cursor_pos++;
                redraw_line(); // 重新绘制整行以反映插入
            }
            break;
    }
}

// 处理输入命令的函数
void process_command(char *input) {
    // --- 命令解析逻辑 ---
    // 找到命令的结束位置 (第一个空格或字符串末尾)
    char* command = input;
    char* args = NULL;
    int i = 0;
    while(input[i] != '\0') {
        if (input[i] == ' ') {
            input[i] = '\0'; // 分割命令和参数
            args = &input[i+1];
            break;
        }
        i++;
    }

    // --- 命令处理逻辑 ---
    if (strcmp(command, "help") == 0) {
        kprint("\nSimple Shell v1.0");
        kprint("\nCommands: help, clear, ls, mkdir, touch, cat, write");
    } else if (strcmp(command, "clear") == 0) {
        clear_screen();
    } else if (strcmp(command, "ls") == 0) {
        ls_current_dir();
    } else if (strcmp(command, "mkdir") == 0) {
        if (args == NULL) kprint("\nUsage: mkdir <dir_name>");
        else vfs_mkdir(args);
    } else if (strcmp(command, "touch") == 0) {
        if (args == NULL) kprint("\nUsage: touch <file_name>");
        else vfs_touch(args);
    } else if (strcmp(command, "cat") == 0) {
        if (args == NULL) kprint("\nUsage: cat <file_name>");
        else vfs_cat(args);
    } else if (strcmp(command, "write") == 0) {
        if (args == NULL) {
            kprint("\nUsage: write <file_name> \"content\"");
        } else {
            char* filename = args;
            char* content = NULL;
            int j = 0;
            while(args[j] != '\0') {
                if (args[j] == ' ' && args[j+1] == '"') {
                    args[j] = '\0';
                    content = &args[j+2];
                    break;
                }
                j++;
            }

            if (content) {
                // 找到内容的结束引号并替换为 \0
                int k = 0;
                while(content[k] != '\0') {
                    if (content[k] == '"') {
                        content[k] = '\0';
                        break;
                    }
                    k++;
                }
                vfs_write(filename, content);
            } else {
                kprint("\nUsage: write <file_name> \"content\"");
            }
        }
    } else {
        kprint("\nUnknown command: ");
        kprint(command);
    }
    kprint("\n> ");
}


// 初始化 shell
void init_shell() {
    kprint("Welcome to the Simple Shell!\n");
    kprint("Type 'help' for a list of commands.\n\n");
    kprint("> ");
    init_keyboard(); // shell 启动的一部分是初始化键盘
}