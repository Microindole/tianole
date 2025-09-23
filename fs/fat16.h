#ifndef FAT16_H
#define FAT16_H

#include "common.h"

// FAT16 引导扇区 (Boot Sector) 结构体
// __attribute__((packed)) 确保编译器不会为了对齐而改变结构体大小
typedef struct {
    uint8_t jmp[3];
    char oem_name[8];
    uint16_t bytes_per_sector;      // 每个扇区字节数 (通常是 512)
    uint8_t sectors_per_cluster;    // 每个簇的扇区数
    uint16_t reserved_sector_count; // 保留扇区数 (引导扇区是其中之一)
    uint8_t fat_count;              // FAT表的数量 (通常是 2)
    uint16_t root_entry_count;      // 根目录区的条目容量
    uint16_t total_sectors_short;   // 总扇区数 (如果为0, 则使用下面的32位值)
    uint8_t media_type;
    uint16_t sectors_per_fat;       // 每个FAT表占用的扇区数
    uint16_t sectors_per_track;
    uint16_t head_count;
    uint32_t hidden_sector_count;
    uint32_t total_sectors_long;    // 32位的总扇区数

    // 扩展引导记录
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t boot_signature;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8]; // 文件系统类型, e.g., "FAT16   "

} __attribute__((packed)) fat16_boot_sector_t;

// FAT16 目录条目结构体 (32 字节)
typedef struct {
    char filename[8];
    char extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high; // FAT32 使用, FAT16 中为 0
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster_low;  // 文件/目录内容起始簇号
    uint32_t file_size;          // 文件大小 (字节)
} __attribute__((packed)) fat16_directory_entry_t;

typedef struct {
    fat16_directory_entry_t* entries;
    uint32_t entry_count;
} fat16_directory_t;

// 声明读取根目录的函数
fat16_directory_t* fat16_get_root_directory();

// 初始化 FAT16 文件系统 (我们下一步要实现的目标)
void init_fat16();
void fat16_format();

extern fat16_boot_sector_t bpb;

#endif