// mm/kheap.c (完全修正版)

#include "kheap.h"
#include "paging.h" // 需要它来进行物理地址转换
#include "string.h"
#include <stddef.h>

// 内存块的头部信息
typedef struct header {
    uint32_t magic;      // 魔术数字，用于校验
    _Bool is_free;       // 标记此内存块是否空闲
    uint32_t size;       // 内存块的总大小 (包括头部)
} header_t;

// 内存块的尾部信息
typedef struct footer {
    uint32_t magic;      // 魔术数字，与头部对应
    header_t* header;    // 指向此内存块的头部
} footer_t;

// --- 全局变量 ---
static uint32_t heap_start_addr = 0;
static uint32_t heap_end_addr = 0;
extern uint32_t end; // 内核的结束地址，由 linker.ld 提供

void init_kheap() {
    heap_start_addr = (uint32_t)&end;
    // 对齐到下一页边界
    if (heap_start_addr & 0xFFF) {
        heap_start_addr &= 0xFFFFF000;
        heap_start_addr += 0x1000;
    }
    
    // 暂时将堆的末尾设置为起始后1MB
    heap_end_addr = heap_start_addr + 1024 * 1024; 

    // 创建第一个大的空闲块
    header_t* initial_block = (header_t*)heap_start_addr;
    initial_block->magic = 0xDEADBEEF;
    initial_block->is_free = 1;
    initial_block->size = heap_end_addr - heap_start_addr;
    
    footer_t* initial_footer = (footer_t*) (heap_end_addr - sizeof(footer_t));
    initial_footer->magic = 0xDEADBEEF;
    initial_footer->header = initial_block;

    kprint("Linked-list heap initialized.\n");
}

void* kmalloc(uint32_t size) {
    // 加上头部和尾部的大小，并对齐到4字节
    uint32_t total_size = size + sizeof(header_t) + sizeof(footer_t);
    if (total_size & 0x3) {
        total_size &= ~0x3;
        total_size += 4;
    }

    // 遍历堆寻找合适的空闲块 (First-fit)
    header_t* current = (header_t*)heap_start_addr;
    while ((uint32_t)current < heap_end_addr) {
        if (current->is_free && current->size >= total_size) {
            
            // 如果这个块比需要的大很多，就进行分割
            if (current->size > total_size + sizeof(header_t) + sizeof(footer_t)) {
                // 创建新的空闲块
                header_t* new_block = (header_t*)((uint32_t)current + total_size);
                new_block->magic = 0xDEADBEEF;
                new_block->is_free = 1;
                new_block->size = current->size - total_size;
                
                // 为新块写入尾部
                footer_t* new_footer = (footer_t*)((uint32_t)new_block + new_block->size - sizeof(footer_t));
                new_footer->magic = 0xDEADBEEF;
                new_footer->header = new_block;

                // 更新当前块的大小
                current->size = total_size;
            }

            // 标记当前块为已使用
            current->is_free = 0;
            
            // 为当前块写入尾部
            footer_t* footer = (footer_t*)((uint32_t)current + current->size - sizeof(footer_t));
            footer->magic = 0xDEADBEEF;
            footer->header = current;

            // 返回给用户的地址是头部后面的部分
            return (void*)((uint32_t)current + sizeof(header_t));
        }
        
        // 移动到下一个块
        current = (header_t*)((uint32_t)current + current->size);
    }
    
    kprint("Error: kmalloc failed, out of memory!\n");
    return NULL; // 没有找到
}

void kfree(void* p) {
    if (p == NULL) return;

    header_t* header = (header_t*)((uint32_t)p - sizeof(header_t));
    if (header->magic != 0xDEADBEEF) {
        // 错误处理
        return;
    }

    header->is_free = 1;

    // --- 尝试与后面的块合并 ---
    header_t* next_block = (header_t*)((uint32_t)header + header->size);
    if ((uint32_t)next_block < heap_end_addr && next_block->magic == 0xDEADBEEF && next_block->is_free) {
        header->size += next_block->size;
        // 更新合并后大块的尾部
        footer_t* footer = (footer_t*)((uint32_t)header + header->size - sizeof(footer_t));
        footer->header = header;
    }
    
    // --- 尝试与前面的块合并 ---
    footer_t* prev_footer = (footer_t*)((uint32_t)header - sizeof(footer_t));
    if ((uint32_t)prev_footer > heap_start_addr && prev_footer->magic == 0xDEADBEEF) {
        header_t* prev_block = prev_footer->header;
        if (prev_block->is_free) {
            prev_block->size += header->size;
            // 更新合并后大块的尾部
            footer_t* footer = (footer_t*)((uint32_t)prev_block + prev_block->size - sizeof(footer_t));
            footer->header = prev_block;
        }
    }
}

// 分配一块页对齐的内存
void* kmalloc_a(uint32_t size) {
    // 简单的实现：多分配一些，然后返回对齐的地址
    // 注意：这会浪费一些内存，并且 kfree 无法正确处理，但这对于我们的目的来说足够了
    void* mem = kmalloc(size + 4096);
    uint32_t addr = (uint32_t)mem;
    if (addr & 0xFFF) {
        addr &= 0xFFFFF000;
        addr += 0x1000;
    }
    return (void*)addr;
}