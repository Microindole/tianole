# 本机环境与真机启动约束

这个文档记录 Tianole 未来在本机落地时必须考虑的环境信息。它不属于 README 首页内容，但对后续真机启动、U 盘验证、启动项接入很重要。

## 机器信息

- 日期环境：`2026-05-08`
- 机器：`LENOVO 21HX`
- CPU：`13th Gen Intel Core i5-13500H`
- 核心 / 线程：`12` 核 `16` 线程
- 内存：`16 GB`
- 显卡：`Intel Iris Xe Graphics`
- 磁盘：`1 TB NVMe SSD`

## 固件与启动环境

- 固件模式：`UEFI`
- 分区表：`GPT`
- BIOS 版本：`LBCN26WW`
- BIOS 日期：`2024-08-23`
- 当前系统状态：本机有 `Windows 11` 与 `Linux Mint`
- 启动情况：当前多启动由 `Mint` 的 `GRUB` 管理
- 额外观察：系统报告 `HypervisorPresent = True`
- 未确认项：`Secure Boot` 状态仍需单独确认

## 这些信息意味着什么

- 不走 `BIOS + MBR + 16-bit boot sector` 路线
- 不把 `32-bit` 作为主线
- 第一阶段不碰真实 GPU 驱动，先依赖 `UEFI GOP framebuffer`
- 第一阶段不碰真实 `NVMe` 驱动
- 真机阶段不应一开始改写现有 `GRUB`
- 真机阶段不应一开始覆盖现有 EFI 分区中的关键文件

## 真机验证原则

1. 第一阶段只在 `QEMU + OVMF` 下验证
2. 第二阶段优先用 `U 盘 UEFI` 启动验证
3. 第三阶段再考虑新增本机 UEFI 启动项
4. 在系统稳定前，不改写现有系统磁盘启动链

## 开发环境

主开发环境建议：

- `WSL2 + Ubuntu`

推荐工具：

```bash
sudo apt update
sudo apt install -y \
  build-essential \
  clang \
  lld \
  nasm \
  make \
  cmake \
  qemu-system-x86 \
  ovmf \
  mtools \
  dosfstools \
  gdisk \
  xorriso \
  gdb \
  python3
```

Windows 侧主要承担：

- 日常文件管理
- 下载与资料整理
- U 盘管理
- BIOS / 启动菜单辅助操作
- 真机测试前准备
