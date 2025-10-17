#include "fat16.h"
#include "../drivers/ata.h"
#include "string.h" // for memset
#include "../mm/kheap.h"
#include "common.h"
#include <stddef.h>

fat16_boot_sector_t bpb;

// 0 代表根目录
uint16_t current_directory_cluster = 0;

// 辅助函数：将簇号转换为 LBA 扇区地址
static uint32_t cluster_to_lba(uint16_t cluster) {
    if (cluster < 2) return 0; // 安全检查
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    uint32_t data_area_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat) + root_dir_sectors;
    return data_area_start_sector + (cluster - 2) * bpb.sectors_per_cluster;
}

// 辅助函数：更新FAT表并写回硬盘
static void fat16_update_fat(uint16_t cluster, uint16_t value) {
    uint32_t fat_start_sector = bpb.reserved_sector_count;
    uint32_t fat_size_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;
    uint16_t* fat_buffer = (uint16_t*)kmalloc(fat_size_bytes);
    
    for (uint32_t i = 0; i < bpb.sectors_per_fat; i++) {
        ata_read_sector(fat_start_sector + i, (uint8_t*)fat_buffer + (i * bpb.bytes_per_sector));
    }

    fat_buffer[cluster] = value;

    uint32_t fat_sector_offset = (cluster * 2) / bpb.bytes_per_sector;
    uint8_t* sector_ptr = (uint8_t*)fat_buffer + fat_sector_offset * bpb.bytes_per_sector;
    ata_write_sector(fat_start_sector + fat_sector_offset, sector_ptr);
    ata_write_sector(fat_start_sector + bpb.sectors_per_fat + fat_sector_offset, sector_ptr);

    kfree(fat_buffer);
}

void fat16_format() {
    // 1. 准备一个512字节的缓冲区
    uint8_t boot_sector_buffer[512];
    memset(boot_sector_buffer, 0, 512);

    // 2. 填充引导扇区结构体 (BPB - BIOS Parameter Block)
    //    这些值是针对我们 10MB 的 hdd.img 计算的
    fat16_boot_sector_t* p_bpb = (fat16_boot_sector_t*)boot_sector_buffer;
    p_bpb->bytes_per_sector = 512;
    p_bpb->sectors_per_cluster = 1; // 每个簇1个扇区，即512字节
    p_bpb->reserved_sector_count = 1; // 引导扇区本身
    p_bpb->fat_count = 2;             // 2个FAT表
    p_bpb->root_entry_count = 512;    // 根目录最多512个条目
    p_bpb->total_sectors_short = 20480; // 10MB / 512 bytes = 20480 个扇区
    p_bpb->media_type = 0xF8;
    p_bpb->sectors_per_fat = 32;      // 根据簇数量计算得出
    p_bpb->sectors_per_track = 32;
    p_bpb->head_count = 64;
    p_bpb->total_sectors_long = 0;
    memcpy(p_bpb->fs_type, "FAT16   ", 8);
    memcpy(p_bpb->oem_name, "MYOS    ", 8);

    // 写入引导扇区签名
    boot_sector_buffer[510] = 0x55;
    boot_sector_buffer[511] = 0xAA;

    // 3. 将填充好的引导扇区写入硬盘的第一个扇区 (LBA 0)
    ata_write_sector(0, boot_sector_buffer);

    // 4. 清空 FAT 表区域
    //    FAT1 从第1个扇区开始，共32个扇区
    uint8_t empty_sector[512];
    memset(empty_sector, 0, 512);
    for (uint32_t i = 0; i < p_bpb->sectors_per_fat * p_bpb->fat_count; i++) {
        ata_write_sector(p_bpb->reserved_sector_count + i, empty_sector);
    }

    // 5. 清空根目录区域
    //    根目录区紧跟在两个FAT表之后
    uint32_t root_dir_start_sector = p_bpb->reserved_sector_count + (p_bpb->fat_count * p_bpb->sectors_per_fat);
    uint32_t root_dir_sectors = (p_bpb->root_entry_count * 32) / p_bpb->bytes_per_sector;
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        ata_write_sector(root_dir_start_sector + i, empty_sector);
    }
}

