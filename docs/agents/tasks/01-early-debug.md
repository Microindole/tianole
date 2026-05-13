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

### 1. Early console backend

- 建立通用 early log 前端，后端可以是 QEMU debug port、COM1、framebuffer console 或后续平台 console。
- x86 I/O port 访问集中在 arch 层，通用日志代码不能直接写端口。
- early 阶段不能依赖动态内存、锁、线程或 VFS。
- 后续切到正式 console 后，early backend 仍可作为 panic fallback。

### 2. Panic 与 halt 路径

- `panic()` 必须能在 allocator、scheduler、interrupt 都不可靠时输出最小信息。
- panic 路径要尽量避免递归 panic，至少保证第二次进入时直接 halt。
- arch 层提供 `arch_halt_forever()`，通用 panic 不直接写 `hlt` 循环细节。
- 后续 oops 和 panic 要区分：可恢复异常进入 oops，不可恢复错误进入 panic。

### 3. Log buffer 与正式 printk 预留

- early log 前端后续应能迁移到 ring buffer，而不是让所有代码直接写串口。
- 预留 log level、CPU id、timestamp、caller/source 信息。
- 预留多 consumer：serial、debug port、framebuffer console、内存 buffer。
- 预留 crash dump 或重启后读取日志的路径。

## Linux 参考原则

- 参考 Linux `printk`/console 分层：调用方写通用日志接口，console driver 负责具体输出设备。
- 参考 Linux early console 思路：早期输出简单可靠，正式 console 接管后仍保留诊断价值。
- 参考 Linux panic/oops 分离：panic 是不可恢复停机，oops 是异常诊断和策略入口。
- 参考 Linux ring buffer 思路：日志先进入内核缓冲，再由不同 console 消费，避免业务代码绑定输出硬件。

## 非玩具化约束

- 日志前端不能直接绑定 COM1。
- COM1 端口号要集中定义，不能散落在业务代码。
- panic 路径不能依赖动态内存。
- 输出接口要允许后续接 framebuffer console。
- 日志接口不能要求 scheduler、heap 或 interrupt 已经初始化。
- panic/oops 策略不能写进异常入口汇编。
- 正式 printk 出现后，early log 调用方不能被迫大规模改名或换接口。
- 任何新增 backend 都不能影响其他 backend 的可用性。

## 验收方式

- QEMU 日志中能看到 bootloader 和 kernel 输出。
- 串口或 debug log 中能看到 `boot services exited`。
- 人为触发 panic 时能看到 panic 信息并停止。
- 禁用某一个输出 backend 时，其他 backend 仍可输出关键日志。
- panic 路径在 heap 未初始化前也能输出。
- `scripts/check.sh` 至少验证 debug log 和 serial log 两条路径。

## 当前状态

基础完成：

- kernel early log 已拆成通用前端和 x86 backend。
- x86 backend 已同时写 QEMU debug port 和 COM1。
- x86 backend 已可在 UEFI GOP framebuffer 上显示 early log，QEMU 图形窗口可直接观察启动日志。
- `panic()` 已接入 early log 和 `arch_halt_forever()`。
- `scripts/check.sh` 已验证 `build/debug.log` 和 `build/serial.log` 中的关键启动行。

后续扩展：

- printk/log level/ring buffer。
- oops 格式。
- 异常栈输出。
- 内核符号化输出。
- framebuffer console。
- crash dump。
- early log 到 printk 的接管流程。
- panic 后保留最后日志的内存 buffer。
- 多 CPU 后的日志串行化规则。

验收依据：

- 正常启动日志能进入 QEMU debug log。
- kernel 日志能进入 COM1 serial log。
- `panic()` 已被 `02-cpu-interrupts.md` 的 invalid opcode 测试覆盖。
