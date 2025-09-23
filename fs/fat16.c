#include "fat16.h"
#include "../drivers/ata.h"
#include "string.h" // for memset
#include "../mm/kheap.h"
#include "common.h"

// 声明一个全局的引导扇区变量，方便后续访问
fat16_boot_sector_t bpb;

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


// 一个辅助函数，将 "HELLO.TXT" 这样的字符串转换为 FAT16 的 8.3 格式
static void to_fat16_filename(const char* filename, char* out_name) {
    memset(out_name, ' ', 11); // 先用空格填充

    int dot_pos = -1;
    for (int i = 0; filename[i] != '\0'; i++) {
        if (filename[i] == '.') {
            dot_pos = i;
            break;
        }
    }

    if (dot_pos == -1) { // 没有扩展名
        for (int i = 0; i < 8 && filename[i] != '\0'; i++) {
            out_name[i] = toupper(filename[i]);
        }
    } else { // 有扩展名
        for (int i = 0; i < 8 && i < dot_pos; i++) {
            out_name[i] = toupper(filename[i]);
        }
        for (int i = 0; i < 3 && filename[dot_pos + 1 + i] != '\0'; i++) {
            out_name[8 + i] = toupper(filename[dot_pos + 1 + i]);
        }
    }
}

void fat16_touch(const char* filename) {
    // 1. 将输入的文件名转换为 8.3 格式
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    // 2. 读取整个根目录到内存
    uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    uint32_t root_dir_size_bytes = root_dir_sectors * bpb.bytes_per_sector;
    fat16_directory_entry_t* root_dir_buffer = (fat16_directory_entry_t*)kmalloc(root_dir_size_bytes);
    
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        ata_read_sector(root_dir_start_sector + i, (uint8_t*)root_dir_buffer + (i * bpb.bytes_per_sector));
    }

    // 3. 寻找一个空的目录条目
    int free_entry_index = -1;
    for (uint32_t i = 0; i < bpb.root_entry_count; i++) {
        if (root_dir_buffer[i].filename[0] == 0x00 || (uint8_t)root_dir_buffer[i].filename[0] == 0xE5) {
            free_entry_index = i;
            break;
        }
    }

    if (free_entry_index == -1) {
        kprint("\nError: Root directory is full.");
        kfree(root_dir_buffer);
        return;
    }

    // 4. 填充新的目录条目信息
    fat16_directory_entry_t* new_entry = &root_dir_buffer[free_entry_index];
    memcpy(new_entry->filename, fat_filename, 8);
    memcpy(new_entry->extension, fat_filename + 8, 3);
    new_entry->attributes = 0x20; // 存档位 (普通文件)
    new_entry->file_size = 0;
    new_entry->first_cluster_low = 0;
    // (其他时间、日期字段暂时忽略，保持为0)

    // 5. 计算被修改的条目在哪个扇区
    uint32_t sector_to_write = root_dir_start_sector + (free_entry_index * 32) / bpb.bytes_per_sector;

    // 6. 将这个修改后的扇区写回硬盘
    ata_write_sector(sector_to_write, (uint8_t*)root_dir_buffer + (sector_to_write - root_dir_start_sector) * bpb.bytes_per_sector);

    // 7. 释放内存
    kfree(root_dir_buffer);
}

void fat16_mkdir(const char* dirname) {
    // 1. 将输入的目录名转换为 8.3 格式
    char fat_filename[11];
    to_fat16_filename(dirname, fat_filename);

    // 2. 读取整个根目录到内存
    uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    uint32_t root_dir_size_bytes = root_dir_sectors * bpb.bytes_per_sector;
    fat16_directory_entry_t* root_dir_buffer = (fat16_directory_entry_t*)kmalloc(root_dir_size_bytes);
    
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        ata_read_sector(root_dir_start_sector + i, (uint8_t*)root_dir_buffer + (i * bpb.bytes_per_sector));
    }

    // 3. 寻找一个空的目录条目
    int free_entry_index = -1;
    for (uint32_t i = 0; i < bpb.root_entry_count; i++) {
        if (root_dir_buffer[i].filename[0] == 0x00 || (uint8_t)root_dir_buffer[i].filename[0] == 0xE5) {
            free_entry_index = i;
            break;
        }
    }

    if (free_entry_index == -1) {
        kprint("\nError: Root directory is full.");
        kfree(root_dir_buffer);
        return;
    }

    // 4. 填充新的目录条目信息
    fat16_directory_entry_t* new_entry = &root_dir_buffer[free_entry_index];
    memcpy(new_entry->filename, fat_filename, 8);
    memcpy(new_entry->extension, fat_filename + 8, 3);
    
    // --- 核心区别在这里 ---
    new_entry->attributes = 0x10; // 0x10 代表这是一个目录 (ATTRIBUTE_DIRECTORY)
    // --- 核心区别结束 ---

    new_entry->file_size = 0;
    new_entry->first_cluster_low = 0; // 目录刚创建时也没有内容，所以簇号为0

    // 5. 计算被修改的条目在哪个扇区
    uint32_t sector_to_write = root_dir_start_sector + (free_entry_index * 32) / bpb.bytes_per_sector;

    // 6. 将这个修改后的扇区写回硬盘
    ata_write_sector(sector_to_write, (uint8_t*)root_dir_buffer + (sector_to_write - root_dir_start_sector) * bpb.bytes_per_sector);

    // 7. 释放内存
    kfree(root_dir_buffer);
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

