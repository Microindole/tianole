#include "exec.h"
#include "common.h"
#include "string.h"
#include "task.h"
#include "../mm/kheap.h"
#include "../mm/paging.h"
#include "../fs/fat16.h"
#include "cpu/gdt.h"
#include "cpu/usermode.h"
#include "../drivers/ata.h"
#include <stddef.h>

extern fat16_boot_sector_t bpb;
extern uint16_t current_directory_cluster;
extern volatile task_t* current_task;

// 辅助函数：将簇号转换为LBA扇区地址（复制自fat16.c）
static uint32_t cluster_to_lba(uint16_t cluster) {
    if (cluster < 2) return 0;
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    uint32_t data_area_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat) + root_dir_sectors;
    return data_area_start_sector + (cluster - 2) * bpb.sectors_per_cluster;
}

// 辅助函数：读取FAT表项
static uint16_t fat16_get_next_cluster(uint16_t cluster) {
    uint32_t fat_start_sector = bpb.reserved_sector_count;
    uint32_t fat_offset = cluster * 2;
    uint32_t fat_sector = fat_start_sector + (fat_offset / bpb.bytes_per_sector);
    uint32_t entry_offset = fat_offset % bpb.bytes_per_sector;

    uint8_t sector_buffer[512];
    ata_read_sector(fat_sector, sector_buffer);

    uint16_t table_value = *(uint16_t*)(sector_buffer + entry_offset);
    return table_value;
}

// 辅助函数：转换文件名为FAT16格式
static void to_fat16_filename(const char* filename, char* out_name) {
    memset(out_name, ' ', 11);

    int dot_pos = -1;
    for (int i = 0; filename[i] != '\0'; i++) {
        if (filename[i] == '.') {
            dot_pos = i;
            break;
        }
    }

    if (dot_pos == -1) {
        for (int i = 0; i < 8 && filename[i] != '\0'; i++) {
            out_name[i] = toupper(filename[i]);
        }
    } else {
        for (int i = 0; i < 8 && i < dot_pos; i++) {
            out_name[i] = toupper(filename[i]);
        }
        for (int i = 0; i < 3 && filename[dot_pos + 1 + i] != '\0'; i++) {
            out_name[8 + i] = toupper(filename[dot_pos + 1 + i]);
        }
    }
}

// 辅助函数：从文件系统读取整个文件内容
static uint8_t* read_file_content(const char* filename, uint32_t* out_size) {
    // 读取当前目录
    fat16_directory_t* dir = fat16_read_directory(current_directory_cluster);
    if (!dir) {
        return NULL;
    }

    // 转换文件名
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    // 查找文件
    fat16_directory_entry_t* entry = NULL;
    for (uint32_t i = 0; i < dir->capacity; i++) {
        if (dir->entries[i].filename[0] == 0x00) break;
        if (dir->entries[i].filename[0] == 0xE5) continue;
        if (dir->entries[i].attributes & 0x10) continue; // 跳过目录

        if (memcmp(dir->entries[i].filename, fat_filename, 11) == 0) {
            entry = &dir->entries[i];
            break;
        }
    }

    if (!entry) {
        kfree(dir->entries);
        kfree(dir);
        return NULL;
    }

    // 分配缓冲区
    uint32_t file_size = entry->file_size;
    uint8_t* buffer = (uint8_t*)kmalloc(file_size);
    if (!buffer) {
        kfree(dir->entries);
        kfree(dir);
        return NULL;
    }

    // 读取文件内容
    uint16_t current_cluster = entry->first_cluster_low;
    uint32_t bytes_read = 0;

    while (current_cluster >= 2 && current_cluster < 0xFFF8) {
        uint32_t lba = cluster_to_lba(current_cluster);

        for (uint32_t i = 0; i < bpb.sectors_per_cluster; i++) {
            if (bytes_read >= file_size) break;
            ata_read_sector(lba + i, buffer + bytes_read);
            bytes_read += bpb.bytes_per_sector;
        }

        current_cluster = fat16_get_next_cluster(current_cluster);
    }

    kfree(dir->entries);
    kfree(dir);

    *out_size = file_size;
    return buffer;
}

