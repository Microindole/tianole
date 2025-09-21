// user/init.c

// 系统调用封装函数 (目前我们先手写汇编)
void syscall_putc(char c) {
    asm volatile ("int $0x80" : : "a"(1), "b"(c)); // 1 号系统调用：putc
}

void _start() {
    const char* msg = "Hello from User Space!\n";
    for (int i = 0; msg[i] != '\0'; i++) {
        syscall_putc(msg[i]);
    }

    // 用户程序结束后，进入无限循环
    while(1);
}