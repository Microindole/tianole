#ifndef ATA_H
#define ATA_H

#include "common.h"

// 从硬盘读取一个扇区
// lba: 逻辑块地址 (扇区号)
// buffer: 读取数据存放的缓冲区 (必须至少 512 字节)
void ata_read_sector(uint32_t lba, uint8_t* buffer);

#endif