#include "kheap.h"
#include <stddef.h>

// --- 数据结构定义 ---

// 内存块的头部结构
typedef struct header {
    struct header* next; // 指向下一个空闲块
    uint32_t size;       // 整个块的大小（包括头部）
} header_t;

// --- 全局变量 ---

// 由 linker.ld 提供，标记了内核的结束位置
extern uint32_t end;

// 空闲链表的头指针
static header_t* free_list_head = NULL;

// 堆的起始地址
static uint32_t heap_start_addr = 0;


// --- 函数实现 ---

void init_kheap() {
    // 将堆的起始地址设置为内核结束的位置，并进行页对齐（4KB）
    heap_start_addr = (uint32_t)&end;
    if (heap_start_addr & 0xFFFFF000) {
        heap_start_addr &= 0xFFFFF000;
        heap_start_addr += 0x1000;
    }
    
    // 初始时，整个堆就是一整个大的空闲块
    free_list_head = (header_t*)heap_start_addr;
    // 假设我们初始有 1MB 的堆空间
    free_list_head->size = 1024 * 1024; 
    free_list_head->next = NULL;
}


void kfree(void* p) {
    if (p == NULL) {
        return;
    }

    // 通过用户指针找到块的头部
    header_t* block_to_free = (header_t*)((char*)p - sizeof(header_t));

    // 遍历空闲链表，找到合适的位置插入，并尝试合并
    header_t* current = free_list_head;
    header_t* prev = NULL;

    while (current != NULL && current < block_to_free) {
        prev = current;
        current = current->next;
    }

    // 插入并尝试与前一个块合并
    if (prev != NULL) {
        if ((char*)prev + prev->size == (char*)block_to_free) {
            // 与前一个块合并
            prev->size += block_to_free->size;
            block_to_free = prev; // 合并后的新块是 prev
        } else {
            // 无法与前一个块合并，正常插入
            prev->next = block_to_free;
        }
    } else {
        // 作为新的头节点
        free_list_head = block_to_free;
    }

    // 尝试与后一个块合并
    if (current != NULL) {
        if ((char*)block_to_free + block_to_free->size == (char*)current) {
            block_to_free->size += current->size;
            block_to_free->next = current->next; // 跳过 current
        } else {
            block_to_free->next = current;
        }
    } else {
        block_to_free->next = NULL;
    }
}


void* kmalloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }

    // 加上头部的大小，并进行对齐
    uint32_t total_size = size + sizeof(header_t);
    if (total_size % 4 != 0) { // 简单 4 字节对齐
        total_size = (total_size / 4 + 1) * 4;
    }
    
    // 遍历空闲链表，寻找第一个足够大的块 (First-Fit 算法)
    header_t* current = free_list_head;
    header_t* prev = NULL;

    while (current != NULL) {
        if (current->size >= total_size) {
            // 找到了合适的块
            if (current->size > total_size + sizeof(header_t)) {
                // 块太大，进行分裂
                header_t* new_free_block = (header_t*)((char*)current + total_size);
                new_free_block->size = current->size - total_size;
                new_free_block->next = current->next;
                
                current->size = total_size;
                
                if (prev != NULL) {
                    prev->next = new_free_block;
                } else {
                    free_list_head = new_free_block;
                }
            } else {
                // 大小刚好，直接从链表中移除
                if (prev != NULL) {
                    prev->next = current->next;
                } else {
                    free_list_head = current->next;
                }
            }
            // 返回给用户的地址是头部之后的位置
            return (void*)((char*)current + sizeof(header_t));
        }
        prev = current;
        current = current->next;
    }

    // 没有找到足够大的空闲块
    return NULL;
}