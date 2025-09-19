// mm/kheap.c (Simplified Bump Allocator for Debugging)
// NOTE: This version does not support kfree(). It is for testing only.

#include "kheap.h"
#include "common.h"
#include <stddef.h>

extern uint32_t end; // Provided by the linker script
static uint32_t heap_ptr = 0;

void init_kheap() {
    heap_ptr = (uint32_t)&end;
    // Align to a 4K boundary to be safe
    if (heap_ptr & 0xFFF) {
        heap_ptr &= 0xFFFFF000;
        heap_ptr += 0x1000;
    }
    kprint("Simplified bump-allocator heap initialized.\n");
}

// kfree does nothing in this simple allocator
void kfree(void* p) {
    (void)p; // Suppress unused parameter warning
}

void* kmalloc(uint32_t size) {
    if (size == 0) {
        return NULL;
    }

    // Remember the current position
    uint32_t new_ptr = heap_ptr;
    
    // "Bump" the pointer forward by the requested size
    heap_ptr += size;
    
    // In a real kernel, we should check if heap_ptr exceeds available memory.
    // For this test, we assume we have enough.
    
    return (void*)new_ptr;
}

void* kmalloc_a(uint32_t size) {
    // Align the heap pointer to a 4K boundary before allocating
    if (heap_ptr & 0xFFF) {
        heap_ptr &= 0xFFFFF000;
        heap_ptr += 0x1000;
    }
    return kmalloc(size);
}
