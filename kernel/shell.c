#include "shell.h"
#include "common.h"
#include "../drivers/keyboard.h" // shell 需要初始化 keyboard
#include "../fs/vfs.h"
#include <stddef.h>  // for NULL
#include "string.h"  // for strcmp
#include "syscall.h"
#include "task.h"
#include "../fs/fat16.h"

// --- 缓冲区和光标状态 ---
#define CMD_BUFFER_SIZE 256
#define HISTORY_MAX_COUNT 16
static char cmd_buffer[CMD_BUFFER_SIZE];
static int buffer_len = 0;
static int cursor_pos = 0;
static int prompt_len = 0;

// --- 历史命令缓冲区 ---
static char command_history[HISTORY_MAX_COUNT][CMD_BUFFER_SIZE];
static int history_count = 0;      // 已保存的历史命令总数
static int history_head = 0;       // 指向下一条要插入的位置 (环形缓冲区的头部)
static int history_view_idx = -1;  // 当前正在查看的历史命令索引 (-1 表示没在看)

// --- 声明外部函数 ---
void get_current_path(char* buffer);
void fat16_ls();

// 外部变量，来自 kernel.c
extern int cursor_x, cursor_y;
extern unsigned short* const VIDEO_MEMORY;

static void redraw_line() {
    // 1. 定义屏幕宽度和显示属性
    const int VGA_WIDTH = 80;
    unsigned char attribute_byte = 0x0F; // 白底黑字
    unsigned short blank = 0x20 | (attribute_byte << 8); // 带属性的空格

    // 2. 计算输入区域在显存中的起始位置
    int start_offset = cursor_y * VGA_WIDTH + prompt_len;

    // 3. 用“带属性的空格”直接清空输入区域
    //    从提示符末尾一直清到行尾
    for (int i = 0; i < (VGA_WIDTH - prompt_len); i++) {
        VIDEO_MEMORY[start_offset + i] = blank;
    }

    // 4. 将新的命令内容直接写入显存
    for (int i = 0; i < buffer_len; i++) {
        VIDEO_MEMORY[start_offset + i] = cmd_buffer[i] | (attribute_byte << 8);
    }

    // 5. 更新硬件光标到正确的位置
    cursor_x = prompt_len + cursor_pos;
    move_cursor();
}


