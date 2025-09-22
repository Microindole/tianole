#include "fat16.h"
#include "../drivers/ata.h"
#include "string.h" // for memset

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