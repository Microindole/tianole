#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "../cpu/isr.h"

// 页表项 (PTE)
typedef struct page_table_entry {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t unused     : 7;
    uint32_t frame      : 20;
} __attribute__((packed)) pte_t;

// 页目录项 (PDE)
typedef struct page_directory_entry {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t unused     : 8;
    uint32_t page_table_addr : 20;
} __attribute__((packed)) pde_t;

// 页表
typedef struct page_table {
    uint32_t entries[1024];
} page_table_t;

// 页目录
typedef struct page_directory {
    uint32_t entries[1024];
} page_directory_t;

// --- 关键修正：添加缺失的函数声明 ---
extern void load_page_directory(page_directory_t*);

// 初始化分页系统
void init_paging();

#endif
