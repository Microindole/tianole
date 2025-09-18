#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "common.h"

// --- 新增：定义特殊按键码 ---
// 我们选择一个普通字符不会使用的值域
#define KEY_LEFT_ARROW      0xE0
#define KEY_RIGHT_ARROW     0xE1
// 可以在这里添加 KEY_UP, KEY_DOWN 等

void init_keyboard();

#endif