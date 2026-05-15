# Kernel Design Guide

本文档记录 Tianole 当前阶段的顶层内核设计约束。它不是最终架构说明，也不是 Linux 的缩小版；它的作用是让后续实现不再只围绕单个文件局部修补，而是先确认子系统边界、调用方向和生命周期规则。

当代码改动影响目录职责、公共结构体、跨子系统 API、锁语义、生命周期或启动阶段顺序时，应该同步更新本文档或对应任务文档。

## 设计原则

- 参考 Linux 的分层和长期演进方式，但不照搬 Linux 的历史复杂度。
- 入口代码只编排流程，不承载具体策略。
- 子系统通过明确 API 交互，不直接访问对方内部结构。
- 公共结构体越少越好；公共头文件暴露的是契约，不是临时实现细节。
- 当前可以是单 CPU、单地址空间、简单调度，但接口不能把这些简化写成永久事实。
- 能力先闭环，再收紧边界；收紧边界时要补文档和自测。

## 顶层目录职责

- `arch/`：架构相关代码，包括启动入口、GDT/TSS/IDT、trap/IRQ entry、上下文切换、架构 timer、页表格式、TLB 和 CPU 指令封装。
- `kernel/`：架构无关的核心内核逻辑，包括 early log 前端、panic、IRQ 分发、调度、锁、时间抽象、进程和 syscall 的长期位置。
- `mm/`：架构无关内存管理，包括物理页、heap、后续 page metadata、buddy/slab、VMA、page cache 和回收。
- `drivers/`：设备驱动和设备模型，包括 input、block、PCI、ACPI、platform、GPU、network、sound 等。
- `block/`：通用块层。具体硬件驱动在 `drivers/`，块请求调度和块设备抽象在 `block/`。
- `fs/`：VFS 和具体文件系统。VFS 不依赖 shell、ELF loader 或具体块设备。
- `include/tianole/`：架构无关公共内核 API。只有跨子系统调用方真正需要的类型和函数才能放这里。
- `arch/<arch>/include/`：架构公开 API，例如中断控制、I/O port、上下文切换声明。
- `kernel/selftest/`：启动阶段自测和当前阶段演示线程，不放进具体实现目录。
- `scripts/`：构建、检查、QEMU 启动和结构规则工具。
- `docs/`：面向人的设计、路线和约束文档。

## 调用方向

允许的主要调用方向：

- `kernel/` 可以调用 `arch_` 前缀的架构 API。
- `kernel/` 可以调用 `mm/` 提供的分配和映射 API。
- `drivers/` 可以调用 IRQ、timer、wait queue、workqueue、MM 和 bus/resource API。
- `fs/` 可以调用 `mm/`、`block/` 和锁/等待 API。
- `user/` 或 `usr/` 里的用户程序只能通过 syscall/libc/VFS 暴露的接口进入 kernel。

禁止或需要重新设计的调用方向：

- 通用层不能直接读取 x86 页表编码、PIC/PIT 端口或 IDT gate 格式。
- 驱动不能直接调用 shell、VFS 内部对象或调度器内部链表。
- shell 和用户态工具不能包含 kernel 私有头或读取 kernel 内部结构体。
- 文件系统不能直接控制具体磁盘硬件。
- keyboard/input driver 不能直接操作 terminal 或 shell。
- boot/selftest 代码不能混进核心实现文件。

## 公共头文件规则

`include/tianole/` 只能放公共契约：

- 跨子系统需要调用的函数声明。
- 调用方必须知道的 opaque handle 或稳定结构。
- 明确属于公共 ABI 的枚举、flag 和错误码。

避免放入公共头文件：

- 只被一个子系统内部使用的字段。
- 具体链表节点、缓存节点、临时统计字段。
- 架构私有 bit 编码和寄存器布局。
- 调试或 selftest 专用结构。

如果公共结构体开始泄漏太多内部字段，优先考虑：

- 把结构改成 opaque。
- 拆出私有头，例如 `kernel/sched/sched.h`。
- 提供 accessor 或 helper，而不是让调用方直接改字段。

当前允许的阶段性妥协：

- `struct thread` 和 `struct wait_queue` 仍在公共头中暴露，方便早期调度和自测推进。
- 后续收紧调度状态机时，应逐步减少外部直接写 `thread->state`、`wait_next`、`next` 等字段。

