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

### 1. init process

- `init` 是第一个用户态进程，负责启动 shell 和回收孤儿进程。
- kernel 只启动 init，不直接启动 shell 或具体命令。
- init 失败要有可观测错误和 fallback 策略。
- 后续 service manager 可以替换 init，但不影响 kernel 进程模型。

### 2. Shell and command execution

- shell 运行在用户态，通过 syscall、VFS 和 terminal 访问系统能力。
- shell 负责解析命令行、查找可执行文件、spawn/exec、wait 和显示退出状态。
- 内建命令和外部命令要区分，不能把所有工具写进 shell。
- shell 不能调用 kernel 内部符号或绕过 VFS。

### 3. Minimal libc and user ABI

- 建立最小 libc 封装 syscall、errno、字符串、内存、路径和文件 API。
- 用户程序通过 libc 或稳定 syscall ABI 访问 kernel，不包含 kernel 私有头文件。
- `argv/envp`、工作目录、文件描述符和退出码语义要逐步稳定。
- 后续移植更复杂工具时，优先补 libc/VFS/syscall 缺口，而不是给工具写特殊路径。

### 4. Observability interfaces

- kernel 状态通过 syscall 或虚拟文件系统暴露，例如 procfs/debugfs 的早期等价物。
- 调试命令读取这些接口，不直接链接 kernel 数据结构。
- 先提供进程列表、内存统计、mount/file 状态、timer/scheduler 简要状态。
- 后续 ftrace、perf、crash dump 和符号化输出放在可观测接口路线中。

### 5. Terminal and job control

- shell 从 terminal/tty 读取输入，不直接读取 keyboard event。
- 预留 session、controlling terminal、foreground process group 和 signal 交互。
- 早期可以没有完整 job control，但接口不能排斥 Ctrl-C、后台任务和管道。
- terminal 行编辑、回显和控制字符不写进 keyboard driver。

## Linux 参考原则

- 参考 Linux init 模型：kernel 启动第一个用户进程，系统策略留给用户态。
- 参考 Unix shell 模型：shell 通过 fork/exec/wait 或等价接口组合程序。
- 参考 Linux procfs/sysfs/debugfs：调试信息通过文件或稳定接口暴露，而不是用户程序读 kernel 内存。
- 参考 Linux tty/session/job control：交互输入输出是 terminal 层职责，不属于键盘驱动或 shell 内核特权。

## 非玩具化约束

- shell 必须运行在用户态。
- shell 不能直接调用 kernel 内部函数。
- 调试信息通过 syscall 或虚拟文件接口暴露。
- 工具能力应服务于后续 git 移植。
- shell 不是内核调试器，不能用 shell 规避系统调用和 VFS 的缺失。
- `git` 目标至少需要可用 libc、文件状态、权限、时间、路径、进程等待等接口。
- init、shell、基础命令不能混成一个二进制除非接口仍保持独立。
- 外部命令必须走进程创建和 VFS 加载路径。
- 用户态工具不能包含 kernel 私有结构体布局。
- 调试接口要只读优先，避免早期工具直接修改 kernel 状态。
- 缺失能力应补 syscall/libc/VFS，而不是给单个命令加后门。

## 验收方式

- 启动后进入 shell。
- shell 能读取键盘输入。
- shell 能执行至少两个外部命令。
- 命令可访问 VFS。
- 基础用户空间接口能支撑后续移植更复杂工具。
- init 能启动 shell 并回收退出的子进程。
- `ls` 通过 VFS readdir 工作，`cat` 通过 open/read 工作。
- shell 能显示外部命令退出状态。
- 调试命令通过虚拟文件或 syscall 读取状态。
- 用户态程序不直接包含 kernel 私有头。

## 当前状态

未开始。

进入本阶段前，需要 `05-input-events.md`、`06-storage-vfs.md` 和 `07-user-mode.md` 的最小闭环。

## 后续扩展

- 管道和重定向。
- 环境变量。
- cwd 和相对路径。
- job control。
- procfs/sysfs/debugfs。
- 更完整 libc。
- 动态链接。
- git 移植所需的文件、时间、权限和进程接口。
