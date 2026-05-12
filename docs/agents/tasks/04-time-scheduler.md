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

### 1. Timer 与时钟基础

- 接入一个可替换的硬件 timer 后端，当前可以先用 PIT periodic timer。
- 通用层只暴露 `timer_tick()`、`timer_ticks()` 和后续 timeout/timer callback 所需接口。
- 调度器不能直接读取 PIT/APIC 硬件细节，只能依赖通用 time/timer 层。
- tick handler 中只做短路径工作：更新时间、唤醒到期 sleep、标记 reschedule，不在 IRQ handler 内做复杂策略。
- 后续预留 one-shot timer、高精度 timer 和 timer wheel/min-heap 等实现空间。

### 2. Kernel thread 与上下文切换

- 建立 kernel thread 对象，包含 id、状态、入口、参数、内核栈、调度链表节点和等待链表节点。
- 建立新线程 trampoline，让线程从独立内核栈进入入口函数。
- 建立 arch 私有上下文切换入口，只保存/恢复该 ABI 需要的调度上下文。
- 线程入口返回不能落入未知地址，必须统一进入 thread exit 路径。
- 线程对象和内核栈不能依赖“永不释放”，必须有明确生命周期和回收规则。

### 3. Run queue 与调度核心

- 建立动态 run queue，线程数量不能固定写死。
- 先实现单 CPU round-robin，再为优先级、时间片、per-cpu run queue 留边界。
- 调度核心需要集中处理 `RUNNING`、`READY`、`SLEEPING`、`WAITING`、`DEAD` 等状态转换。
- 调度选择、入队、出队和当前线程切换必须有统一锁规则。
- idle thread 必须作为没有 runnable thread 时的兜底执行体。

### 4. Interrupt-exit reschedule

- timer IRQ 只设置 `need_resched`，真正切换应收敛在明确的 IRQ 返回边界。
- `sched_irq_exit()` 要逐步演进为 trap-frame aware 模型，明确从中断返回时哪些现场可以被切换。
- 不要长期把普通协作式 `sched_yield()` 栈切换入口当成完整抢占式切换模型。
- 后续进入用户态后，用户态返回边界、内核可抢占点和 syscall 返回边界要能共享这套设计。
- 任意 IRQ handler 都不应该直接调用会睡眠的接口。

### 5. Sleep、wait queue 与 lost wakeup

- `sched_sleep()` 负责按 tick 睡眠并由 timer 唤醒。
- wait queue 要提供基础 sleep/wakeup、条件等待和带 timeout 的条件等待。
- 条件检查、入队睡眠和 wakeup 必须有明确锁规则，避免 lost wakeup。
- wakeup 必须发生在修改条件之后；等待方必须在同一同步规则下检查条件并决定是否睡眠。
- wait queue API 要区分可睡眠路径和不可睡眠路径，不能允许 IRQ handler 进入睡眠。
- timeout wait 被 timer 唤醒后，必须能从 wait queue 清理残留节点。

### 6. Locking 与不可睡眠上下文

- 建立单 CPU interrupt-safe lock：保存中断状态、关闭中断、释放时恢复原状态。
- 明确哪些锁可以在 IRQ 入口使用，哪些路径允许睡眠，哪些路径必须原子完成。
- run queue、wait queue、timer sleep list、thread id 分配等共享状态必须逐步纳入锁保护。
- 禁止持有不可睡眠锁时调用 `sched_yield()`、`sched_sleep()` 或 wait queue sleep。
- 后续 SMP 前，需要先把接口命名和调用约束整理清楚，避免把单 CPU 假设泄漏到所有调用方。

### 7. Thread lifecycle 与回收

- 线程入口返回后进入统一退出路径，而不是直接释放自身栈。
- 区分当前线程不能立即释放自身内核栈的问题，调度器只能在安全边界回收非当前 DEAD 线程。
- 为未来 `join/wait`、进程退出、引用计数和资源释放预留 `ZOMBIE` 或等价状态。
- 线程释放必须覆盖线程对象、内核栈、等待队列残留、run queue 残留和调试信息。
- 回收路径需要自测，避免重复释放、释放当前线程栈或释放仍在队列中的线程。

### 8. Deferred work、completion 与后续驱动基础

- 为 workqueue/deferred work 预留接口位置，让 IRQ handler 可以把复杂工作推迟到线程上下文。
- 为 completion 或一次性事件等待预留基础，后续驱动初始化和 I/O 完成会使用。
- 为 timer callback 或 delayed work 预留设计，避免所有超时逻辑都手写在调度器里。
- 这些能力可以晚于基础 wait queue 实现，但任务文档必须提前记录边界，避免后续驱动直接依赖临时接口。

## Linux 参考原则

本阶段不照搬 Linux CFS、SMP、RCU、hrtimer 等完整复杂度，但接口边界应参考 Linux 的长期分层方式：

- `kernel/sched/` 风格：调度策略、线程状态和 run queue 维护集中在调度子系统，不散落到驱动或 `kernel/main.c`。
- `wait_event*` 风格：等待方围绕“条件检查 + 入队睡眠 + 被唤醒后重新检查条件”组织逻辑，条件本身由调用方拥有。
- `wake_up*` 风格：唤醒方先改变受保护条件，再唤醒等待队列；否则容易出现 lost wakeup。
- `spin_lock_irqsave()` 风格：在中断和普通内核路径共享的数据结构上，先用保存/关闭中断的单 CPU lock 建立正确语义。
- `kthread` 风格：线程函数返回、显式退出、停止请求和资源回收要有统一生命周期，不依赖“这个线程永远不结束”。
- `timer/workqueue` 风格：IRQ handler 做最少工作，复杂处理推迟到线程上下文或 deferred work。

