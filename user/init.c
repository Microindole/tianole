// user/init.c (接收 kprint 函数指针的版本)

// 定义一个函数指针类型，方便使用
typedef void (*kprint_t)(const char*);

void _start(kprint_t kprint_func) {
    // 通过接收到的函数指针，调用内核的打印服务
    kprint_func("Hello again! This time, I'm calling a kernel function!\n");
    
    kprint_func("The Ring 0 userland experiment is a success.\n");

    // 无限循环
    while(1);
}