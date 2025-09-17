// 定义VGA显存的起始地址和屏幕尺寸
#define VGA_ADDRESS 0xb8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25

// 内核的入口函数
void kernel_main(void) {
    // 获取VGA显存指针
    volatile unsigned short *vga_buffer = (unsigned short*)VGA_ADDRESS;
    
    // 要显示的消息
    const char *message = "Hello, C Kernel!";
    
    // 遍历消息并显示在屏幕上 (白底黑字)
    for (int i = 0; message[i] != '\0'; ++i) {
        // 字符的颜色属性 0x0F 表示白底黑字
        vga_buffer[i] = (unsigned short)message[i] | (unsigned short)0x0F00;
    }
}