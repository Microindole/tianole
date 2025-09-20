// 这是一个用户程序，它不能包含任何内核头文件。
// 它唯一能和内核通信的方式，就是通过我们即将定义的“系统调用”。
#include "syscall.h"

// 用户程序的入口点，而不是 kernel_main
void main() {
    // 调用一个我们“假设”存在的系统调用，来打印字符串
    syscall_puts("Hello from user space!");
    
    // 程序结束后，进入一个死循环
    while(1) {}
}