// 辅助函数：将簇号转换为 LBA 扇区地址
static uint32_t cluster_to_lba(uint16_t cluster) {
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    uint32_t data_area_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat) + root_dir_sectors;
    return data_area_start_sector + (cluster - 2) * bpb.sectors_per_cluster;
}

// 辅助函数：更新FAT表并写回硬盘
static void fat16_update_fat(uint16_t cluster, uint16_t value) {
    uint32_t fat_start_sector = bpb.reserved_sector_count;
    uint32_t fat_size_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;
    uint16_t* fat_buffer = (uint16_t*)kmalloc(fat_size_bytes);
    
    // 将FAT1读入内存
    for (uint32_t i = 0; i < bpb.sectors_per_fat; i++) {
        ata_read_sector(fat_start_sector + i, (uint8_t*)fat_buffer + (i * bpb.bytes_per_sector));
    }

    // 在内存中更新FAT
    fat_buffer[cluster] = value;

    // 计算被修改的扇区
    uint32_t fat_sector_offset = (cluster * 2) / bpb.bytes_per_sector;

    // 将修改后的扇区同时写回两个FAT表
    uint8_t* sector_ptr = (uint8_t*)fat_buffer + fat_sector_offset * bpb.bytes_per_sector;
    ata_write_sector(fat_start_sector + fat_sector_offset, sector_ptr);
    ata_write_sector(fat_start_sector + bpb.sectors_per_fat + fat_sector_offset, sector_ptr);

    kfree(fat_buffer);
}

// write 命令的实现 (支持多簇文件)
void fat16_write_content(const char* filename, const char* content) {
    // --- 1. 找到文件的目录条目 (与之前类似) ---
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    fat16_directory_entry_t* root_dir_buffer = (fat16_directory_entry_t*)kmalloc(root_dir_sectors * bpb.bytes_per_sector);
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        ata_read_sector(root_dir_start_sector + i, (uint8_t*)root_dir_buffer + (i * bpb.bytes_per_sector));
    }

    int entry_index = -1;
    for (uint32_t i = 0; i < bpb.root_entry_count; i++) {
        if (memcmp(root_dir_buffer[i].filename, fat_filename, 11) == 0) {
            entry_index = i;
            break;
        }
    }

    if (entry_index == -1) {
        kprint("\nError: File not found.");
        kfree(root_dir_buffer);
        return;
    }

    // --- 2. 准备写入循环 ---
    uint16_t first_cluster = 0;
    uint16_t current_cluster = 0;
    uint32_t content_len = strlen(content);
    uint32_t bytes_written = 0;
    uint8_t* content_ptr = (uint8_t*)content;

    while (bytes_written < content_len) {
        // a. 寻找一个新的空闲簇
        uint16_t next_cluster = fat16_find_free_cluster();
        if (next_cluster == 0) {
            kprint("\nError: Disk is full.");
            // (注意：这里应该有一个回滚机制来释放已分配的簇，但我们暂时简化)
            break;
        }
        
        // b. 更新FAT表，将新簇链接到链上
        if (current_cluster != 0) {
            fat16_update_fat(current_cluster, next_cluster);
        } else {
            first_cluster = next_cluster; // 如果是第一个簇，记录下来
        }
        current_cluster = next_cluster;

        // c. 准备要写入的数据
        uint8_t cluster_buffer[512];
        memset(cluster_buffer, 0, 512);
        uint32_t bytes_to_write = (content_len - bytes_written) > 512 ? 512 : (content_len - bytes_written);
        memcpy(cluster_buffer, content_ptr, bytes_to_write);

        // d. 将数据写入数据区
        ata_write_sector(cluster_to_lba(current_cluster), cluster_buffer);

        // e. 更新计数器
        bytes_written += bytes_to_write;
        content_ptr += bytes_to_write;
    }

    // --- 3. 用文件结尾标记来终止簇链 ---
    if (current_cluster != 0) {
        fat16_update_fat(current_cluster, 0xFFFF);
    }

    // --- 4. 更新根目录条目 ---
    fat16_directory_entry_t* entry = &root_dir_buffer[entry_index];
    entry->first_cluster_low = first_cluster;
    entry->file_size = content_len;

    uint32_t dir_sector_offset = (entry_index * 32) / bpb.bytes_per_sector;
    ata_write_sector(root_dir_start_sector + dir_sector_offset, (uint8_t*)root_dir_buffer + dir_sector_offset * bpb.bytes_per_sector);
    
    kfree(root_dir_buffer);
}