## 非玩具化约束

- 调度器不能直接绑定某一种硬件 timer。
- 线程数量不能固定写死。
- 等待队列要能被驱动、文件系统和进程等待复用。
- 日志只能辅助观察，不能成为调度逻辑的一部分。
- 内核对象不能依赖“永不释放”的假设。
- 任何可睡眠路径和不可睡眠路径要从接口上区分。
- 任何会修改 run queue、wait queue、sleep list 或 thread state 的路径都要说明锁规则。
- wait queue 条件等待必须能解释为什么不会丢唤醒。
- IRQ handler 不能直接执行可能睡眠或长时间运行的工作。
- 线程状态转换必须单向、可审计，不能通过随意改 `state` 修补行为。
- 当前阶段可以只支持单 CPU，但接口不能把“永远单 CPU”写死成公共契约。
- arch 层只负责 trap、IRQ、timer 硬件和上下文切换细节，不承载通用调度策略。

## 验收方式

- 两个以上 kernel thread 能轮转。
- 线程可以 sleep 并被 timer 唤醒。
- 调度现场保存和恢复稳定。
- 基础对象生命周期规则有文档和调用约束。
- wait queue 能完成普通 wakeup、条件等待和 timeout 等待。
- timeout 路径不会把线程永久留在 wait queue 中。
- timer IRQ 触发后，调度请求在明确的 IRQ exit 边界被消费。
- 线程入口正常返回后进入统一退出和回收路径。
- `scripts/check.sh` 覆盖 timer、thread、sleep、wait queue、timeout wait 和退出回收的可观测日志。
- 新增调度 API 必须同时更新公开头文件、自测和任务文档。

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
- wait queue 已有内部 interrupt-safe lock，条件检查、等待入队和 wakeup 队列修改已收敛到同一同步边界，降低 lost wakeup 风险。
- 已建立单 CPU interrupt-safe lock 基础，当前 `spin_lock_irqsave()` 会保存并关闭中断，`spin_unlock_irqrestore()` 会恢复原中断状态。
- 已把 `kernel_thread_create()` 中的线程 id 分配和 run queue 入队纳入 interrupt-safe lock 保护。
- 已建立 `sched_irq_exit()`，timer IRQ 只设置 `need_resched`，trap 的 IRQ 返回边界统一消费调度请求。
- 已建立最小 DEAD 线程回收路径，调度前会释放非当前 DEAD 线程的内核栈和线程对象。
- 已把调度代码按职责拆分为 `core.c`、`thread.c`、`wait.c`、`idle.c` 和私有 `sched.h`，并把当前阶段自测/演示线程移到 `kernel/selftest/sched.c`。
- `scripts/check.sh` 已验证 `timer initialized`、`timer tick=1/2/3`、`scheduler initialized`、`kernel thread selftest ok`、timer 驱动线程轮转、`sched_sleep()`、wait queue wakeup、条件等待和超时等待。

后续扩展：

### A. 调度状态机收紧

- 明确合法状态转换，例如 `READY -> RUNNING -> READY`、`RUNNING -> SLEEPING/WAITING`、`SLEEPING/WAITING -> READY`、`RUNNING -> DEAD`。
- 增加状态转换辅助函数，减少外部代码直接写 `thread->state`。
- 自测非法状态转换和重复入队问题。

### B. wait queue 锁语义

- 为 wait queue 增加内部锁或要求调用方持有指定锁，并在接口命名中体现约束。
- 继续把条件所属数据的修改规则文档化；当前 wait queue 内部锁已经覆盖条件检查、等待入队和 wakeup 队列修改。
- 区分 `wake_one`、`wake_all`、timeout wakeup 和条件 wakeup 的状态处理。
- 检查 wakeup 是否可能唤醒 DEAD、RUNNING 或未入队线程。

### C. interrupt-exit reschedule

- 把当前 `sched_irq_exit()` 继续收敛为更严格的 trap-frame aware interrupt-exit reschedule 模型。
- 明确 interrupt nested、idle、当前线程不可抢占等情况下是否允许调度。
- 避免把普通线程栈切换入口长期当成完整抢占式切换。
- 为未来 syscall return 和 user-mode return 复用同一 reschedule 边界。

### D. thread lifecycle

- 为线程退出增加更完整的生命周期状态、引用规则和最终释放约束。
- 补充“当前线程不能释放自身内核栈”的文档和自测。
- 为未来 `kthread_stop()`、join/wait 和进程退出保留接口空间。
- 回收路径需要覆盖等待队列残留、run queue 残留和 timer sleep 残留。

### E. deferred execution

- 增加最小 workqueue 或 deferred work，让 IRQ handler 可以只入队工作并唤醒 worker。
- 增加 completion 风格的一次性等待原语，供驱动初始化和异步完成使用。
- 增加 delayed work 或 timer callback 的设计草案，避免 timeout 逻辑复制到每个子系统。

下一阶段：

- 继续在 `04-time-scheduler.md` 内推进 wait queue 锁语义、线程生命周期和更严格的 interrupt-exit reschedule。
- 在这些边界稳定前，不急着进入 `05-input-events.md` 的键盘驱动实现。
