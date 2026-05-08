# Tianole 路线图

## 总原则

这个项目按“可中断、可回坑、每一阶段都能单独验收”的方式推进。

目标不是只在虚拟机里亮屏，而是：

- 在 `UEFI x86_64` 真机上独立启动
- 逐步具备完整内核基础设施
- 逐步具备运行本地 `git` 的能力

这里参考 Linux 的是分层思路和演进顺序，不是照搬早期 Linux 的启动方式或目录树。

## 推荐开发顺序

### 1. 启动链

- 跑通 `UEFI app -> bootloader -> kernel`
- 保证 `kernel_main()` 真正开始执行
- 保证最小可见输出

完成标志：

- `bootloader` 成功把控制权交给独立 `kernel`
- `kernel` 能输出第一行字并停住

### 2. 早期调试输出

- 保留当前 `debug` 输出路径
- 增加 early serial log
- 为后续脱离固件后的调试做准备

完成标志：

- 内核在更早阶段也能稳定输出日志

### 3. 内存与异常基础

- 获取 `UEFI memory map`
- 正确执行 `ExitBootServices`
- 建立 `GDT/IDT`
- 增加基础异常处理
- 建立物理页分配器
- 建立内核堆
- 增加定时器

完成标志：

- 退出 UEFI 后内核仍稳定运行
- 关键异常能进入处理路径
- 内核能稳定分配页和堆内存

### 4. 执行流与调度

- 建立内核线程
- 建立调度器
- 建立上下文切换
- 增加睡眠/唤醒与同步原语

完成标志：

- 可以同时运行多个内核线程
- 调度由时钟驱动

### 5. 用户态入口

- 建立用户地址空间
- 增加系统调用入口
- 增加 ELF 用户程序加载
- 增加进程创建和等待

完成标志：

- 能加载并运行最小用户程序
- 用户程序能调用系统调用输出

### 6. 文件系统与存储

- 建立块设备层
- 建立 `VFS`
- 接入一个实际文件系统
- 实现路径解析、文件读写、目录操作

完成标志：

- 能挂载文件系统
- 能打开、读取、写入文件

### 7. shell 与基础工具

- 增加 `init`
- 增加简单 shell
- 增加基础命令
- 补最小 `libc` 能力

完成标志：

- 启动后能进入 shell
- 能执行几个基础程序

### 8. 面向 git 的补齐

- 优先支持本地 `git`
- 不先碰网络协议栈
- 补足 `git` 依赖的文件、进程、时间、路径等接口
- 先以 `git init / status / add / commit` 为目标

完成标志：

- 能运行本地仓库的基础 `git` 操作

### 9. 真机落地

- 先 `QEMU + OVMF`
- 再 `U 盘 UEFI` 启动
- 最后再考虑加入本机启动项

完成标志：

- 不破坏现有本机启动链
- 能在真机上独立启动并进入系统

## 当前进度

当前已经完成：

- 最小 `UEFI` 启动链
- `bootloader + kernel` 拆分
- 独立 `kernel.elf` 加载
- `kernel_main()` 实际执行
- `memory map` 已通过 `boot_info` 传递，并能在 kernel 中统计描述符与可用页数
- `ExitBootServices` 已在进入 kernel 前执行
- x86 早期日志接口已集中到 `arch/x86/include/tianole/early_log.h`
- 构建布局已改为接近 Linux 的目录 Makefile 组织方式
- 本地检查脚本和 GitHub Actions 已接入

当前还没有完成：

- early serial log
- framebuffer console
- 真正的内存管理和异常子系统

## 当前代码组织原则

当前先不直接复制 Linux 的完整目录树，但开始采用接近 Linux 的分层方向：

- `arch/`：架构相关实现，目前只放 `x86`
- `kernel/`：未来放架构无关的内核主体
- `include/tianole/`：内核与 bootloader 共享的项目自有接口
- `docs/`：正式设计与路线图
- `docs/agents/`：agent 协作文档

当前这样拆分的目的不是过早抽象，而是先把边界立住：

- `UEFI` 相关定义留在 `arch/x86/`
- 通用交接结构放在 `include/tianole/`
- 以后新增架构时，优先新增新的 `arch/<name>/`

## 最近里程碑

### 里程碑 1：退出 UEFI 前的最后准备

- 获取并保存完整 `memory map`
- 扩展 `boot_info`
- 建立 early serial log

### 里程碑 2：退出 UEFI

- 正确执行 `ExitBootServices`
- `kernel` 在退出固件服务后继续稳定运行

### 里程碑 3：早期内核基础设施

- 增加 framebuffer console
- 建立物理页分配器
- 建立基础异常处理