// cat 命令的实现 (支持多簇文件)
void fat16_cat(const char* filename) {
    // --- 1. 找到目标文件的目录条目 ---
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    fat16_directory_entry_t* root_dir_buffer = (fat16_directory_entry_t*)kmalloc(root_dir_sectors * bpb.bytes_per_sector);
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        ata_read_sector(root_dir_start_sector + i, (uint8_t*)root_dir_buffer + (i * bpb.bytes_per_sector));
    }

    int entry_index = -1;
    for (uint32_t i = 0; i < bpb.root_entry_count; i++) {
        if (memcmp(root_dir_buffer[i].filename, fat_filename, 11) == 0) {
            entry_index = i;
            break;
        }
    }

    if (entry_index == -1) {
        kprint("\nError: File not found.");
        kfree(root_dir_buffer);
        return;
    }

    // --- 2. 获取文件的起始簇号和大小 ---
    fat16_directory_entry_t* entry = &root_dir_buffer[entry_index];
    uint16_t current_cluster = entry->first_cluster_low;
    uint32_t file_size = entry->file_size;

    if (current_cluster == 0) {
        kprint("\nFile is empty.");
        kfree(root_dir_buffer);
        return;
    }

    // --- 3. 将 FAT1 完整读入内存，用于查询下一个簇 ---
    uint32_t fat_start_sector = bpb.reserved_sector_count;
    uint32_t fat_size_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;
    uint16_t* fat_buffer = (uint16_t*)kmalloc(fat_size_bytes);
    for (uint32_t i = 0; i < bpb.sectors_per_fat; i++) {
        ata_read_sector(fat_start_sector + i, (uint8_t*)fat_buffer + (i * bpb.bytes_per_sector));
    }

    // --- 4. 循环读取和打印文件内容 ---
    kprint("\n");
    uint8_t* read_buffer = (uint8_t*)kmalloc(bpb.bytes_per_sector);
    uint32_t bytes_remaining = file_size;

    while (1) {
        // 读取当前簇的数据
        uint32_t lba = cluster_to_lba(current_cluster);
        ata_read_sector(lba, read_buffer);

        // 打印数据
        uint32_t bytes_to_print = (bytes_remaining > bpb.bytes_per_sector) ? bpb.bytes_per_sector : bytes_remaining;
        for (uint32_t i = 0; i < bytes_to_print; i++) {
            kputc(read_buffer[i]);
        }
        bytes_remaining -= bytes_to_print;

        // 查找下一个簇
        current_cluster = fat_buffer[current_cluster];
        
        // 如果到达文件末尾 (簇链结束) 或数据已读完，则退出循环
        if (current_cluster >= 0xFFF8 || bytes_remaining == 0) {
            break;
        }
    }

    // --- 5. 清理 ---
    kfree(root_dir_buffer);
    kfree(fat_buffer);
    kfree(read_buffer);
}

