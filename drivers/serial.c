#include "serial.h"

// COM1 串口的 I/O 端口地址
#define PORT 0x3f8

void init_serial() {
    outb(PORT + 1, 0x00); // 禁用所有中断
    outb(PORT + 3, 0x80); // 使能 DLAB (波特率设置)
    outb(PORT + 0, 0x03); // 设置波特率除数为 3 (38400 bps)
    outb(PORT + 1, 0x00); //
    outb(PORT + 3, 0x03); // 8 位, 无校验, 1 个停止位
    outb(PORT + 2, 0xC7); // 使能 FIFO, 清空它们
    outb(PORT + 4, 0x0B); // IRQs enabled, RTS/DSR set
}

// 检查发送队列是否为空
static int is_transmit_empty() {
    return inb(PORT + 5) & 0x20;
}

// 发送一个字符
void serial_putc(char a) {
    while (is_transmit_empty() == 0); // 等待直到发送队列为空
    outb(PORT, a);
}

// 发送一个字符串
void serial_print(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        serial_putc(str[i]);
    }
}