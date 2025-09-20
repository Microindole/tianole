#ifndef USER_SYSCALL_H
#define USER_SYSCALL_H

// 我们约定：
// 系统调用号 1 = puts (打印一个字符串)

// 这是一个 C 语言的“包装函数”。
// 它将 C 函数调用转换成一个 `int 0x80` 中断，
// 并将参数通过寄存器传递给内核。
static inline void syscall_puts(const char* s) {
    asm volatile (
        "int $0x80"      // 触发系统调用中断
        :                // 没有输出
        : "a" (2),       // 将系统调用号 1 放入 eax 寄存器
          "b" (s)        // 将字符串的地址放入 ebx 寄存器
        : "memory"       // 告诉编译器内存被修改了
    );
}

#endif