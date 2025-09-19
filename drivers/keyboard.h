#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "common.h"

// --- 定义特殊按键码 ---
// 这里要选择一个普通字符不会使用的值域
#define KEY_LEFT_ARROW      0xE0
#define KEY_RIGHT_ARROW     0xE1
#define KEY_UP_ARROW        0xE2
#define KEY_DOWN_ARROW      0xE3

void init_keyboard();

#endif