## 启动阶段边界

启动路径按阶段推进：

1. bootloader 收集 boot info、memory map、kernel image。
2. kernel entry 建立最小运行环境。
3. early log 和 panic 可用。
4. trap/IRQ 基础可用。
5. MM 基础可用：物理页、页表、heap。
6. timer、thread、scheduler、wait queue 可用。
7. input、VFS、user mode、drivers 逐步接入。

启动阶段约束：

- early log/panic 不能依赖 heap、scheduler 或 VFS。
- trap/IRQ 初始化不能依赖复杂驱动模型。
- MM 初始化前不能使用 `kmalloc()`。
- scheduler 初始化前不能 sleep 或 wait。
- 驱动初始化不能假设 shell、VFS 或用户态已经存在。

## 日志、console 与终端

日志路径分层参考 Linux `printk`、console 和 tty 的长期边界：

- `early_log` 只作为最早期启动和 panic/emergency 兜底输出后端。
- `kernel/printk/` 放通用日志前端、日志缓冲和 console 注册逻辑，对齐 Linux `kernel/printk/`。
- 普通内核代码使用 `printk()`、`pr_info()`、`pr_warn()`、`pr_err()` 等通用日志前端。
- `printk` 前端负责格式化并写入内核日志缓冲，再同步写入已注册 console。
- console 是日志输出设备，例如 debug port、串口和 framebuffer boot console；它不等同于 tty。
- tty/terminal 是交互字符设备，负责行规程、回显、控制字符和未来用户态 shell 的标准输入输出。
- kdb 是 early debug consumer，不是正式 shell，也不应该作为普通内核日志 API 的使用理由。

约束：

- 新增普通初始化、自测和驱动诊断日志时使用 `pr_*`，不要继续直接调用 `early_log_*`。
- trap、oops、panic 等 fatal 诊断可以使用 `pr_emerg()` / `pr_err()`，但必须保留不依赖 heap、scheduler、VFS 或 tty 的兜底输出。
- 交互输出不要长期走 `printk`；`input_console` 和 kdb 后续应迁到 terminal/tty 或 debug console 边界。
- `panic()` 的声明属于 `<tianole/panic.h>`；不要为了 panic 引入 `<tianole/early_log.h>`。

## 中断、锁和 deferred work

中断路径分层：

- arch entry 保存现场并进入 C trap/IRQ 分发。
- IRQ core 根据 IRQ number 调用注册 handler。
- 具体设备 handler 做最短路径工作。
- 复杂处理应推迟到 deferred work、workqueue 或线程上下文。

锁规则：

- 中断和普通内核路径共享的数据，先使用 `spin_lock_irqsave()` 建立单 CPU 正确语义。
- 持有不可睡眠锁时不能调用 `sched_yield()`、`sched_sleep()` 或 wait queue sleep。
- 可睡眠路径和不可睡眠路径要从 API 命名、文档或调用约束上区分。
- 后续 SMP 前，需要把当前单 CPU lock 语义整理成可扩展接口。

等待规则：

- wait queue 负责“入队、睡眠、唤醒”的同步边界。
- 条件等待遵循“检查条件、入队睡眠、醒后重新检查”的模式。
- 唤醒方先修改受保护条件，再调用 wakeup。
- 条件所属数据如果不在 wait queue 内部，必须明确由哪把锁保护。

## 调度与线程生命周期

调度子系统职责：

- run queue 维护。
- 当前线程选择和上下文切换。
- thread state 转换。
- sleep/wakeup 和 wait queue 交互。
- DEAD/ZOMBIE 线程的安全回收。

调度状态机应逐步收紧：

- `READY -> RUNNING -> READY`
- `RUNNING -> SLEEPING/WAITING`
- `SLEEPING/WAITING -> READY`
- `RUNNING -> DEAD`
- 未来加入 `ZOMBIE` 支撑 join/wait 和进程退出。

约束：

- 外部子系统不应直接修改 run queue。
- 外部子系统不应长期直接写 `thread->state`。
- 当前线程不能释放自身内核栈。
- 线程入口返回必须进入统一 exit 路径。
- IRQ 中不直接做完整线程切换；调度请求应在明确的 interrupt-exit 边界消费。

