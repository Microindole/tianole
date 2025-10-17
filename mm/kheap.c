#include "kheap.h"
#include "common.h"
#include "string.h"
#include <stddef.h>

// 内存块的头部信息
typedef struct header {
    uint32_t magic;    // 魔术数字，用于校验
    uint32_t size;     // 内存块的总大小 (包括头部)
    _Bool is_free;     // 标记此内存块是否空闲
    struct header* next; // 指向下一个内存块的指针
    struct header* prev; // 指向前一个内存块的指针
} header_t;

static header_t* heap_start = NULL; // 指向堆的起始地址
extern uint32_t end;                 // 内核的结束地址，由 linker.ld 提供


// 将一个内存块从空闲链表中移除
static void remove_from_free_list(header_t* block) {
    if (block->prev) {
        block->prev->next = block->next;
    }
    if (block->next) {
        block->next->prev = block->prev;
    }
    // 如果移除的是链表头，需要更新头指针
    if (heap_start == block) {
        heap_start = block->next;
    }
    block->prev = NULL;
    block->next = NULL;
}

// 将一个内存块添加到空闲链表的头部
static void add_to_free_list(header_t* block) {
    block->prev = NULL;
    block->next = heap_start;
    if (heap_start) {
        heap_start->prev = block;
    }
    heap_start = block;
}


void init_kheap() {
    // 将堆的起始位置设置在内核 BSS 段之后，并进行页对齐
    uint32_t heap_addr = (uint32_t)&end;
    if (heap_addr & 0xFFF) {
        heap_addr &= 0xFFFFF000;
        heap_addr += 0x1000;
    }
    
    uint32_t heap_size = 1024 * 1024;
    
    heap_start = (header_t*)heap_addr;
    heap_start->magic = 0xDEADBEEF;
    heap_start->size = heap_size;
    heap_start->is_free = 1;
    heap_start->prev = NULL;
    heap_start->next = NULL;

    kprint("Linked-list heap initialized.\n");
}

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

    header->is_free = 1;
    add_to_free_list(header);

    header_t* next_block = (header_t*)((char*)header + header->size);
    if (next_block->magic == 0xDEADBEEF && next_block->is_free) {
        remove_from_free_list(next_block);
        header->size += next_block->size;
    }

    header_t* prev_block = header->prev; // 从链表中找
    if (prev_block && prev_block->is_free && ((char*)prev_block + prev_block->size) == (char*)header) {
        remove_from_free_list(header);
        prev_block->size += header->size;
    }
}

void* kmalloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // 实际需要分配的大小 = 用户请求的大小 + 头部大小
    uint32_t required_size = size + sizeof(header_t);

    header_t* current = heap_start;
    while (current) {
        // 寻找一个足够大的空闲块
        if (current->is_free && current->size >= required_size) {
            
            // 如果这个块比需要的大很多，就进行分割
            if (current->size > required_size + sizeof(header_t)) {
                header_t* new_block = (header_t*)((char*)current + required_size);
                new_block->magic = 0xDEADBEEF;
                new_block->is_free = 1;
                new_block->size = current->size - required_size;
                
                // 新分割出的块加入空闲链表
                add_to_free_list(new_block);
                remove_from_free_list(current); // 从旧位置移除
                add_to_free_list(new_block); // 添加新块

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
    // 简单的实现：多分配一些，然后返回对齐的地址
    // 注意：这会浪费一些内存，并且 kfree 无法正确处理
    void* mem = kmalloc(size + 4096);
    uint32_t addr = (uint32_t)mem;
    if (addr & 0xFFF) {
        addr &= 0xFFFFF000;
        addr += 0x1000;
    }
    return (void*)addr;
}