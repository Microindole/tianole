# Tianole

新的起点。这个仓库现在不保留旧实现，先把目标机器、开发环境、技术路线和最小步骤固定下来，避免再次在错误方向上堆代码。

## 当前结论

- 语言：第一版用 `C`
- 架构：`x86_64`
- 启动方式：`UEFI`
- 主开发环境：`WSL2`
- 日常验证：先 `QEMU + OVMF`
- 真机验证：先 `U 盘 UEFI` 启动
- 最终目标：在自己的笔记本上以独立启动项或可控方式启动，不先破坏现有 `Windows 11 + Linux Mint + GRUB`

## 为什么现在不用旧代码

旧实现的问题不是“有几个 bug”，而是路线已经和目标真机错位：

- 旧项目是 `32位 + Multiboot + GRUB + qemu-system-i386`
- 你的目标机器是现代 `UEFI + GPT + x86_64`
- 旧项目已经铺到任务、FAT16、ATA PIO、用户态切换，但底座仍是老 PC 路线
- 继续修补会持续消耗精力在错误底座上

所以现在直接重开，比在旧代码上修修补补更省。

## 这台机器已经确认的信息

- 当前日期环境：`2026-05-08`
- 机器：`LENOVO 21HX`
- CPU：`13th Gen Intel Core i5-13500H`
- 核心/线程：`12` 核 `16` 线程
- 内存：`16 GB`
- 显卡：`Intel Iris Xe Graphics`
- 磁盘：`1 TB NVMe SSD`
- 固件模式：`UEFI`
- 分区表：`GPT`
- BIOS 版本：`LBCN26WW`
- BIOS 日期：`2024-08-23`
- 当前系统状态：你现在在 `Windows 11`，机器上同时有 `Linux Mint`
- 启动情况：GRUB 由 `Mint` 管理，你现在只是从该多启动环境进入了 Windows
- 额外观察：系统报告 `HypervisorPresent = True`
- 未确认项：`Secure Boot` 状态未成功读出，后续真机启动自制 OS 时必须单独检查

## 这些信息意味着什么

- 不要走 `BIOS + MBR + 16位 boot sector` 教程路线
- 不要再把 `32位` 当主线
- 第一阶段不要碰真实 GPU 驱动，先用 `UEFI GOP framebuffer`
- 第一阶段不要碰真实 `NVMe` 驱动
- 第一阶段不要急着做复杂用户态
- 真机阶段不要一开始去改现有 `GRUB` 或覆盖 EFI 启动链

## 为什么现在选 C，不选 Rust

你的条件是：

- 想长期做
- 但会断断续续
- 目标是最后装到自己的真机
- 设计思路参考 Linux

在这个组合下，`C + x86_64 + UEFI` 的阻力最低。

原因：

- 回坑成本低
- Linux 资料、历史实现、底层文档更贴近 C
- 你要适配的核心问题是 `UEFI / GPT / framebuffer / 内存管理 / 中断`，不是语言特性
- Rust 不是不能做，但更容易把精力花在 `no_std`、启动框架和抽象边界上

结论：

- 第一版内核主体：`C`
- 少量汇编：只保留最必要的入口和极少数底层切换
- 以后如果需要，再局部引入 Rust

## 为什么用 WSL2

主开发环境建议：

- `WSL2 + Ubuntu`

原因：

- `gcc/clang`
- `make`
- `nasm`
- `lld`
- `objdump/readelf`
- `qemu`
- `mtools`
- `ovmf`

这套工具在 Linux 环境里更顺，资料也基本默认 Linux。

Windows 仍然保留这些职责：

- 日常文件管理
- 处理下载
- 需要时管理 U 盘
- 重启进入 BIOS/启动菜单
- 真机测试前的辅助操作

长期工作流：

1. 在 `WSL2` 中写代码、编译、跑 `QEMU`
2. 需要更底层的镜像/介质实验时，可以切去 `Mint`
3. 真机验证优先走 `U 盘 UEFI` 启动
4. 系统稳定后，再决定是否集成进本机磁盘启动项

## 目标路线

第一阶段只做最小系统：

1. `UEFI` 启动成功
2. 获取内存映射
3. 初始化 `framebuffer`
4. 屏幕输出
5. 建立基础页表
6. 基础中断框架
7. 一个极简内核循环

这一阶段完成后，再考虑：

1. 物理内存分配器
2. 简单堆分配
3. 键盘输入
4. 简单 shell
5. ELF 加载
6. 用户态
7. 文件系统

## WSL2 中建议的基础环境

以 Ubuntu 为例：

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

可选检查：

```bash
gcc --version
clang --version
ld.lld --version
nasm -v
qemu-system-x86_64 --version
```

## 新仓库的起步步骤

切到 `WSL` 后，建议按这个顺序开始：

1. 在 `WSL` 中打开这个仓库
2. 保持仓库目标单纯，只做 `C + UEFI + x86_64`
3. 先搭最小目录结构
4. 先让一个最小 `UEFI` 程序在 `QEMU + OVMF` 下跑起来
5. 再把它拆成 `bootloader / kernel` 或直接决定最小加载方案
6. 先证明启动链可控，再扩内核

## 建议的最小目录结构

第一版建议从非常小的结构开始：

```text
tianole/
  README.md
  .gitignore
  Makefile
  boot/
  kernel/
  include/
  scripts/
  build/
```

先不要一开始就拆出：

- `fs/`
- `drivers/`
- `mm/`
- `userland/`
- `lib/`

等真正需要时再拆，不要先造目录架子。

## 真机测试原则

你这台机器是 `Windows 11 + Linux Mint` 双系统，并且已有 `GRUB` 启动环境，所以真机测试要克制：

1. 第一阶段只在 `QEMU + OVMF` 验证
2. 第二阶段只从 `U 盘` 启动
3. 第三阶段再考虑新增 UEFI 启动项
4. 不要一开始改当前磁盘上的 `GRUB` 配置
5. 不要一开始尝试覆盖现有 EFI 分区中的关键文件

## 当前最重要的技术边界

- 做现代 PC，不做老式 BIOS 教程项目
- 先做可启动、可显示、可调试
- 先不要做“看起来很完整”的模块铺设
- 先把启动链做对，再谈进程、文件系统、用户态
- 任何新代码都要服务于真机 `UEFI x86_64` 目标

## 下一步

切到 `WSL` 后，下一步不是继续讨论语言，而是直接开始搭最小骨架：

1. 建立 `Makefile`
2. 建立最小 `boot/` 与 `kernel/`
3. 跑通 `QEMU + OVMF`
4. 在屏幕上输出一行可见文本

做到这一步后，再继续扩展。
