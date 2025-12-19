#ifndef USERMODE_H
#define USERMODE_H

#include "common.h"

// 切换到用户态并执行指定的函数
// entry: 用户态函数的入口地址
// user_stack: 用户态栈的栈顶地址
void switch_to_usermode(uint32_t entry, uint32_t user_stack);

#endif
