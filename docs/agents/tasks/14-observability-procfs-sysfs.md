# 14 可观测性、procfs、sysfs 与 tracing

## 目标

把系统状态从 kdb/日志迁移到可查询的内核接口，让用户态工具能通过文件和 syscall 观察进程、内存、设备、网络和调度状态。

## 前置条件

- VFS 支持虚拟文件系统。
- 进程、设备模型和基础用户态可用。
- printk/log ring buffer 有基础能力或至少可查询日志快照。

## 建议边界

- `fs/proc/`：进程和内核状态视图。
- `fs/sysfs/`：设备模型、bus、driver、device 属性。
- `fs/debugfs/`：开发调试接口，默认不承诺稳定 ABI。
- `kernel/trace/`：tracepoints、event buffer、调试采样。
- `kernel/printk/`：日志 ring buffer 和用户态读取入口。

## 实现内容

- procfs：`/proc`, `/proc/<pid>`, `/proc/meminfo`, `/proc/cpuinfo` 或等价最小集合。
- sysfs：设备、driver、bus 的只读属性导出。
- debugfs：非稳定调试节点，替代 kdb 扩展冲动。
- tracepoints：调度、IRQ、syscall、block、net 的事件挂点。
- 用户态读取内核日志的接口，例如 `/dev/kmsg` 或 `/proc/kmsg` 风格入口。

### 1. Virtual filesystem model

- procfs/sysfs/debugfs 都应通过 VFS mount 和 file operations 暴露。
- 虚拟文件内容应按 read offset 语义工作，不能只打印到 console。
- 生命周期要和进程/device 引用对齐，避免读取过程中对象释放。

### 2. Process and system views

- `/proc/<pid>` 应来自 process table，不应由 shell 手工维护。
- meminfo/cpuinfo/sched 状态要从对应子系统读取摘要。
- 早期可以只读，但格式应稳定到足以支持用户态工具。

### 3. Device model exposure

- sysfs 反映 device/driver/bus 层次，不是驱动私有日志目录。
- 属性读写需要权限和锁规则；早期优先只读。
- 后续热插拔、电源管理、网络接口配置都依赖可观测设备树。

### 4. Tracing and debug

- tracepoint 要是低耦合 hook，不应把 tracing 逻辑写进业务路径。
- ring buffer 要有容量和丢失统计。
- debugfs 不承诺稳定 ABI，不能被基础用户态工具依赖。

## Linux 参考原则

- 参考 Linux `fs/proc/`、`fs/sysfs/`、`fs/debugfs/`、`kernel/trace/`。
- procfs 面向进程和系统状态，sysfs 面向设备模型，debugfs 面向开发调试。
- 用户态工具应该通过文件接口观察系统，而不是要求内核 kdb 增加命令。

## 非玩具化约束

- 不继续扩展 kdb 作为万能观测入口。
- 虚拟文件必须遵守 VFS read/open/close 语义。
- procfs/sysfs 读取不能长期持有全局锁导致系统停顿。
- debugfs 节点不能成为稳定用户 ABI。
- trace buffer 溢出必须可观测。

## 验收方式

- 用户态 `cat /proc/meminfo` 或等价工具能读取内存摘要。
- `/proc/<pid>` 能列出至少 pid、状态、父进程。
- sysfs 能列出已注册 input/block/net 或平台设备。
- debugfs 可暴露一个只读调试计数器。
- trace selftest 能记录并读取一次调度或 syscall 事件。

## 当前状态

未开始。

本阶段应在 VFS、用户态和 device model 有基础后推进。

## 后续扩展

- ftrace/perf 风格采样。
- crash dump。
- BPF 或等价动态观测机制。
- 更完整 `/proc` ABI。
- structured kernel logs。
