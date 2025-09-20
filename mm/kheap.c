// mm/kheap.c - A simplified and robust version.

#include "kheap.h"
#include "common.h"
#include "string.h"
#include <stddef.h>

// --- 数据结构定义 ---

// 内存块的头部信息
typedef struct header {
    uint32_t magic;    // 魔术数字，用于校验
    uint32_t size;     // 内存块的总大小 (包括头部)
    _Bool is_free;     // 标记此内存块是否空闲
    struct header* next; // 指向下一个 *空闲* 内存块的指针
    struct header* prev; // 指向前一个 *空闲* 内存块的指针
} header_t;

// --- 全局变量 ---
static header_t* free_list_head = NULL; // 指向空闲链表的头部
extern uint32_t end;                  // 内核的结束地址，由 linker.ld 提供

// --- 内部辅助函数 ---

// 将一个内存块从空闲链表中移除
static void remove_from_free_list(header_t* block) {
    if (block->prev) {
        block->prev->next = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
    if (free_list_head == block) {
        free_list_head = block->next;
    }
    block->prev = NULL;
    block->next = NULL;
}

// 将一个内存块添加到空闲链表的头部
static void add_to_free_list(header_t* block) {
    block->is_free = 1;
    block->prev = NULL;
    block->next = free_list_head;
    if (free_list_head) {
        free_list_head->prev = block;
    }
    free_list_head = block;
}

// --- 核心功能实现 ---

void init_kheap() {
    // 将堆的起始位置设置在内核 BSS 段之后，并进行页对齐
    uint32_t heap_addr = (uint32_t)&end;
    if (heap_addr & 0xFFF) {
        heap_addr &= 0xFFFFF000;
        heap_addr += 0x1000;
    }
    
    // 假设我们有 1MB 的堆空间 (可以根据需要调整)
    uint32_t heap_size = 1024 * 1024;
    
    free_list_head = (header_t*)heap_addr;
    free_list_head->magic = 0xDEADBEEF;
    free_list_head->size = heap_size;
    free_list_head->prev = NULL;
    free_list_head->next = NULL;
    
    add_to_free_list(free_list_head);

    kprint("Linked-list heap initialized.\n");
}

// kfree (简化版): 只标记为空闲并加回链表，不进行合并。
// 这可以避免复杂的指针操作带来的bug，对于当前阶段的内核来说足够稳定。
void kfree(void* p) {
    if (p == NULL) {
        return;
    }

    // 通过用户指针找到内存块的头部
    header_t* header = (header_t*)((char*)p - sizeof(header_t));

    // 校验魔术数字，防止释放野指针
    if (header->magic != 0xDEADBEEF) {
        kprint("Error: Invalid pointer passed to kfree()!\n");
        return;
    }

    add_to_free_list(header);
}

void* kmalloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // 实际需要分配的大小 = 用户请求的大小 + 头部大小，并对齐
    uint32_t required_size = size + sizeof(header_t);
    
    // 遍历空闲链表
    header_t* current = free_list_head;
    while (current) {
        // 寻找一个足够大的空闲块
        if (current->size >= required_size) {
            
            // 如果这个块比需要的大很多，就进行分割
            if (current->size > required_size + sizeof(header_t)) {
                // 创建一个新头部，代表分割后剩下的空闲部分
                header_t* remainder = (header_t*)((char*)current + required_size);
                remainder->magic = 0xDEADBEEF;
                remainder->size = current->size - required_size;
                add_to_free_list(remainder); // 将新的、更小的空闲块加回链表
                
                // 调整当前块的大小，它即将被分配出去
                current->size = required_size;
            }
            
            // 标记此块为已使用并从空闲链表移除
            current->is_free = 0;
            remove_from_free_list(current);
            
            // 返回给用户的是头部后面的地址
            return (void*)((char*)current + sizeof(header_t));
        }
        current = current->next;
    }

    kprint("Error: kmalloc failed, out of memory!\n");
    return NULL; // 没有找到合适的块
}

// kmalloc_a (页对齐分配) 的实现可以保持简化，或根据需要实现
void* kmalloc_a(uint32_t size) {
    // 这个实现会浪费内存，并且不能被kfree，但在当前阶段是可用的
    void* mem = kmalloc(size + 4096);
    uint32_t addr = (uint32_t)mem;
    if (addr & 0xFFF) {
        addr &= 0xFFFFF000;
        addr += 0x1000;
    }
    return (void*)addr;
}