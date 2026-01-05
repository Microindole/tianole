# Tianole OS

一个使用 Rust 编写的 64 位操作系统，遵循类似 Linux 的设计哲学。

## 特性

- 🦀 **纯 Rust 实现**：利用 Rust 的内存安全和零成本抽象
- 🚀 **现代引导**：使用 Limine bootloader（高半核布局）
- 🖥️ **图形输出**：VGA framebuffer 支持
- 📝 **串口日志**：UART 16550 调试输出
- 🏗️ **宏内核架构**：高性能的单体设计

## 快速开始

### 环境要求

- Rust nightly 工具链
- QEMU（用于测试）
- xorriso（用于创建 ISO）
- make

### 构建和运行

```bash
# 编译内核
make kernel

# 构建 ISO 镜像
make all

# 在 QEMU 中运行
make run

# 清理构建文件
make clean
```

## 项目结构

```
tianole/
├── kernel/          # 内核代码
│   ├── src/
│   │   ├── arch/    # 架构相关代码（x86_64）
│   │   ├── drivers/ # 设备驱动
│   │   ├── mm/      # 内存管理
│   │   ├── task/    # 进程/任务管理
│   │   ├── fs/      # 文件系统
│   │   └── syscall/ # 系统调用
│   └── Cargo.toml
├── common/          # 共享库
├── linker.ld        # 链接器脚本（高半核布局）
├── limine.cfg       # Limine 引导配置
└── Makefile         # 构建脚本
```

## 架构设计

### 高半核内存布局

内核运行在高半核虚拟地址空间（`0xFFFFFFFF80200000`），提供：
- 用户空间和内核空间完全隔离
- 更好的安全性（ASLR）
- 符合 Linux/BSD 等现代 OS 设计

### 当前功能

- ✅ Limine 引导支持
- ✅ 高半核内核加载
- ✅ VGA 图形输出
- ✅ 串口驱动和日志系统
- ✅ 基础异常处理

### 计划功能

- [ ] 中断处理（IDT）
- [ ] 物理内存管理
- [ ] 堆分配器
- [ ] 键盘驱动
- [ ] 简单 Shell
- [ ] 虚拟内存管理
- [ ] 进程调度
- [ ] 文件系统

## 开发指南

### 调试

使用串口日志进行调试：

```rust
serial_println!("Debug: value = {}", some_value);
```

日志会输出到 QEMU 的终端窗口。

### 编译速度优化

Rust 的增量编译使得小修改的重新编译非常快速（通常 < 0.1 秒）：

```bash
cargo check -p kernel  # 快速语法检查
cargo build -p kernel  # 完整编译
```

## 许可证

[添加你的许可证信息]

## 致谢

- [Limine](https://github.com/limine-bootloader/limine) - 现代化的引导器
- [OSDev Wiki](https://wiki.osdev.org/) - 操作系统开发资源
