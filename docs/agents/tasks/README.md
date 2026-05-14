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

## 当前状态

整体主线已经推进到 `05-input-events.md`：timer、kernel thread、
wait queue、workqueue、PS/2 keyboard、input console 和临时 early kdb
都已有基础实现。当前不是回退到早期阶段，而是在继续 tty/user mode
之前回补 `02-cpu-interrupts.md` 的架构债务。

这次回补的原因是：后续 syscall、用户态异常返回、NMI、更多 IRQ 和
真正 tty/terminal 都依赖稳定的 trap/IRQ 边界。如果 exception policy
继续混在 default handler 或临时 dispatch 里，后面会在 `05`、`07`
阶段反复返工。

| 阶段 | 状态 | 说明 |
| --- | --- | --- |
| `01-early-debug.md` | 基础完成 | early log、COM1、QEMU debug port、panic 已有。 |
| `02-cpu-interrupts.md` | 进行中 | 已有 GDT/TSS/IDT、trap frame、IRQ0/IRQ1；入口表已包含 gate/DPL/IST，vector 分区、legacy int 0x80 syscall 预留、kernel/user trap 来源判断、user exception policy 边界和 future user IRET frame `rsp/ss` 预留。 |
| `03-memory.md` | 基础完成 | 已有物理页分配、页表、page fault 诊断、内核堆。 |
| `04-time-scheduler.md` | 进行中 | 已有 PIT、kernel thread、基础调度、sleep、wait queue、workqueue。 |
| `05-input-events.md` | 进行中 | 已有 input event、PS/2 keyboard、input console、临时 early kdb。 |
| `06-storage-vfs.md` | 未开始 | 后续做块层和 VFS。 |
| `07-user-mode.md` | 未开始 | 后续做 syscall、用户态和进程。 |
| `08-shell-tools.md` | 未开始 | 当前 kdb 不是正式 shell。 |

## 下一步

当前优先回补 `02-cpu-interrupts.md` 的债务：

- 回到 `05-input-events.md`：把临时 input console 往 tty/terminal 雏形推进。
- 在 `07-user-mode.md` 前确定 syscall/sysret 与 int 0x80 的取舍；当前只预留 legacy int 0x80 vector 边界，不实现完整用户态 syscall。
- default handler 只保留为未知或暂未覆盖 vector 的兜底，不能继续承载常见异常策略。

然后回到 `05-input-events.md`：把临时 input console 往 tty/terminal 雏形推进。`kdb` 只保留为早期 debug 入口，不当作 shell 继续扩展。
