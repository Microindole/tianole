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
