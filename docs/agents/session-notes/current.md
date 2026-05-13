# 当前交接摘要

## 当前阶段

当前主线停留在 `docs/agents/tasks/04-time-scheduler.md`。`01-early-debug.md`、`02-cpu-interrupts.md` 和 `03-memory.md` 已具备基础能力；`05-input-events.md` 还没有开始。

下一个接手者应先读：

- `docs/agents/README.md`
- `docs/agents/code-style.md`
- `docs/agents/kernel-design.md`
- `docs/agents/tasks/README.md`
- `docs/agents/tasks/04-time-scheduler.md`
- 本文件

## 最近完成

- early log 增加 UEFI GOP framebuffer backend，QEMU 图形窗口能直接显示启动日志。
- `make run` / `make run-headless` 依赖 `kernel.elf`，避免只构建 bootloader 后启动失败。
- wait queue 增加显式 locked wakeup 接口：
  - `wait_queue_lock_irqsave()`
  - `wait_queue_unlock_irqrestore()`
  - `wait_queue_wake_one_locked()`
  - `wait_queue_wake_all_locked()`
- `wait_queue_wait()` 文档明确：如果条件由 wait queue 同步，写方必须在同一 queue lock 下修改条件并执行 locked wakeup，避免 lost wakeup。
- scheduler demo 中的条件等待路径已经按上述规则更新。

## 最近验证

最近一次修改后已执行：

```sh
python3 scripts/tools/check_structure.py --all
git diff --check
make all
scripts/check.sh
```

`scripts/check.sh` 会运行正常启动、异常和 page fault 测试路径。page fault 测试里的 page fault 日志是预期输出，不代表失败。

## 下一步

继续 `04-time-scheduler.md`，不要直接跳到 `05-input-events.md`。

推荐下一刀：

1. 收紧 thread lifecycle：补强 DEAD 回收边界、当前线程不能释放自身栈的断言、未来 join/wait 的状态预留。
2. 或者继续收紧 interrupt-exit reschedule：让 `sched_irq_exit()` 的边界更明确，为未来 syscall return/user-mode return 复用。
3. 如果继续 wait queue，则扩大“条件修改 + locked wakeup”规则覆盖面，并补自测。

键盘中断、scancode、input queue 和 terminal/tty 应等 `04-time-scheduler.md` 的等待、生命周期和 interrupt-exit 边界更稳定后再开始。

## 注意事项

- 不要把截图文件如 `a.png`、`b.png`、`c.png` 纳入提交，除非任务明确要求。
- 修改 C/H 文件后运行 `clang-format`。
- 结构性变更后运行 `python3 scripts/tools/check_structure.py --all`。
- 行为变更后至少运行 `make all`，调度/中断/内存路径变更应运行 `scripts/check.sh`。
- 提交前检查 `git diff --check`。