// 辅助函数：将物理帧映射到虚拟地址（用户态可访问）
static void map_user_page(uint32_t virtual_addr, uint32_t physical_frame) {
    page_directory_t* dir = (page_directory_t*)current_task->directory;

    uint32_t pde_index = virtual_addr >> 22;
    uint32_t pte_index = (virtual_addr >> 12) & 0x3FF;

    // 检查页表是否存在
    if (!(dir->entries[pde_index] & 0x1)) {
        // 分配新页表
        page_table_t* table = (page_table_t*)kmalloc_a(sizeof(page_table_t));
        memset(table, 0, sizeof(page_table_t));

        // 设置页目录项：present=1, rw=1, user=1
        dir->entries[pde_index] = ((uint32_t)table) | 0x7;
    }

    // 获取页表
    page_table_t* table = (page_table_t*)(dir->entries[pde_index] & ~0xFFF);

    // 设置页表项：present=1, rw=1, user=1
    table->entries[pte_index] = (physical_frame << 12) | 0x7;
}

// exec系统调用的实现
int do_exec(const char* filename) {
    // 1. 从文件系统读取文件
    uint32_t file_size;
    uint8_t* file_content = read_file_content(filename, &file_size);

    if (!file_content) {
        kprint("Error: File not found: ");
        kprint(filename);
        kprint("\n");
        return -1;
    }

    // 2. 计算需要的页面数
    uint32_t pages_needed = (file_size + 0xFFF) / 0x1000;

    // 3. 为用户程序分配物理内存并映射到用户态地址空间
    for (uint32_t i = 0; i < pages_needed; i++) {
        // 分配物理页面
        void* physical_page = kmalloc_a(0x1000);
        uint32_t virtual_addr = USER_PROGRAM_BASE + (i * 0x1000);

        // 清零页面
        memset(physical_page, 0, 0x1000);

        // 复制文件内容
        uint32_t offset = i * 0x1000;
        uint32_t bytes_to_copy = (file_size - offset < 0x1000) ?
                                 (file_size - offset) : 0x1000;
        memcpy(physical_page, file_content + offset, bytes_to_copy);

        // 映射到虚拟地址空间
        map_user_page(virtual_addr, (uint32_t)physical_page >> 12);
    }

    // 4. 为用户程序分配栈
    void* user_stack_physical = kmalloc_a(USER_STACK_SIZE);
    memset(user_stack_physical, 0, USER_STACK_SIZE);

    // 映射用户栈（栈向下增长，所以映射在USER_STACK_TOP - USER_STACK_SIZE处）
    for (uint32_t i = 0; i < USER_STACK_SIZE / 0x1000; i++) {
        uint32_t virtual_addr = USER_STACK_TOP - USER_STACK_SIZE + (i * 0x1000);
        uint32_t physical_addr = (uint32_t)user_stack_physical + (i * 0x1000);
        map_user_page(virtual_addr, physical_addr >> 12);
    }

    // 5. 设置TSS中的内核栈（用于从用户态陷入内核态）
    // 当前任务的内核栈
    set_kernel_stack(current_task->kernel_stack_ptr);

    // 6. 释放文件缓冲区
    kfree(file_content);

    // 7. 切换到用户态并执行
    kprint("Switching to user mode, entry point: ");
    char hex_buf[12];
    itoa_hex(USER_PROGRAM_BASE, hex_buf, 12);
    kprint(hex_buf);
    kprint("\n");

    // 切换到用户态（这个函数不会返回）
    switch_to_usermode(USER_PROGRAM_BASE, USER_STACK_TOP);

    return 0;
}

// exec系统调用处理函数
void syscall_exec(registers_t* regs) {
    const char* filename = (const char*)regs->ebx;
    int result = do_exec(filename);
    regs->eax = result;
}
