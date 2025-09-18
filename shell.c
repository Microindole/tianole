#include "shell.h"
#include "common.h"
#include "keyboard.h" // shell 需要初始化 keyboard
#include "vfs.h"
#include <stddef.h>  // for NULL

// 简单的字符串比较函数
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 处理输入命令的函数
void process_command(char *command) {
    if (command == NULL || command[0] == '\0') {
        kprint("\n> ");
        return;
    }

    if (strcmp(command, "help") == 0) {
        kprint("\nSimple Shell v1.0");
        kprint("\nCommands: help, clear, ls");
    } else if (strcmp(command, "clear") == 0) {
        clear_screen();
    } else if (strcmp(command, "ls") == 0) {
        ls_current_dir();
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