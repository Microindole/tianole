# Agent 任务索引

这个目录保存适合 agent 分阶段执行的任务说明。每个任务文档都应该说明：

- 目标
- 前置条件
- 建议目录边界
- 非玩具化约束
- 验收方式

当前推荐顺序：

1. `01-early-debug.md`：串口、panic、早期日志。
2. `02-cpu-interrupts.md`：CPU 基础、GDT、IDT、异常、中断入口。
3. `03-memory.md`：物理页分配、虚拟内存、内核堆。
4. `04-time-scheduler.md`：时钟、内核线程、调度与等待。
5. `05-input-events.md`：键盘输入、事件队列、终端输入模型。
6. `06-storage-vfs.md`：块设备、页缓存、VFS、基础文件系统。
7. `07-user-mode.md`：用户态、系统调用、进程、ELF 程序加载。
8. `08-shell-tools.md`：init、shell、调试命令、最小工具集。
9. `09-driver-expansion.md`：PCI/ACPI/更完整设备驱动，音频等后期设备。
10. `10-real-machine.md`：真机启动、安全验证、硬件差异处理。

## 当前进度

| 阶段 | 状态 | 说明 |
| --- | --- | --- |
| `01-early-debug.md` | 基础完成 | 已有 early log 前端、QEMU debug port、COM1 串口和最小 `panic()`。 |
| `02-cpu-interrupts.md` | 基础完成 | 已有 GDT/TSS/IDT、exception vector 0-31、`trap_frame` 和 invalid opcode 回归测试。 |
| `03-memory.md` | 基础完成 | 已有最小物理页 allocator、页表 map/unmap/query、page fault 诊断和内核堆。 |
| `04-time-scheduler.md` | 进行中 | 已有 PIT timer interrupt 和通用 tick 计数；kernel thread、调度和等待队列未完成。 |
| `05-input-events.md` | 未开始 | 需要输入事件模型和键盘接入。 |
| `06-storage-vfs.md` | 未开始 | 需要块层、缓存、VFS 和基础文件系统。 |
| `07-user-mode.md` | 未开始 | 需要 syscall、进程、用户地址空间和 ELF 加载。 |
| `08-shell-tools.md` | 未开始 | 需要 init、shell、最小用户态工具和 libc 基础。 |
| `09-driver-expansion.md` | 未开始 | 需要设备模型、PCI/ACPI、存储、网络等驱动扩展。 |
| `10-real-machine.md` | 未开始 | 需要 U 盘真机启动验证和硬件差异记录。 |

当前最合适的下一步仍在 `04-time-scheduler.md` 内：引入最小 kernel thread 和内核栈，再做调度循环。

## Linux 级能力缺口路由

这些能力不一定马上实现，但必须在路线中占位，避免后续走偏。

- 硬件发现、平台层、总线、资源分配：`09-driver-expansion.md`
- ACPI、Device Tree、PCI、USB、IRQ routing：`09-driver-expansion.md`
- NUMA、CPU topology、电源管理、热插拔：`09-driver-expansion.md`
- 内核对象生命周期、引用计数、资源释放：`04-time-scheduler.md`、`06-storage-vfs.md`、`07-user-mode.md`
- 锁、等待队列、workqueue、timer、延迟执行：`04-time-scheduler.md`
- 完整虚拟内存、匿名页、缺页加载、COW、mmap、换页、内存回收：`03-memory.md`、`07-user-mode.md`
- page cache、block cache、writeback、文件一致性：`06-storage-vfs.md`
- slab/slub、小对象缓存：`03-memory.md`
- POSIX syscall、`fork/exec/wait`、`fcntl/stat/chmod`、`errno`、路径语义：`07-user-mode.md`、`08-shell-tools.md`
- uid/gid、文件权限、进程权限：`07-user-mode.md`、`08-shell-tools.md`
- signal、pipe、socket、eventfd、futex、共享内存、poll/select、IPC：`07-user-mode.md`
- tty、session、job control：`05-input-events.md`、`08-shell-tools.md`
- libc、动态链接、用户空间工具链：`08-shell-tools.md`
- printk、panic/oops、异常栈、内核符号：`01-early-debug.md`、`02-cpu-interrupts.md`
- ftrace、perf、procfs、sysfs、debugfs、crash dump：`08-shell-tools.md`、`09-driver-expansion.md`
- 用户/组、权限、capability、隔离模型：`07-user-mode.md`
- 网络栈和 socket API：`09-driver-expansion.md`

当前不进入实现队列，但不能遗忘：

- NUMA
- CPU topology
- 电源管理
- 热插拔
- cgroup/namespace
- LSM 类安全框架
- swap
- 完整 USB 栈
- 完整图形栈
- 音频