fat16_directory_t* fat16_read_directory(uint16_t cluster) {
    fat16_directory_t* dir = (fat16_directory_t*)kmalloc(sizeof(fat16_directory_t));
    uint8_t* buffer;

    if (cluster == 0) { // 读取根目录
        uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
        uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
        uint32_t root_dir_size_bytes = root_dir_sectors * bpb.bytes_per_sector;
        
        buffer = (uint8_t*)kmalloc(root_dir_size_bytes);
        for (uint32_t i = 0; i < root_dir_sectors; i++) {
            ata_read_sector(root_dir_start_sector + i, buffer + (i * bpb.bytes_per_sector));
        }
        dir->entries = (fat16_directory_entry_t*)buffer;
        dir->capacity = bpb.root_entry_count;
        dir->entry_count = bpb.root_entry_count; // For root, count is fixed
    } else { // 读取子目录
        // 子目录大小是动态的，我们先假设它至少有一个簇
        uint32_t dir_size_bytes = bpb.sectors_per_cluster * bpb.bytes_per_sector;
        buffer = (uint8_t*)kmalloc(dir_size_bytes); // TODO: 支持跨簇的目录
        
        uint32_t lba = cluster_to_lba(cluster);
        ata_read_sector(lba, buffer);

        dir->entries = (fat16_directory_entry_t*)buffer;
        dir->capacity = dir_size_bytes / 32;
        dir->entry_count = 0;
        for(uint32_t i=0; i<dir->capacity; i++) {
            if (dir->entries[i].filename[0] == 0x00) break;
            dir->entry_count++;
        }
    }
    return dir;
}


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

// 辅助函数：在目录中查找空条目或同名条目
static int find_entry_in_dir(fat16_directory_t* dir, const char* fat_filename) {
    for (uint32_t i = 0; i < dir->capacity; i++) {
        // 文件名第一个字节为0x00或0xE5表示空位
        if (!fat_filename && (dir->entries[i].filename[0] == 0x00 || (uint8_t)dir->entries[i].filename[0] == 0xE5)) {
            return i;
        }
        // 比较文件名
        if (fat_filename && memcmp(dir->entries[i].filename, fat_filename, 11) == 0) {
            return i;
        }
    }
    return -1;
}


// 这个函数用于在内核启动时读取并加载引导扇区信息
void init_fat16() {
    uint8_t boot_sector_buffer[512];
    ata_read_sector(0, boot_sector_buffer);
    memcpy(&bpb, boot_sector_buffer, sizeof(fat16_boot_sector_t));
}

fat16_directory_t* fat16_get_root_directory() {
    // 1. 计算根目录的起始扇区 LBA 地址
    uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);

    // 2. 计算根目录占用的总扇区数
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;

    // 3. 为整个根目录分配一块连续的内存
    uint32_t root_dir_size_bytes = root_dir_sectors * bpb.bytes_per_sector;
    uint8_t* root_dir_buffer = (uint8_t*)kmalloc(root_dir_size_bytes);

    // 4. 循环读取所有根目录扇区到内存
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        ata_read_sector(root_dir_start_sector + i, root_dir_buffer + (i * bpb.bytes_per_sector));
    }

    // 5. 创建并填充 fat16_directory_t 结构体
    fat16_directory_t* root_dir = (fat16_directory_t*)kmalloc(sizeof(fat16_directory_t));
    root_dir->entries = (fat16_directory_entry_t*)root_dir_buffer;
    root_dir->entry_count = bpb.root_entry_count;

    return root_dir;
}

void fat16_touch(const char* filename) {
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    // 使用通用函数读取当前目录
    fat16_directory_t* current_dir = fat16_read_directory(current_directory_cluster);

    if (find_entry_in_dir(current_dir, fat_filename) != -1) {
        kprint("\nError: File or directory already exists.");
        kfree(current_dir->entries);
        kfree(current_dir);
        return;
    }

    int free_entry_index = find_entry_in_dir(current_dir, NULL);
    if (free_entry_index == -1) {
        kprint("\nError: Directory is full.");
        kfree(current_dir->entries);
        kfree(current_dir);
        return;
    }

    // 填充新的目录条目信息
    fat16_directory_entry_t* new_entry = &current_dir->entries[free_entry_index];
    memcpy(new_entry->filename, fat_filename, 11);
    new_entry->attributes = 0x20; // 存档位 (普通文件)
    new_entry->file_size = 0;
    new_entry->first_cluster_low = 0;

    // 计算被修改的扇区并写回
    uint32_t sector_to_write;
    uint8_t* buffer_to_write;
    if (current_directory_cluster == 0) {
        uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
        sector_to_write = root_dir_start_sector + (free_entry_index * 32) / bpb.bytes_per_sector;
        buffer_to_write = (uint8_t*)current_dir->entries + (sector_to_write - root_dir_start_sector) * bpb.bytes_per_sector;
    } else {
        sector_to_write = cluster_to_lba(current_directory_cluster) + (free_entry_index * 32) / bpb.bytes_per_sector;
        buffer_to_write = (uint8_t*)current_dir->entries + ((free_entry_index * 32) - ((free_entry_index * 32) % bpb.bytes_per_sector));
    }
    ata_write_sector(sector_to_write, buffer_to_write);

    kfree(current_dir->entries);
    kfree(current_dir);
}

