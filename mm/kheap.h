// mm/kheap.h
#ifndef KHEAP_H
#define KHEAP_H

#include "common.h"

// 初始化堆
void init_kheap();

// 分配一块对齐的内存
void* kmalloc(uint32_t size);

// 释放一块内存
void kfree(void* p);

#endif
