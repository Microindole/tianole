#include "syscall.h"

void main() {
    syscall_puts("Hello from user space!");
    
    while(1) {}
}