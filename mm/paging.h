#ifndef PAGING_H
#define PAGING_H

#include "common.h"
#include "../cpu/isr.h"

// --- 以下的结构体定义只用于“解释”一个32位整数，而不是直接使用 ---
// 页表项 (PTE) 的位域结构
typedef struct page_table_entry {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t dirty      : 1;
    uint32_t unused     : 7;
    uint32_t frame      : 20;
} __attribute__((packed)) pte_t;

// 页目录项 (PDE) 的位域结构
typedef struct page_directory_entry {
    uint32_t present    : 1;
    uint32_t rw         : 1;
    uint32_t user       : 1;
    uint32_t accessed   : 1;
    uint32_t unused     : 8;
    uint32_t page_table_addr : 20;
} __attribute__((packed)) pde_t;
// --- 结构体定义结束 ---


// --- 这是实际在内存中使用的结构 ---
// 页表 (包含 1024 个 uint32_t 条目)
typedef struct page_table {
    uint32_t entries[1024];
} page_table_t;

// 页目录 (包含 1024 个 uint32_t 条目)
typedef struct page_directory {
    uint32_t entries[1024];
} page_directory_t;
// --- 实际结构结束 ---

// 初始化分页系统
void init_paging();

#endif