#include "shell.h"
#include "common.h"
#include "../drivers/keyboard.h" // shell 需要初始化 keyboard
#include "../fs/vfs.h"
#include <stddef.h>  // for NULL
#include "string.h"  // for strcmp

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