## 内存、VFS 和缓存边界

MM 层负责：

- boot memory map 转换成内核长期内存区域模型。
- 物理页分配和 page metadata。
- 内核 heap 和后续 slab/slub。
- 用户地址空间、VMA、page fault 策略。
- page cache 和内存回收。

VFS 层负责：

- superblock、inode、dentry、file 等对象边界。
- path lookup。
- open/read/write/close/readdir 等通用文件操作。
- mount、权限、时间戳和错误码语义的长期位置。

block 层负责：

- block device 抽象。
- block request 和完成状态。
- 具体硬件驱动与 VFS 之间的隔离。

约束：

- 具体文件系统不直接访问磁盘硬件。
- 具体块设备驱动不解析文件系统。
- page cache 不属于某个具体文件系统或驱动。
- ELF loader 通过 VFS 读文件，不直接读取块设备或 initramfs 私有结构。

## 设备模型与驱动边界

长期设备模型包含：

- device
- driver
- bus
- resource
- probe/remove
- IRQ/DMA/MMIO/PIO resource 管理

驱动约束：

- probe 失败必须释放已申请资源。
- 驱动不修改无关子系统内部状态。
- 驱动不能长期 busy wait；应使用 IRQ、wait queue 或 deferred work。
- QEMU 专用设备支持不能破坏真机路径。
- ACPI/PCI/Device Tree 等平台信息隔离在平台层，通用驱动消费抽象资源。

输入路径：

- keyboard driver 产生 input event。
- input core 提供队列和等待。
- terminal/tty 消费 input event 并生成字节流或行。
- shell 读取 terminal，不读取 scancode。

存储路径：

- storage driver 接入 block layer。
- block layer 服务 filesystem。
- VFS 服务 kernel loader、用户态程序和 shell。

## 用户态与 syscall 边界

用户态相关对象要分开：

- process
- thread
- address space
- open file table
- credentials
- signal state

syscall 规则：

- syscall number、参数寄存器、返回值和错误码必须稳定记录。
- 用户指针必须复制和检查，不能直接解引用。
- syscall handler 不暴露 kernel 内部结构体。
- 用户态异常终止进程或转换成后续 signal，不能默认 panic。
- `fork/exec/wait`、signal、pipe、poll/select、futex 等长期路径要在 ABI 上预留。

用户程序规则：

- shell 和工具通过 libc 或 syscall ABI 访问 kernel。
- 用户程序不包含 kernel 私有头。
- 调试信息通过 syscall、procfs/debugfs 或等价虚拟文件接口暴露。

## 测试和回归边界

测试分层：

- `kernel/selftest/`：启动阶段内核自测，验证当前内核能力。
- `scripts/checks/`：主机侧检查，例如 build、boot、trap、page fault、structure。
- `scripts/tools/`：项目结构和自定义规则工具。

规则：

- 具体实现目录不放 selftest 主体。
- 新增公共 API 要有自测或 boot check 观察点。
- 调整目录边界要更新 `docs/agents/linux-layout.md`、`code-style.md` 或本文档。
- `scripts/check.sh` 是本地和 CI 的完整回归入口。
- 只改文档至少运行 `git diff --check`。

## 设计变更流程

遇到以下情况，先更新设计或任务文档，再改代码：

- 新增顶层目录或改变目录职责。
- 公共头文件新增跨子系统结构体。
- 修改 thread、wait queue、page、inode、file、process 等核心对象生命周期。
- 新增锁规则、等待规则或 IRQ 调用约束。
- 引入新硬件路径或新驱动模型。
- 新增 syscall ABI 或用户态可见行为。

遇到以下情况，可以直接小步改代码，但完成后要同步文档：

- 当前任务文档已经明确要求的局部实现。
- 不改变 API 的内部重构。
- selftest 或 check 脚本补强。
- 修复明显 bug，且不改变子系统边界。

## 当前主线

当前主线仍是 `docs/agents/tasks/04-time-scheduler.md`：

- wait queue 内部锁已经建立。
- 下一步应收紧 thread state helper，减少外部直接写状态。
- 然后继续完善 thread lifecycle 和 interrupt-exit reschedule。
- 在这些边界稳定前，不进入 keyboard/input 代码实现。
