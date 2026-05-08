# 08 init、shell 与工具

## 目标

建立可交互系统入口，并用用户态工具验证进程、VFS、输入和输出路径。

## 前置条件

- 用户态可运行程序。
- VFS 可读目录和文件。
- 输入事件和终端输出可用。

## 建议边界

- `user/init/`：第一个用户态进程。
- `user/shell/`：shell。
- `user/bin/`：基础命令。
- `lib/` 或 `user/lib/`：最小 libc。

## 实现内容

- 启动第一个 `init`。
- 启动 shell。
- 实现 `echo`、`pwd`、`ls`、`cat` 等基础命令。
- 增加查看 kernel 状态的调试接口。
- 规划最小 libc、errno、环境变量、时间接口、路径接口和 `stat/chmod` 类文件接口。
- 为 tty、session、job control 留出后续位置。
- 为 procfs/sysfs/debugfs、ftrace、perf、crash dump 类可观测接口预留路径。

## 非玩具化约束

- shell 必须运行在用户态。
- shell 不能直接调用 kernel 内部函数。
- 调试信息通过 syscall 或虚拟文件接口暴露。
- 工具能力应服务于后续 git 移植。
- shell 不是内核调试器，不能用 shell 规避系统调用和 VFS 的缺失。
- `git` 目标至少需要可用 libc、文件状态、权限、时间、路径、进程等待等接口。

## 验收方式

- 启动后进入 shell。
- shell 能读取键盘输入。
- shell 能执行至少两个外部命令。
- 命令可访问 VFS。
- 基础用户空间接口能支撑后续移植更复杂工具。
