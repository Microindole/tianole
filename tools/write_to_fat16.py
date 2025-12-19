#!/usr/bin/env python3
"""
将用户程序写入FAT16镜像的工具脚本
"""

import struct
import sys

def write_file_to_fat16(hdd_img_path, user_bin_path, filename):
    """
    将用户程序写入FAT16镜像
    """
    # 读取用户程序
    with open(user_bin_path, 'rb') as f:
        user_data = f.read()

    file_size = len(user_data)
    print(f"User program size: {file_size} bytes")

    # 打开硬盘镜像
    with open(hdd_img_path, 'r+b') as hdd:
        # 读取引导扇区
        hdd.seek(0)
        bpb = hdd.read(512)

        # 解析BPB
        bytes_per_sector = struct.unpack('<H', bpb[11:13])[0]
        sectors_per_cluster = bpb[13]
        reserved_sectors = struct.unpack('<H', bpb[14:16])[0]
        fat_count = bpb[16]
        root_entry_count = struct.unpack('<H', bpb[17:19])[0]
        sectors_per_fat = struct.unpack('<H', bpb[22:24])[0]

        print(f"Bytes per sector: {bytes_per_sector}")
        print(f"Sectors per cluster: {sectors_per_cluster}")
        print(f"Reserved sectors: {reserved_sectors}")
        print(f"FAT count: {fat_count}")
        print(f"Root entries: {root_entry_count}")
        print(f"Sectors per FAT: {sectors_per_fat}")

        # 计算位置
        fat1_start = reserved_sectors * bytes_per_sector
        fat2_start = fat1_start + sectors_per_fat * bytes_per_sector
        root_dir_start = fat2_start + sectors_per_fat * bytes_per_sector
        root_dir_sectors = (root_entry_count * 32) // bytes_per_sector
        data_area_start = root_dir_start + root_dir_sectors * bytes_per_sector

        # 读取FAT表
        hdd.seek(fat1_start)
        fat = bytearray(hdd.read(sectors_per_fat * bytes_per_sector))

        # 查找空闲簇
        free_cluster = None
        for i in range(2, len(fat) // 2):
            cluster_val = struct.unpack('<H', fat[i*2:i*2+2])[0]
            if cluster_val == 0:
                free_cluster = i
                break

        if free_cluster is None:
            print("Error: No free cluster found!")
            return False

        print(f"Found free cluster: {free_cluster}")

        # 计算需要的簇数
        cluster_size = sectors_per_cluster * bytes_per_sector
        clusters_needed = (file_size + cluster_size - 1) // cluster_size
        print(f"Clusters needed: {clusters_needed}")

        # 分配簇链
        current_cluster = free_cluster
        for i in range(clusters_needed):
            if i == clusters_needed - 1:
                # 最后一个簇
                struct.pack_into('<H', fat, current_cluster * 2, 0xFFFF)
            else:
                # 指向下一个簇
                next_cluster = free_cluster + i + 1
                struct.pack_into('<H', fat, current_cluster * 2, next_cluster)
                current_cluster = next_cluster

        # 写回FAT表（两份）
        hdd.seek(fat1_start)
        hdd.write(fat)
        hdd.seek(fat2_start)
        hdd.write(fat)

        # 在根目录创建条目
        hdd.seek(root_dir_start)
        root_dir = bytearray(hdd.read(root_dir_sectors * bytes_per_sector))

        # 查找空闲目录项
        entry_offset = None
        for i in range(root_entry_count):
            offset = i * 32
            if root_dir[offset] == 0x00 or root_dir[offset] == 0xE5:
                entry_offset = offset
                break

        if entry_offset is None:
            print("Error: No free directory entry!")
            return False

        # 创建目录项
        # 文件名（8.3格式）
        name_83 = filename.upper().ljust(11)
        for i, c in enumerate(name_83[:11]):
            root_dir[entry_offset + i] = ord(c)

        # 属性（普通文件）
        root_dir[entry_offset + 11] = 0x00

        # 起始簇号
        struct.pack_into('<H', root_dir, entry_offset + 26, free_cluster)

        # 文件大小
        struct.pack_into('<I', root_dir, entry_offset + 28, file_size)

        # 写回根目录
        hdd.seek(root_dir_start)
        hdd.write(root_dir)

        # 写入文件数据
        bytes_written = 0
        current_cluster = free_cluster

        for i in range(clusters_needed):
            cluster_lba = data_area_start + (current_cluster - 2) * cluster_size
            chunk_size = min(cluster_size, file_size - bytes_written)
            chunk = user_data[bytes_written:bytes_written + chunk_size]

            # 填充到簇大小
            if len(chunk) < cluster_size:
                chunk += b'\x00' * (cluster_size - len(chunk))

            hdd.seek(cluster_lba)
            hdd.write(chunk)

            bytes_written += chunk_size

            if i < clusters_needed - 1:
                current_cluster += 1

        print(f"Successfully wrote {bytes_written} bytes to {filename}")
        return True

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python3 write_to_fat16.py <hdd.img> <user_program.bin> [filename]")
        sys.exit(1)

    hdd_img = sys.argv[1]
    user_bin = sys.argv[2]
    filename = sys.argv[3] if len(sys.argv) > 3 else "HELLO   BIN"

    success = write_file_to_fat16(hdd_img, user_bin, filename)
    sys.exit(0 if success else 1)
