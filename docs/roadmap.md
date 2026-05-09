# Tianole 路线图

这个文件只做路线入口，不承载详细设计。详细任务放在
`docs/agents/tasks/`，避免每次修改都把长期上下文塞进总路线。

## 当前状态

基础已具备：

- 最小 `UEFI -> bootloader -> kernel` 启动链。
- bootloader 与 kernel 拆分。
- 独立 `kernel.elf` 加载。
- 进入 kernel 前调用 `ExitBootServices`。
- `memory map` 通过 `boot_info` 传入 kernel。
- kernel 能统计 memory map 描述符数量和 conventional memory 页数。
- early log 基础：通用前端、x86 backend、QEMU debug port、COM1 串口。
- panic 基础：最小 `panic()` 已接入 early log 和架构 halt 路径。
- CPU exception 基础：x86_64 GDT/TSS/IDT、exception vector 0-31、统一 trap frame。
- trap 验证：invalid opcode 已能进入 trap dispatch 并最终进入 panic。
- 物理内存基础：已从 boot memory map 建立最小物理页 allocator。
- 物理页验证：`alloc_page/free_page` selftest 已接入启动检查。
- 页表基础：已切换到内核自有 PML4，并提供最小 `map_page/unmap_page/virt_to_phys`。
- 页表验证：页表 map/unmap/query selftest 已接入启动检查。
- 构建系统已拆成根 Makefile、`arch/x86/Makefile` 和目录 Makefile。
- `scripts/check.sh` 已验证启动日志、串口日志和 invalid opcode 异常路径，GitHub Actions 已接入。
- `.clang-format` 已用于强制当前 C 代码风格。

后续未完成：

- 完整日志体系：printk、log level、ring buffer、oops、符号化、crash dump。
- 完整中断体系：PIC/APIC、外部 IRQ、timer interrupt。
- 内存管理扩展：长期内存区域模型、page fault 策略、内核堆。
- 后续主线：调度、进程、文件系统、用户态和 shell。

## 当前下一步

下一步执行：

- `docs/agents/tasks/03-memory.md`

目标：

- 建立最小内核堆。
- 让 page fault 能进入现有异常路径并输出有效诊断。

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
