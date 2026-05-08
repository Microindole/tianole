# Tianole 路线图

这个文件只做路线入口，不承载详细设计。详细任务放在
`docs/agents/tasks/`，避免每次修改都把长期上下文塞进总路线。

## 当前状态

已经完成：

- 最小 `UEFI -> bootloader -> kernel` 启动链。
- bootloader 与 kernel 拆分。
- 独立 `kernel.elf` 加载。
- 进入 kernel 前调用 `ExitBootServices`。
- `memory map` 通过 `boot_info` 传入 kernel。
- kernel 能统计 memory map 描述符数量和 conventional memory 页数。
- kernel early log 已拆成通用前端和 x86 backend。
- x86 early log backend 同时输出到 QEMU debug port 和 COM1 串口。
- 最小 `panic()` 已接入 early log 和架构 halt 路径。
- 构建系统已拆成根 Makefile、`arch/x86/Makefile` 和目录 Makefile。
- `scripts/check.sh` 已验证启动日志和串口日志，GitHub Actions 已接入。
- `.clang-format` 已用于强制当前 C 代码风格。

还没有完成：

- oops 早期错误路径。
- GDT/IDT/异常/中断。
- 物理页分配器、虚拟内存和内核堆。
- 调度、进程、文件系统、用户态和 shell。

## 当前下一步

下一步执行：

- `docs/agents/tasks/02-cpu-interrupts.md`

目标：

- 建立 x86_64 GDT/TSS/IDT。
- 建立异常入口和 trap frame。
- 让未处理异常进入 panic。
- 为后续 IRQ、timer 和 page fault 做准备。

## 任务路由

按顺序推进：

1. `docs/agents/tasks/01-early-debug.md`
2. `docs/agents/tasks/02-cpu-interrupts.md`
3. `docs/agents/tasks/03-memory.md`
4. `docs/agents/tasks/04-time-scheduler.md`
5. `docs/agents/tasks/05-input-events.md`
6. `docs/agents/tasks/06-storage-vfs.md`
7. `docs/agents/tasks/07-user-mode.md`
8. `docs/agents/tasks/08-shell-tools.md`
9. `docs/agents/tasks/09-driver-expansion.md`
10. `docs/agents/tasks/10-real-machine.md`

Linux 级长期能力缺口和对应阶段，见：

- `docs/agents/tasks/README.md`