// --- 按键处理函数 ---
void shell_handle_key(uint16_t keycode) {
    switch (keycode) {
        case KEY_UP_ARROW:
            if (history_count > 0 && (history_view_idx == -1 || history_view_idx > 0)) {
                if (history_view_idx == -1) {
                    history_view_idx = history_head;
                }
                history_view_idx = (history_view_idx - 1 + history_count) % history_count;
                
                // 找到实际存储的位置
                int actual_idx = (history_head - 1 - ((history_head - 1 - history_view_idx + history_count)%history_count) + HISTORY_MAX_COUNT) % HISTORY_MAX_COUNT;

                strcpy(cmd_buffer, command_history[actual_idx]);
                buffer_len = strlen(cmd_buffer);
                cursor_pos = buffer_len;
                redraw_line();
            }
            break;

        case KEY_DOWN_ARROW:
            if (history_view_idx != -1) {
                history_view_idx = (history_view_idx + 1) % history_count;
                
                // 如果回到了起点，则清空输入框
                if (history_view_idx == history_head) {
                     history_view_idx = -1;
                     cmd_buffer[0] = '\0';
                } else {
                    int actual_idx = (history_head - 1 - ((history_head - 1 - history_view_idx + history_count)%history_count) + HISTORY_MAX_COUNT) % HISTORY_MAX_COUNT;
                    strcpy(cmd_buffer, command_history[actual_idx]);
                }

                buffer_len = strlen(cmd_buffer);
                cursor_pos = buffer_len;
                redraw_line();
            }
            break;
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

// 一个专门打印提示符的函数
static void print_prompt() {
    char path[256];
    get_current_path(path);
    kprint(path); 
    kprint("> ");
    // 计算并保存提示符的长度
    prompt_len = strlen(path) + 2; // path + "> "
}

// 处理输入命令的函数
void process_command(char *input) {
    // 只有非空命令才加入历史记录
    if (strlen(input) > 0) {
        strcpy(command_history[history_head], input);
        history_head = (history_head + 1) % HISTORY_MAX_COUNT;
        if (history_count < HISTORY_MAX_COUNT) {
            history_count++;
        }
    }
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
        kprint("\nCommands: help, clear, ls, mkdir, touch, cat, write, append, forktest, ps");
    } else if (strcmp(command, "clear") == 0) {
        clear_screen();
    } else if (strcmp(command, "ls") == 0) {
        // ls_current_dir();
        fat16_ls();
    } else if (strcmp(command, "cd") == 0) {
        if (args == NULL) kprint("\nUsage: cd <dir_name>");
        else vfs_cd(args);
    } else if (strcmp(command, "mkdir") == 0) {
        if (args == NULL) kprint("\nUsage: mkdir <dir_name>");
        else fat16_mkdir(args);
    } else if (strcmp(command, "touch") == 0) {
        if (args == NULL) kprint("\nUsage: touch <file_name>");
        else fat16_touch(args);
    } else if (strcmp(command, "cat") == 0) {
        if (args == NULL) kprint("\nUsage: cat <file_name>");
        else fat16_cat(args);
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
                int k = 0;
                while(content[k] != '\0') {
                    if (content[k] == '"') {
                        content[k] = '\0';
                        break;
                    }
                    k++;
                }
                fat16_write_content(filename, content);
            } else {
                kprint("\nUsage: write <file_name> \"content\"");
            }
        }
    } else if (strcmp(command, "append") == 0) {
        // 'append' 命令的逻辑和 'write' 几乎一样
        if (args == NULL) {
            kprint("\nUsage: append <file_name> \"content_to_append\"");
        } else {
            char* filename = args;
            char* content = NULL;
            int j = 0;
            // 循环遍历参数字符串
            while(args[j] != '\0') {
                // 寻找 "空格 + 双引号" 这个组合，它是文件名和内容的分割点
                if (args[j] == ' ' && args[j+1] == '"') {
                    args[j] = '\0'; // 将空格替换为字符串结束符，从而分离出文件名
                    content = &args[j+2]; // 内容从双引号之后开始
                    break;
                }
                j++;
            }
            if (content) {
                // 找到内容的结束双引号，并替换为字符串结束符
                int k = 0;
                while(content[k] != '\0') {
                    if (content[k] == '"') {
                        content[k] = '\0';
                        break;
                    }
                    k++;
                }
                // 调用 VFS 的 append 函数
                vfs_append(filename, content);
            } else {
                kprint("\nUsage: append <file_name> \"content_to_append\"");
            }
        }
    } else if (strcmp(command, "forktest") == 0) {
        kprint("\nParent process attempting to fork...\n");
        int pid = fork(); // 调用你写的 fork() 系统调用！
        if (pid == 0) {
            // fork() 在子进程中返回 0。
            // 但我们的子进程会直接跳转到 child_entry_point，
            // 所以理论上这段代码不会在子进程中执行。
            // 把它留在这里作为逻辑完整性的展示。
        } else {
            // fork() 在父进程中返回子进程的 PID。
            kprint("Parent process: Fork successful! Child PID is ");
            char buf[8];
            itoa(pid, buf, 8, 10);
            kprint(buf);
            kprint("\n");
        }
    } else if (strcmp(command, "ps") == 0) {
        list_processes();
    } else {
        kprint("\nUnknown command: ");
        kprint(command);
    }
    kprint("\n");       // 在命令输出后换行
    print_prompt();   // 打印新的提示符
}


// 初始化 shell
void init_shell() {
    kprint("Welcome to the Simple Shell!\n");
    kprint("Type 'help' for a list of commands.\n\n");
    kprint("\n");       // 在欢迎语后换行
    print_prompt();   // 打印第一个提示符
    init_keyboard(); // shell 启动的一部分是初始化键盘
}