void fat16_append(const char* filename, const char* content) {
    // --- 1. 找到文件的目录条目 ---
    char fat_filename[11];
    to_fat16_filename(filename, fat_filename);

    uint32_t root_dir_start_sector = bpb.reserved_sector_count + (bpb.fat_count * bpb.sectors_per_fat);
    uint32_t root_dir_sectors = (bpb.root_entry_count * 32) / bpb.bytes_per_sector;
    fat16_directory_entry_t* root_dir_buffer = (fat16_directory_entry_t*)kmalloc(root_dir_sectors * bpb.bytes_per_sector);
    for (uint32_t i = 0; i < root_dir_sectors; i++) {
        ata_read_sector(root_dir_start_sector + i, (uint8_t*)root_dir_buffer + (i * bpb.bytes_per_sector));
    }

    int entry_index = -1;
    for (uint32_t i = 0; i < bpb.root_entry_count; i++) {
        if (memcmp(root_dir_buffer[i].filename, fat_filename, 11) == 0) {
            entry_index = i;
            break;
        }
    }

    if (entry_index == -1) {
        kprint("\nError: File not found.");
        kfree(root_dir_buffer);
        return;
    }

    fat16_directory_entry_t* entry = &root_dir_buffer[entry_index];
    uint16_t current_cluster = entry->first_cluster_low;
    uint32_t file_size = entry->file_size;

    // 如果文件为空，append 等同于 write
    if (current_cluster == 0) {
        kfree(root_dir_buffer);
        fat16_write_content(filename, content);
        return;
    }

    // --- 2. 找到文件的最后一个簇 ---
    uint32_t fat_size_bytes = bpb.sectors_per_fat * bpb.bytes_per_sector;
    uint16_t* fat_buffer = (uint16_t*)kmalloc(fat_size_bytes);
    uint32_t fat_start_sector = bpb.reserved_sector_count;
    for (uint32_t i = 0; i < bpb.sectors_per_fat; i++) {
        ata_read_sector(fat_start_sector + i, (uint8_t*)fat_buffer + (i * bpb.bytes_per_sector));
    }

    while (fat_buffer[current_cluster] < 0xFFF8) {
        current_cluster = fat_buffer[current_cluster];
    }
    kfree(fat_buffer); // FAT 已经用完，可以释放

    // --- 3. 读取最后一个簇，准备追加 ---
    uint8_t* cluster_buffer = (uint8_t*)kmalloc(bpb.bytes_per_sector);
    ata_read_sector(cluster_to_lba(current_cluster), cluster_buffer);

    uint32_t offset_in_cluster = file_size % bpb.bytes_per_sector;
    uint32_t space_in_cluster = bpb.bytes_per_sector - offset_in_cluster;
    uint32_t content_len = strlen(content);
    uint8_t* content_ptr = (uint8_t*)content;
    uint32_t bytes_to_write_now = (content_len < space_in_cluster) ? content_len : space_in_cluster;

    // a. 先填满最后一个簇的剩余空间
    memcpy(cluster_buffer + offset_in_cluster, content_ptr, bytes_to_write_now);
    ata_write_sector(cluster_to_lba(current_cluster), cluster_buffer);
    
    content_ptr += bytes_to_write_now;
    uint32_t bytes_remaining = content_len - bytes_to_write_now;

    // --- 4. 如果还有剩余内容，则启动多簇写入循环 ---
    while (bytes_remaining > 0) {
        uint16_t next_cluster = fat16_find_free_cluster();
        if (next_cluster == 0) {
            kprint("\nError: Disk is full.");
            break;
        }

        // 更新FAT表，将新簇链接到链上
        fat16_update_fat(current_cluster, next_cluster);
        current_cluster = next_cluster;

        // 写入剩余内容
        memset(cluster_buffer, 0, bpb.bytes_per_sector);
        bytes_to_write_now = (bytes_remaining > 512) ? 512 : bytes_remaining;
        memcpy(cluster_buffer, content_ptr, bytes_to_write_now);
        ata_write_sector(cluster_to_lba(current_cluster), cluster_buffer);

        content_ptr += bytes_to_write_now;
        bytes_remaining -= bytes_to_write_now;
    }

    // --- 5. 用文件结尾标记终止簇链 ---
    fat16_update_fat(current_cluster, 0xFFFF);
    kfree(cluster_buffer);

    // --- 6. 更新目录条目中的文件大小 ---
    entry->file_size += content_len;
    uint32_t dir_sector_offset = (entry_index * 32) / bpb.bytes_per_sector;
    ata_write_sector(root_dir_start_sector + dir_sector_offset, (uint8_t*)root_dir_buffer + dir_sector_offset * bpb.bytes_per_sector);
    kfree(root_dir_buffer);
}