void fat16_mkdir(const char* dirname) {
    char fat_filename[11];
    to_fat16_filename(dirname, fat_filename);

    fat16_directory_t* parent_dir = fat16_read_directory(current_directory_cluster);

    if (find_entry_in_dir(parent_dir, fat_filename) != -1) {
        kprint("\nError: File or directory already exists.");
        kfree(parent_dir->entries);
        kfree(parent_dir);
        return;
    }

    int free_entry_index = find_entry_in_dir(parent_dir, NULL);
    if (free_entry_index == -1) {
        kprint("\nError: Directory is full.");
        kfree(parent_dir->entries);
        kfree(parent_dir);
        return;
    }

    uint16_t new_cluster = fat16_find_free_cluster();
    if (new_cluster == 0) {
        kprint("\nError: Disk is full.");
        kfree(parent_dir->entries);
        kfree(parent_dir);
        return;
    }
    fat16_update_fat(new_cluster, 0xFFFF); // 标记为目录链的结束

    // 1. 填充父目录中的新条目
    fat16_directory_entry_t* new_entry = &parent_dir->entries[free_entry_index];
    memcpy(new_entry->filename, fat_filename, 11);
    new_entry->attributes = 0x10; // 目录属性
    new_entry->file_size = 0;
    new_entry->first_cluster_low = new_cluster;
    // (省略时间日期)

    // 2. 将修改后的父目录扇区写回
    uint32_t sector_to_write;
    uint8_t* buffer_to_write;
    if (current_directory_cluster == 0) { // 父目录是根目录
        uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
        sector_to_write = root_dir_start_sector + (free_entry_index * 32) / bpb.bytes_per_sector;
        buffer_to_write = (uint8_t*)parent_dir->entries + (sector_to_write - root_dir_start_sector) * bpb.bytes_per_sector;
    } else { // 父目录是子目录
        sector_to_write = cluster_to_lba(current_directory_cluster) + (free_entry_index * 32) / bpb.bytes_per_sector;
         buffer_to_write = (uint8_t*)parent_dir->entries + ( (free_entry_index * 32) - ( (free_entry_index * 32) % bpb.bytes_per_sector) );
    }
    ata_write_sector(sector_to_write, buffer_to_write);
    kfree(parent_dir->entries);
    kfree(parent_dir);


    // 3. 创建新目录自己的数据块 (包含 . 和 ..)
    fat16_directory_entry_t* new_dir_content = (fat16_directory_entry_t*)kmalloc(bpb.bytes_per_sector);
    memset(new_dir_content, 0, bpb.bytes_per_sector);

    // 创建 "." 条目
    memcpy(new_dir_content[0].filename, ".          ", 11);
    new_dir_content[0].attributes = 0x10;
    new_dir_content[0].first_cluster_low = new_cluster;

    // 创建 ".." 条目
    memcpy(new_dir_content[1].filename, "..         ", 11);
    new_dir_content[1].attributes = 0x10;
    new_dir_content[1].first_cluster_low = current_directory_cluster; // 指向父目录

    // 4. 将这个新块写入新分配的簇
    ata_write_sector(cluster_to_lba(new_cluster), (uint8_t*)new_dir_content);
    kfree(new_dir_content);
}

