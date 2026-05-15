# 15 SMP、多核与可伸缩性

## 目标

让 Tianole 从单核内核演进到多 CPU 可运行的内核，建立 per-cpu、IPI、锁层级和多核调度边界。

## 前置条件

- 单核调度、IRQ、内存管理、timer 和 wait queue 稳定。
- ACPI/MP table 或等价 CPU topology 来源可用。
- 锁和中断开关规则已文档化。

## 建议边界

- `kernel/smp/`：CPU bring-up、IPI、CPU mask、stop-the-world。
- `kernel/sched/`：多 CPU runqueue、负载均衡。
- `include/tianole/percpu.h`：per-cpu 变量和访问规则。
- `arch/x86/`：AP startup、local APIC、TSS/GDT per-cpu。
- `kernel/locking/`：spinlock、rwlock、atomic、lockdep 风格检查预留。

## 实现内容

- CPU discovery 和 AP startup。
- local APIC、IPI、per-cpu timer。
- per-cpu storage。
- 多 CPU scheduler runqueue。
- TLB shootdown。
- 锁层级、IRQ disable 规则和死锁诊断。
- RCU 或简化 read-copy-update 风格同步的规划。

### 1. CPU bring-up

- BSP/AP 初始化路径必须分离，但进入通用 kernel CPU online 流程。
- 每个 CPU 需要独立 stack、GDT/TSS、per-cpu area 和 idle thread。
- CPU online/offline 状态要有状态机，失败路径可诊断。

### 2. IPI and TLB shootdown

- IPI 是跨 CPU 调度、停止、TLB shootdown 的基础。
- 修改用户页表或内核映射时需要定义 TLB shootdown 规则。
- IPI handler 不能执行复杂可睡眠路径。

### 3. Scheduler scalability

- 早期可以每 CPU runqueue，后续加入负载均衡。
- thread migration 要处理 CPU affinity、锁和 wakeup 目标 CPU。
- timer tick 和 preemption 要能在多 CPU 下工作。

### 4. Locking and memory ordering

- spinlock、irqsave、atomic、barrier 规则必须统一。
- 哪些锁可在 IRQ 中使用、哪些锁可睡眠，要在文档和代码命名中体现。
- 后续 RCU、seqlock、rwlock 应接入 locking 子系统。

## Linux 参考原则

- 参考 Linux `kernel/smp.c`、`kernel/sched/`、`include/linux/percpu.h`、`arch/x86/kernel/smpboot.c`。
- per-cpu、IPI、TLB shootdown 是多核内核基础，不是性能优化细节。
- 锁规则必须先于大规模驱动/网络扩展建立，否则后续竞态会不可控。

## 非玩具化约束

- 不能只启动 AP 后让它空转而不纳入 CPU 模型。
- 全局大锁可以作为短期过渡，但必须记录替换边界。
- 多核下不能依赖“关中断等于全系统互斥”。
- 页表修改必须考虑其他 CPU 的 TLB。
- per-cpu 数据不能通过裸数组和当前 CPU 猜测散落实现。

## 验收方式

- QEMU `-smp 2` 或更多 CPU 可以启动。
- 每个 CPU 输出 online marker。
- 两个 CPU 都能运行 kernel thread 或 idle loop。
- IPI selftest 可以从一个 CPU 唤醒另一个 CPU。
- TLB shootdown selftest 不破坏页表一致性。

## 当前状态

未开始。

在 06-09 的单核主线稳定前不要求实现，但所有新核心子系统应避免永久绑定单 CPU 假设。

## 后续扩展

- CPU hotplug。
- NUMA。
- RCU。
- lockdep。
- scheduler class。
- high-resolution per-cpu timers。
