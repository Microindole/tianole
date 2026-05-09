# 04 时钟、内核线程与调度

## 目标

建立 kernel 内部并发执行能力，为驱动等待、异步 I/O、进程和用户态做准备。

## 前置条件

- 中断入口可用。
- 内存分配可用。
- 至少一个 timer 可接入。

## 建议边界

- `kernel/sched/`：线程、调度器、等待队列。
- `kernel/time/`：通用时间与 timer 抽象。
- `arch/x86/`：具体 timer、上下文切换。

## 实现内容

- 建立 kernel thread。
- 建立上下文切换。
- 建立 run queue。
- 建立 sleep/wakeup。
- 建立基础 spinlock 或 interrupt-safe lock。
- 接入 timer tick 或 one-shot timer。
- 为 workqueue、deferred work、completion、引用计数生命周期管理预留位置。

## 非玩具化约束

- 调度器不能直接绑定某一种硬件 timer。
- 线程数量不能固定写死。
- 等待队列要能被驱动、文件系统和进程等待复用。
- 日志只能辅助观察，不能成为调度逻辑的一部分。
- 内核对象不能依赖“永不释放”的假设。
- 任何可睡眠路径和不可睡眠路径要从接口上区分。

## 验收方式

- 两个以上 kernel thread 能轮转。
- 线程可以 sleep 并被 timer 唤醒。
- 调度现场保存和恢复稳定。
- 基础对象生命周期规则有文档和调用约束。

## 当前状态

基础完成：

- 已建立 x86 PIC remap，外部 IRQ 使用 vector 32-47，不再和 CPU exception 混用。
- 已接入 PIT periodic timer，当前频率为 100Hz。
- 已建立通用 `timer_tick()` 入口和 `timer_ticks()` 计数接口。
- 已在 trap dispatch 中区分 CPU exception 和外部 IRQ。
- 已对 timer IRQ0 发送 EOI，避免中断只触发一次。
- 已建立最小 kernel thread 对象，包含 id、状态、入口、参数、内核栈和 run queue 链接。
- 已建立动态 run queue，不固定写死线程数量。
- 已能通过 `kernel_thread_create()` 动态分配线程对象和内核栈。
- `scripts/check.sh` 已验证 `timer initialized`、`timer tick=1/2/3`、`scheduler initialized` 和 `kernel thread selftest ok`。

后续扩展：

- 把 IRQ 分发扩展为可注册 handler 的表，而不是只处理 timer。
- 建立上下文切换入口。
- 建立 run queue 和最小 round-robin scheduler。
- 建立 `yield()`、`sleep()`、timer wakeup 和 wait queue。

下一阶段：

- 继续在 `04-time-scheduler.md` 内推进上下文切换入口和最小 round-robin scheduler。
