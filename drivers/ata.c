#include "ata.h"

// ATA I/O 端口
#define ATA_DATA       0x1F0
#define ATA_ERROR      0x1F1
#define ATA_SECT_COUNT 0x1F2
#define ATA_LBA_LO     0x1F3
#define ATA_LBA_MID    0x1F4
#define ATA_LBA_HI     0x1F5
#define ATA_DRIVE_HEAD 0x1F6
#define ATA_STATUS     0x1F7
#define ATA_COMMAND    0x1F7

// ATA 命令
#define ATA_CMD_READ_PIO 0x20

// ATA 状态位
#define ATA_SR_BSY     0x80 // 忙碌
#define ATA_SR_DRQ     0x08 // 数据请求就绪

// 等待硬盘不忙碌
static void ata_wait_bsy() {
    while (inb(ATA_STATUS) & ATA_SR_BSY);
}

// 等待硬盘准备好数据
static void ata_wait_drq() {
    while (!(inb(ATA_STATUS) & ATA_SR_DRQ));
}

void ata_read_sector(uint32_t lba, uint8_t* buffer) {
    ata_wait_bsy();
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F)); // 选择主盘, LBA模式
    outb(ATA_SECT_COUNT, 1);
    outb(ATA_LBA_LO, lba & 0xFF);
    outb(ATA_LBA_MID, (lba >> 8) & 0xFF);
    outb(ATA_LBA_HI, (lba >> 16) & 0xFF);
    outb(ATA_COMMAND, ATA_CMD_READ_PIO);

    ata_wait_bsy();
    ata_wait_drq();

    // 读取 256 个 16位的值 (总共 512 字节)
    for (int i = 0; i < 256; i++) {
        uint16_t data = inw(ATA_DATA);
        buffer[i * 2] = data & 0xFF;
        buffer[i * 2 + 1] = (data >> 8) & 0xFF;
    }
}