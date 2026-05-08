# Tianole 路线图

## 第 0 阶段

- 建立最小 `UEFI` 启动程序。
- 在 `QEMU + OVMF` 下运行。
- 在屏幕和 `debug.log` 中输出可见的启动标记。
- 保证启动后常驻，便于后续调试。

## 第 1 阶段

- 把当前单体启动程序拆成 `boot/` 与 `kernel/`。
- 定义 `boot_info` 交接结构。
- 由 `bootloader` 加载独立 `kernel` 映像。
- 跳转到 `kernel` 入口并建立稳定栈。

## 第 2 阶段

- 获取 `UEFI memory map`。
- 正确执行 `ExitBootServices`。
- 建立最小 framebuffer 控制台。
- 开始做物理内存管理。

## 第 3 阶段

- 建立 `IDT` 和基础异常处理。
- 增加定时器来源。
- 开始早期 `ACPI` 解析。
- 逐步过渡到接近 Linux 风格的 `arch/`、`mm/`、`kernel/` 边界。

## 总体流程

这个项目要按“可中断、可回坑、每一阶段都能单独验收”的方式推进。最终目标不是只在虚拟机里亮屏，而是要在这台 `UEFI x86_64` 笔记本上独立启动，并逐步具备运行本地 `git` 的能力。

建议的长期阶段如下：

### 阶段 A：启动链

- 跑通 `UEFI app -> bootloader -> kernel`
- 保证 `kernel_main()` 真正开始执行
- 保证屏幕或串口可见输出

完成标志：

- `bootloader` 成功把控制权交给独立 `kernel`
- `kernel` 能输出第一行字并停住

### 阶段 B：早期内核基础

- 定义 `boot_info`
- 获取 `memory map`
- 正确执行 `ExitBootServices`
- 建立 early console
- 建立最小 panic / assert / log

完成标志：

- 退出 UEFI 后内核仍能稳定运行
- 崩溃时能给出可定位信息

### 阶段 C：内存与异常

- 建立物理页分配器
- 建立内核堆分配器
- 建立 `GDT/IDT`
- 处理基础异常
- 增加定时器

完成标志：

- 能稳定分配页和堆内存
- 关键异常能进入处理路径

### 阶段 D：任务模型

- 建立内核线程
- 建立调度器
- 建立上下文切换
- 增加睡眠/唤醒与同步原语

完成标志：

- 可以同时运行多个内核线程
- 调度由时钟驱动

### 阶段 E：用户态基础

- 建立用户地址空间
- 增加系统调用入口
- 增加 ELF 加载
- 增加进程创建和等待

完成标志：

- 能加载并运行最小用户程序
- 用户程序能调用系统调用输出

### 阶段 F：存储与文件系统

- 建立块设备层
- 建立 `VFS`
- 接入一个实际文件系统
- 实现路径解析、文件读写、目录操作

完成标志：

- 能挂载文件系统
- 能打开、读取、写入文件

### 阶段 G：最小用户空间

- 增加 `init`
- 增加简单 shell
- 增加基础命令
- 补最小 `libc` 能力

完成标志：

- 启动后能进入 shell
- 能执行几个基础程序

### 阶段 H：面向 git 的补齐

- 优先支持本地 `git`，不先碰网络
- 补足 `git` 依赖的文件、进程、时间、路径等接口
- 先以 `git init / status / add / commit` 为目标

完成标志：

- 能运行本地仓库的基础 `git` 操作

### 阶段 I：真机落地

- 先 `QEMU + OVMF`
- 再 `U 盘 UEFI` 启动
- 最后再考虑加入本机启动项

完成标志：

- 不破坏现有 `Windows 11 + Linux Mint + GRUB`
- 能在真机上独立启动并进入系统

## 当前所处阶段

当前只完成了第 0 阶段的最小成果：

- `UEFI` 启动程序可构建
- `QEMU + OVMF` 能执行它
- 屏幕和 `debug.log` 有可见输出
- 程序可常驻，便于调试

当前还没有完成：

- 独立 `kernel` 接管
- `bootloader -> kernel` 交接
- `memory map`
- `ExitBootServices`
- 真正的内核子系统

## 接下来最近三个里程碑

### 里程碑 1：独立 kernel 接管

- 让 `bootloader` 加载独立 `kernel`
- 建立 `boot_info`
- 跳到 `kernel_main()`
- 由 `kernel` 自己输出第一行字

### 里程碑 2：退出 UEFI

- 获取并保存 `memory map`
- 正确执行 `ExitBootServices`
- `kernel` 在退出固件服务后继续稳定运行

### 里程碑 3：早期调试和内存基础

- 增加 early serial / framebuffer console
- 建立物理页分配器
- 建立基础异常处理
