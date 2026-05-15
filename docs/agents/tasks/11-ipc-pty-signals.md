# 11 IPC、PTY、Signal 与终端语义

## 目标

让 Tianole 的用户态进程不再只是孤立运行的 ELF，而是能通过 Unix 风格的进程间通信、信号和终端控制形成可组合系统。

## 前置条件

- `07-user-mode.md` 已有进程、syscall、用户指针检查和 exit/wait 基础。
- `08-shell-tools.md` 已有 init、shell 和基础外部命令。
- VFS file object 可以承载非普通文件对象。

## 建议边界

- `kernel/ipc/`：pipe、event、共享内存、futex 等 IPC 基础。
- `kernel/signal/`：signal pending、mask、delivery、return frame。
- `drivers/tty/`：PTY、line discipline、session/foreground process group 交互。
- `fs/` 或 `kernel/fs/`：pipe/socket/pty 等特殊 file operations。

## 实现内容

- pipe：读写端、阻塞/非阻塞、EOF、引用计数。
- signal：发送、屏蔽、默认动作、用户态 handler、signal return。
- pty：master/slave、terminal input/output、控制终端。
- process group/session：前台进程组、job control 的内核基础。
- futex 或等价用户态同步入口，作为后续 libc/pthread 的基础。
- eventfd/signalfd/poll/select 的接口预留。

### 1. Pipe and special files

- pipe 必须通过 VFS file operations 暴露，不应由 shell 或 syscall 特判。
- pipe buffer 要有容量、阻塞、唤醒、EOF 和引用计数语义。
- 关闭最后一个 writer 后，reader 应读到 EOF；关闭最后一个 reader 后，writer 应得到错误或 signal。
- 后续 socket、pty、eventfd 应复用“特殊 file object + wait queue”的模型。

### 2. Signal model

- 区分 thread signal、process signal、pending set、blocked mask 和默认动作。
- 用户态异常、kill syscall、terminal job control 都应走同一 signal 递送边界。
- signal handler frame 必须由内核构造，并能通过 signal return syscall 恢复。
- 早期可先实现少量 signal，但结构不能只服务 Ctrl-C demo。

### 3. PTY and terminal control

- PTY master/slave 是用户态 terminal emulator、shell 和 job control 的关键接口。
- line discipline 不能只存在于 kernel kdb；正式 shell 输入应走 tty/pty 文件接口。
- 前台进程组控制 terminal input signal，例如 Ctrl-C、Ctrl-Z。
- 后续图形终端、串口终端和 PTY 应复用同一 tty/line discipline 边界。

### 4. Futex and wait primitives

- futex 作为用户态锁的内核慢路径，不能用全局大锁模拟所有等待。
- futex wait/wake 需要用户地址校验、值比较和 wait queue 生命周期。
- 后续 pthread、condition variable、runtime 都依赖这个边界。

## Linux 参考原则

- 参考 Linux `ipc/`、`kernel/signal.c`、`fs/pipe.c`、`drivers/tty/pty.c`。
- pipe、pty、socket 都是 file operations，不应绕过 VFS。
- signal delivery 和 syscall/trap return 边界耦合，必须和 `trap_frame`/用户态返回模型对齐。
- futex 是用户态同步和调度器之间的窄接口，不应把 pthread 语义写进 scheduler。

## 非玩具化约束

- shell 管道不能通过 shell 内部缓冲假装实现，必须有内核 pipe file。
- signal 不能只设置一个全局 flag，必须归属进程/线程。
- terminal job control 不应写进 keyboard driver 或 kdb。
- IPC 对象必须有引用计数和关闭语义。
- 阻塞路径必须使用 wait queue 或等价机制，不能忙等。
- 用户态 handler 返回不能破坏用户栈或内核栈边界。

## 验收方式

- `echo hello | cat` 或等价用户态测试可以通过 pipe 传输数据。
- `Ctrl-C` 能转成前台进程的 signal，而不是由 shell 私自杀进程。
- 父进程能 wait 到被 signal 终止的子进程状态。
- PTY master/slave 可以传输基础字节流。
- futex selftest 能完成 wait/wake 和超时路径。

## 当前状态

未开始。

进入本阶段前，至少需要用户态 syscall、VFS file object、wait queue 和基础 tty 可用。

## 后续扩展

- POSIX signal 完整集合。
- signalfd、eventfd、timerfd。
- System V IPC 或 POSIX shm。
- 完整 job control。
- 多线程 signal 语义。
