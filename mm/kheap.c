// mm/kheap.c
#include "kheap.h"
#include <stddef.h>

// 由 linker.ld 提供，标记了内核的结束位置
extern uint32_t end;
    
// 指向当前空闲内存的起始位置
uint32_t free_mem_addr = 0;

void init_kheap() {
    free_mem_addr = (uint32_t)&end;
}

// 最终的、带 4 字节对齐的线性分配器
void* kmalloc(uint32_t size) {
    // 确保堆已经被初始化
    // if (free_mem_addr == 0) {
    //     init_kheap();
    // }

    // --- 关键修复：内存对齐 ---
    // 检查当前地址是否是 4 的倍数
    if (free_mem_addr & 0x00000003) { // 检查低两位是否不为0
        free_mem_addr &= 0xFFFFFFFC;   // 将地址向下舍入到最近的 4 的倍数
        free_mem_addr += 4;          // 将地址向上移动到下一个 4 的倍数
    }
    
    // 保存对齐后的地址作为返回值
    uint32_t alloc_addr = free_mem_addr;
        
    // 将指针向后移动，为下一次分配做准备
    free_mem_addr += size;
        
    // 返回分配的内存地址
    return (void*)alloc_addr;
}