uint16_t fat16_find_free_cluster() {
    // 1. 将整个 FAT1 读入内存
    uint32_t fat_start_sector = bpb.reserved_sector_count;
    uint32_t fat_size_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;
    uint16_t* fat_buffer = (uint16_t*)kmalloc(fat_size_bytes);
    
    for (uint32_t i = 0; i < bpb.sectors_per_fat; i++) {
        ata_read_sector(fat_start_sector + i, (uint8_t*)fat_buffer + (i * bpb.bytes_per_sector));
    }

    // 2. 遍历 FAT 条目寻找空闲簇
    //    簇号从 2 开始，0 和 1 是保留的
    //    总簇数 = (总扇区数 - 数据区起始扇区) / 每簇扇区数
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    uint32_t data_area_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat) + root_dir_sectors;
    uint32_t total_clusters = (bpb.total_sectors_short - data_area_start_sector) / bpb.sectors_per_cluster;

    uint16_t free_cluster = 0;
    for (uint16_t i = 2; i < total_clusters; i++) {
        if (fat_buffer[i] == 0x0000) {
            free_cluster = i;
            break;
        }
    }

    // 3. 释放内存并返回结果
    kfree(fat_buffer);
    return free_cluster;
}

// write 命令的实现 (支持多簇文件)
void fat16_write_content(const char* filename, const char* content) {
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    fat16_directory_t* current_dir = fat16_read_directory(current_directory_cluster);

    int entry_index = find_entry_in_dir(current_dir, fat_filename);

    if (entry_index == -1) {
        kprint("\nError: File not found.");
        kfree(current_dir->entries);
        kfree(current_dir);
        return;
    }

    fat16_directory_entry_t* entry = &current_dir->entries[entry_index];
    if (entry->attributes & 0x10) { // 如果是目录
        kprint("\nError: Cannot write to a directory.");
        kfree(current_dir->entries);
        kfree(current_dir);
        return;
    }

    // 确保文件大小为0，如果不是，需要先清空簇链。为了简化，我们假设之前已清空
    if (entry->first_cluster_low != 0) {
        // (这里应该有清空旧簇链的逻辑，但为了简化，我们先跳过)
    }

    uint16_t first_cluster = 0;
    uint16_t current_cluster = 0;
    uint32_t content_len = strlen(content);
    uint32_t bytes_written = 0;
    uint8_t* content_ptr = (uint8_t*)content;

    while (bytes_written < content_len) {
        uint16_t next_cluster = fat16_find_free_cluster();
        if (next_cluster == 0) {
            kprint("\nError: Disk is full.");
            break;
        }

        if (current_cluster != 0) {
            fat16_update_fat(current_cluster, next_cluster);
        } else {
            first_cluster = next_cluster;
        }
        current_cluster = next_cluster;

        uint8_t cluster_buffer[512];
        memset(cluster_buffer, 0, 512);
        uint32_t bytes_to_write = (content_len - bytes_written) > 512 ? 512 : (content_len - bytes_written);
        memcpy(cluster_buffer, content_ptr, bytes_to_write);

        ata_write_sector(cluster_to_lba(current_cluster), cluster_buffer);

        bytes_written += bytes_to_write;
        content_ptr += bytes_to_write;
    }

    if (current_cluster != 0) {
        fat16_update_fat(current_cluster, 0xFFFF);
    }

    entry->first_cluster_low = first_cluster;
    entry->file_size = content_len;

    uint32_t sector_to_write;
    uint8_t* buffer_to_write;
    if (current_directory_cluster == 0) {
        uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
        sector_to_write = root_dir_start_sector + (entry_index * 32) / bpb.bytes_per_sector;
        buffer_to_write = (uint8_t*)current_dir->entries + (sector_to_write - root_dir_start_sector) * bpb.bytes_per_sector;
    } else {
        sector_to_write = cluster_to_lba(current_directory_cluster) + (entry_index * 32) / bpb.bytes_per_sector;
        buffer_to_write = (uint8_t*)current_dir->entries + ((entry_index * 32) - ((entry_index * 32) % bpb.bytes_per_sector));
    }
    ata_write_sector(sector_to_write, buffer_to_write);

    kfree(current_dir->entries);
    kfree(current_dir);
}

