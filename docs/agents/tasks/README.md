# Agent 任务索引

这里记录当前阶段和下一步。细节放在各阶段文档里，不在索引里重复展开。

## 阶段顺序

1. `01-early-debug.md`：early log、panic、调试输出。
2. `02-cpu-interrupts.md`：GDT、TSS、IDT、异常和 IRQ 入口。
3. `03-memory.md`：物理页、页表、内核堆。
4. `04-time-scheduler.md`：timer、kernel thread、调度、wait queue。
5. `05-input-events.md`：input event、键盘、console 输入。
6. `06-storage-vfs.md`：块层、缓存、VFS、文件系统。
7. `07-user-mode.md`：syscall、用户态、进程、ELF 加载。
8. `08-shell-tools.md`：init、用户态 shell、基础工具。
9. `09-driver-expansion.md`：PCI、ACPI、更多设备驱动。
10. `10-real-machine.md`：真机启动和硬件差异处理。
11. `11-ipc-pty-signals.md`：IPC、PTY、signal、job control 基础。
12. `12-network-sockets.md`：网络栈、socket、loopback 和网卡接入。
13. `13-security-credentials.md`：凭证、权限、capability 和安全 hooks。
14. `14-observability-procfs-sysfs.md`：procfs、sysfs、debugfs、tracing。
15. `15-smp-scalability.md`：SMP、多核、per-cpu、IPI、锁规则。
16. `16-crypto-random.md`：crypto API、随机数、完整性基础。
17. `17-config-build-modules.md`：配置、构建矩阵、initcall 和模块边界。
18. `18-containers-namespaces-cgroups.md`：namespace、cgroup 和资源隔离。

## 当前状态

整体主线已经完成 `05-input-events.md` 的基础边界：timer、kernel thread、
wait queue、workqueue、PS/2 keyboard、input device 注册骨架、input event queue、
keycode/keymap/keysym、早期 tty line discipline、临时 input-to-tty bridge 和 early kdb
都有基础实现。当前可以从 05 收口，回到主线推进 `06-storage-vfs.md`。

`02-cpu-interrupts.md` 的关键架构债务也已回补到可继续推进的程度：trap frame、
IRQ/syscall/user-exception 返回边界、kernel/user trap 来源判断和 future user exception
policy 接口都有预留。后续 syscall 和用户态异常处理仍会继续扩展，但不应阻塞 06 的只读存储/VFS 主线。

| 阶段 | 状态 | 说明 |
| --- | --- | --- |
| `01-early-debug.md` | 基础完成 | early log、COM1、QEMU debug port、panic 已有。 |
| `02-cpu-interrupts.md` | 进行中 | 已有 GDT/TSS/IDT、trap frame、IRQ0/IRQ1；入口表已包含 gate/DPL/IST，vector 分区、legacy int 0x80 syscall 预留、kernel/user trap 来源判断、user exception policy 边界和 future user IRET frame `rsp/ss` 预留。 |
| `03-memory.md` | 基础完成 | 已有物理页分配、页表、page fault 诊断、内核堆。 |
| `04-time-scheduler.md` | 进行中 | 已有 PIT、kernel thread、基础调度、sleep、wait queue、workqueue。 |
| `05-input-events.md` | 基础完成 | 已有 input device 注册骨架、PS/2 keyboard、Linux 对齐 keycode、tty keymap/keysym、早期 tty、临时 input-to-tty bridge 和 early kdb。 |
| `06-storage-vfs.md` | 未开始 | 后续做块层和 VFS。 |
| `07-user-mode.md` | 未开始 | 后续做 syscall、用户态和进程。 |
| `08-shell-tools.md` | 未开始 | 当前 kdb 不是正式 shell。 |
| `09-driver-expansion.md` | 未开始 | 设备模型、PCI/ACPI、virtio/AHCI/NVMe 等驱动扩展。 |
| `10-real-machine.md` | 未开始 | 真机 UEFI 启动和硬件差异记录。 |
| `11-ipc-pty-signals.md` | 未开始 | pipe、signal、pty、job control、futex。 |
| `12-network-sockets.md` | 未开始 | socket core、loopback、IPv4/UDP/TCP、网卡接入。 |
| `13-security-credentials.md` | 未开始 | cred、uid/gid、权限、capability、安全 hooks。 |
| `14-observability-procfs-sysfs.md` | 未开始 | procfs、sysfs、debugfs、tracing、日志读取。 |
| `15-smp-scalability.md` | 未开始 | 多核启动、IPI、per-cpu、TLB shootdown、多核调度。 |
| `16-crypto-random.md` | 未开始 | 随机数、hash、crypto API、完整性基础。 |
| `17-config-build-modules.md` | 未开始 | 配置系统、构建矩阵、initcall、模块边界。 |
| `18-containers-namespaces-cgroups.md` | 长期规划 | namespace、cgroup、容器式隔离路线。 |

## 下一步

当前进入 `06-storage-vfs.md`：

- 先做只读块设备/VFS 骨架，让后续用户态 `cat/ls` 能复用同一套文件接口。
- `05-input-events.md` 只保留人工 QEMU 交互验收和后续扩展记录，不继续扩展 kdb 或 input_console 策略。
- `07-user-mode.md` 前仍需确定 syscall/sysret 与 int 0x80 的取舍；当前只预留 legacy int 0x80 vector 边界，不实现完整用户态 syscall。
- `kdb` 只保留为早期 debug 入口，不当作 shell 继续扩展。
- `11-18` 是长期路线图，当前不阻塞 06，但新代码必须避免写死会阻碍 IPC、网络、安全、可观测性、SMP 和配置系统的全局假设。

## 长期路线说明

`01-10` 是第一可用闭环，目标是让内核能启动、调度、读文件、进入用户态并运行基础 shell。这个闭环必要但不充分；如果停在这里，Tianole 仍然更接近教学内核或玩具内核。

参考 Linux 6.8 顶层组织，后续必须把 `ipc/`、`net/`、`security/`、`crypto/`、`procfs/sysfs/debugfs`、SMP 和配置/构建体系纳入路线。Tianole 不需要复制 Linux 的规模，但需要保留相同类型的工程边界：每个大子系统有自己的目录、对象生命周期、syscall/VFS/driver 接入点和测试入口。

长期路线不代表立即实现全部功能。它的作用是约束当前实现：不要把网络写进某个网卡驱动，不要把权限写进 VFS 的临时 if，不要把系统观测继续塞进 kdb，不要让单 CPU 假设扩散到所有锁和调度路径。
