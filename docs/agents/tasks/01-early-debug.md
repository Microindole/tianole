# 01 早期调试输出

## 目标

建立 firmware 之外的稳定日志能力，作为后续 CPU、内存、调度和驱动开发的基础。

## 前置条件

- bootloader 已能进入 kernel。
- kernel 已在 `ExitBootServices` 后运行。
- 当前已有 QEMU debug port 输出。

## 建议边界

- `arch/x86/`：COM1、I/O port、架构相关 early log backend。
- `kernel/`：通用 early log 前端和 panic/oops 入口。
- `scripts/`：QEMU 串口参数和日志验证。

## 实现内容

- 初始化 COM1。
- 增加串口输出 backend。
- 保留 QEMU debug port backend。
- 统一 `early_log_puts()` 一类前端接口。
- 增加最小 `panic()`，panic 后停止 CPU。
- 为后续 printk、oops、内核符号和 crash dump 预留接口边界。

## 非玩具化约束

- 日志前端不能直接绑定 COM1。
- COM1 端口号要集中定义，不能散落在业务代码。
- panic 路径不能依赖动态内存。
- 输出接口要允许后续接 framebuffer console。

## 验收方式

- QEMU 日志中能看到 bootloader 和 kernel 输出。
- 串口或 debug log 中能看到 `boot services exited`。
- 人为触发 panic 时能看到 panic 信息并停止。