// cat 命令的实现 (支持多簇文件)
void fat16_cat(const char* filename) {
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    fat16_directory_t* current_dir = fat16_read_directory(current_directory_cluster);
    int entry_index = find_entry_in_dir(current_dir, fat_filename);

    if (entry_index == -1) {
        kprint("\nError: File not found.");
        kfree(current_dir->entries);
        kfree(current_dir);
        return;
    }

    fat16_directory_entry_t* entry = &current_dir->entries[entry_index];
    uint16_t current_cluster = entry->first_cluster_low;
    uint32_t file_size = entry->file_size;

    if (current_cluster == 0) {
        kprint("\nFile is empty.");
        kfree(current_dir->entries);
        kfree(current_dir);
        return;
    }

    uint32_t fat_start_sector = bpb.reserved_sector_count;
    uint32_t fat_size_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;
    uint16_t* fat_buffer = (uint16_t*)kmalloc(fat_size_bytes);
    for (uint32_t i = 0; i < bpb.sectors_per_fat; i++) {
        ata_read_sector(fat_start_sector + i, (uint8_t*)fat_buffer + (i * bpb.bytes_per_sector));
    }

    kprint("\n");
    uint8_t* read_buffer = (uint8_t*)kmalloc(bpb.bytes_per_sector);
    uint32_t bytes_remaining = file_size;

    while (1) {
        uint32_t lba = cluster_to_lba(current_cluster);
        ata_read_sector(lba, read_buffer);

        uint32_t bytes_to_print = (bytes_remaining > bpb.bytes_per_sector) ? bpb.bytes_per_sector : bytes_remaining;
        for (uint32_t i = 0; i < bytes_to_print; i++) {
            kputc(read_buffer[i]);
        }
        bytes_remaining -= bytes_to_print;

        current_cluster = fat_buffer[current_cluster];

        if (current_cluster >= 0xFFF8 || bytes_remaining == 0) {
            break;
        }
    }

    kfree(current_dir->entries);
    kfree(current_dir);
    kfree(fat_buffer);
    kfree(read_buffer);
}

void fat16_append(const char* filename, const char* content) {
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    fat16_directory_t* current_dir = fat16_read_directory(current_directory_cluster);
    int entry_index = find_entry_in_dir(current_dir, fat_filename);

    if (entry_index == -1) {
        kprint("\nError: File not found.");
        kfree(current_dir->entries);
        kfree(current_dir);
        return;
    }

    fat16_directory_entry_t* entry = &current_dir->entries[entry_index];
    if (entry->attributes & 0x10) {
        kprint("\nError: Cannot append to a directory.");
        kfree(current_dir->entries);
        kfree(current_dir);
        return;
    }

    uint16_t current_cluster = entry->first_cluster_low;
    uint32_t file_size = entry->file_size;

    if (current_cluster == 0) {
        kfree(current_dir->entries);
        kfree(current_dir);
        fat16_write_content(filename, content);
        return;
    }

    uint32_t fat_size_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;
    uint16_t* fat_buffer = (uint16_t*)kmalloc(fat_size_bytes);
    uint32_t fat_start_sector = bpb.reserved_sector_count;
    for (uint32_t i = 0; i < bpb.sectors_per_fat; i++) {
        ata_read_sector(fat_start_sector + i, (uint8_t*)fat_buffer + (i * bpb.bytes_per_sector));
    }

    while (fat_buffer[current_cluster] < 0xFFF8) {
        current_cluster = fat_buffer[current_cluster];
    }
    kfree(fat_buffer);

    uint8_t* cluster_buffer = (uint8_t*)kmalloc(bpb.bytes_per_sector);
    ata_read_sector(cluster_to_lba(current_cluster), cluster_buffer);

    uint32_t offset_in_cluster = file_size % bpb.bytes_per_sector;
    uint32_t space_in_cluster = bpb.bytes_per_sector - offset_in_cluster;
    uint32_t content_len = strlen(content);
    uint8_t* content_ptr = (uint8_t*)content;
    uint32_t bytes_to_write_now = (content_len < space_in_cluster) ? content_len : space_in_cluster;

    memcpy(cluster_buffer + offset_in_cluster, content_ptr, bytes_to_write_now);
    ata_write_sector(cluster_to_lba(current_cluster), cluster_buffer);

    content_ptr += bytes_to_write_now;
    uint32_t bytes_remaining = content_len - bytes_to_write_now;

    while (bytes_remaining > 0) {
        uint16_t next_cluster = fat16_find_free_cluster();
        if (next_cluster == 0) {
            kprint("\nError: Disk is full.");
            break;
        }

        fat16_update_fat(current_cluster, next_cluster);
        current_cluster = next_cluster;

        memset(cluster_buffer, 0, bpb.bytes_per_sector);
        bytes_to_write_now = (bytes_remaining > 512) ? 512 : bytes_remaining;
        memcpy(cluster_buffer, content_ptr, bytes_to_write_now);
        ata_write_sector(cluster_to_lba(current_cluster), cluster_buffer);

        content_ptr += bytes_to_write_now;
        bytes_remaining -= bytes_to_write_now;
    }

    fat16_update_fat(current_cluster, 0xFFFF);
    kfree(cluster_buffer);

    entry->file_size += content_len;
    uint32_t sector_to_write;
    uint8_t* buffer_to_write;
    if (current_directory_cluster == 0) {
        uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
        sector_to_write = root_dir_start_sector + (entry_index * 32) / bpb.bytes_per_sector;
        buffer_to_write = (uint8_t*)current_dir->entries + (sector_to_write - root_dir_start_sector) * bpb.bytes_per_sector;
    } else {
        sector_to_write = cluster_to_lba(current_directory_cluster) + (entry_index * 32) / bpb.bytes_per_sector;
        buffer_to_write = (uint8_t*)current_dir->entries + ((entry_index * 32) - ((entry_index * 32) % bpb.bytes_per_sector));
    }
    ata_write_sector(sector_to_write, buffer_to_write);
    kfree(current_dir->entries);
    kfree(current_dir);
}

