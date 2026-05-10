# 04 时钟、内核线程与调度

## 目标

建立 kernel 内部并发执行能力，为驱动等待、异步 I/O、进程和用户态做准备。

## 前置条件

- 中断入口可用。
- 内存分配可用。
- 至少一个 timer 可接入。

## 建议边界

- `kernel/sched/core.c`：run queue、调度选择、tick 和 IRQ exit 调度边界。
- `kernel/sched/thread.c`：线程对象创建、初始栈、退出和 DEAD 线程回收。
- `kernel/sched/wait.c`：等待队列。
- `kernel/sched/idle.c`：idle thread。
- `kernel/sched/sched.h`：调度子系统私有接口，不向通用内核层公开内部状态。
- `kernel/selftest/sched.c`：当前阶段的调度自测和启动演示线程。
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
- 已建立 IRQ handler 注册表，timer IRQ0 通过 `irq_register()` 接入分发路径。
- 已对已处理 IRQ 发送 EOI，避免中断只触发一次。
- 已建立最小 kernel thread 对象，包含 id、状态、入口、参数、内核栈和 run queue 链接。
- 已建立动态 run queue，不固定写死线程数量。
- 已能通过 `kernel_thread_create()` 动态分配线程对象和内核栈。
- 已建立 x86 上下文切换入口，保存/恢复 callee-saved 寄存器和栈指针。
- 已建立线程 trampoline，新线程能从独立内核栈进入自己的入口函数。
- 已建立协作式 round-robin，两个 kernel thread 能通过 `sched_yield()` 轮转运行。
- 已把调度接入 `timer_tick()`，timer tick 会唤醒到期 sleep 线程并标记 `need_resched`。
- 已建立 idle thread，所有普通线程 sleep/wait 时由 idle 承接 CPU。
- 已提供 `sched_sleep()`，线程可以睡眠指定 tick 数并被 timer 唤醒。
- 已提供 `wait_queue_init()`、`wait_queue_sleep()`、`wait_queue_wake_one()` 和 `wait_queue_wake_all()`。
- 已提供 `wait_queue_wait()` 和 `wait_queue_wait_timeout()`，调用方可以等待条件成立，并在超时路径上获得失败返回值。
- 已建立单 CPU interrupt-safe lock 基础，当前 `spin_lock_irqsave()` 会保存并关闭中断，`spin_unlock_irqrestore()` 会恢复原中断状态。
- 已把 `kernel_thread_create()` 中的线程 id 分配和 run queue 入队纳入 interrupt-safe lock 保护。
- 已建立 `sched_irq_exit()`，timer IRQ 只设置 `need_resched`，trap 的 IRQ 返回边界统一消费调度请求。
- 已建立最小 DEAD 线程回收路径，调度前会释放非当前 DEAD 线程的内核栈和线程对象。
- 已把调度代码按职责拆分为 `core.c`、`thread.c`、`wait.c`、`idle.c` 和私有 `sched.h`，并把当前阶段自测/演示线程移到 `kernel/selftest/sched.c`。
- `scripts/check.sh` 已验证 `timer initialized`、`timer tick=1/2/3`、`scheduler initialized`、`kernel thread selftest ok`、timer 驱动线程轮转、`sched_sleep()`、wait queue wakeup、条件等待和超时等待。

后续扩展：

- 把当前 `sched_irq_exit()` 继续收敛为更严格的 trap-frame aware interrupt-exit reschedule 模型，避免把普通线程栈切换入口长期当成完整抢占式切换。
- 继续扩大 interrupt-safe lock 覆盖范围，明确可睡眠路径和不可睡眠路径的锁规则。
- 为 wait queue 增加更严格的状态检查，并把条件检查与 wakeup 边界纳入明确锁规则。
- 为线程退出增加更完整的生命周期状态、引用规则和最终释放约束。

下一阶段：

- 继续在 `04-time-scheduler.md` 内推进 interrupt-safe lock、线程退出回收和更严格的 interrupt-exit reschedule。