void fat16_cd(const char* dirname) {
    // 处理 "cd .."
    if (strcmp(dirname, "..") == 0) {
        // 在根目录时，"cd .." 仍然是根目录
        if (current_directory_cluster == 0) {
            return;
        }
        // 读取当前目录以找到 ".." 条目
        fat16_directory_t* dir = fat16_read_directory(current_directory_cluster);
        // ".." 总是第二个条目
        current_directory_cluster = dir->entries[1].first_cluster_low;
        kfree(dir->entries);
        kfree(dir);
        return;
    }
    
    // 处理 "cd ."
    if (strcmp(dirname, ".") == 0) {
        return; // 什么都不做
    }
    
    // 处理 "cd /"
    if (strcmp(dirname, "/") == 0) {
        current_directory_cluster = 0;
        return;
    }

    char fat_filename[11];
    to_fat16_filename(dirname, fat_filename);
    
    fat16_directory_t* dir = fat16_read_directory(current_directory_cluster);
    int entry_index = find_entry_in_dir(dir, fat_filename);

    if (entry_index == -1) {
        kprint("\nError: Directory not found: ");
        kprint(dirname);
    } else {
        fat16_directory_entry_t* entry = &dir->entries[entry_index];
        if (entry->attributes & 0x10) { // 检查是否是目录
            current_directory_cluster = entry->first_cluster_low;
        } else {
            kprint("\nError: Not a directory: ");
            kprint(dirname);
        }
    }

    kfree(dir->entries);
    kfree(dir);
}


// 实现获取当前路径的函数
void fat16_get_current_path(char* path_buffer) {
    if (current_directory_cluster == 0) {
        strcpy(path_buffer, "/");
        return;
    }

    char temp_path[256] = {0};
    uint16_t cluster = current_directory_cluster;
    uint16_t parent_cluster;

    while (cluster != 0) {
        fat16_directory_t* dir = fat16_read_directory(cluster);
        parent_cluster = dir->entries[1].first_cluster_low; // ".." 条目

        fat16_directory_t* parent_dir = fat16_read_directory(parent_cluster);
        char current_name[13] = {0};

        // 在父目录中找到当前目录的条目以获取其名称
        for (uint32_t i = 0; i < parent_dir->capacity; i++) {
            if (parent_dir->entries[i].first_cluster_low == cluster && (parent_dir->entries[i].attributes & 0x10)) {
                 int k = 0;
                for (int j = 0; j < 8; j++) {
                    if (parent_dir->entries[i].filename[j] == ' ') break;
                    current_name[k++] = parent_dir->entries[i].filename[j];
                }
                current_name[k] = '\0';
                break;
            }
        }
        
        char part[256] = "/";
        strcat(part, current_name);
        strcat(part, temp_path);
        strcpy(temp_path, part);

        kfree(dir->entries);
        kfree(dir);
        kfree(parent_dir->entries);
        kfree(parent_dir);
        cluster = parent_cluster;
    }
    
    if (strlen(temp_path) == 0) {
        strcpy(path_buffer, "/");
    } else {
        strcpy(path_buffer, temp_path);
    }
}