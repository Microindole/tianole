# 07 用户态、系统调用与进程

## 目标

让 Tianole 从只运行 kernel 代码，进入可运行用户程序的阶段。

## 前置条件

- 虚拟内存可用。
- VFS 可读取 ELF 文件或 init 程序。
- 异常和 syscall 入口可用。

## 建议边界

- `kernel/syscall/`：系统调用入口和分发。
- `kernel/process/`：进程、线程、地址空间。
- `kernel/elf/`：用户 ELF 加载。
- `arch/x86/`：ring 3 切换和 syscall 指令支持。

## 实现内容

- 建立用户地址空间。
- 建立 syscall ABI。
- 建立用户指针检查。
- 加载用户 ELF。
- 支持进程退出和等待。
- 早期可先实现 `spawn/exit/wait`，再演进到 `fork/exec`。
- 为 POSIX syscall、`fcntl/stat/chmod`、signal、pipe、socket、eventfd、futex、共享内存、poll/select 和 IPC 预留 ABI 边界。
- 为 uid/gid、用户/组、文件权限、进程权限、capability 或等价隔离模型预留进程凭据结构。

### 1. User address space

- 每个进程需要独立地址空间，内核映射和用户映射边界清楚。
- 用户页权限必须区分 user/supervisor、read/write、execute。
- 进程退出时必须释放用户页、页表页、VFS 引用和其他资源。
- 后续 `fork`、COW、`mmap` 和 shared memory 要能接入同一地址空间模型。

### 2. Syscall ABI

- 定义 syscall number、参数寄存器、返回值、错误码和 clobber 规则。
- syscall 表要稳定记录，不能由 demo 程序和 kernel 临时约定。
- syscall handler 必须复制和检查用户指针，不能直接解引用用户地址。
- syscall 返回边界要能和调度、signal、进程退出共享。

### 3. Process/thread model

- 区分进程、线程、地址空间、内核调度线程和打开文件表。
- 早期可以一个进程一个线程，但结构不能排斥多线程。
- 建立 pid、父子关系、退出码、wait 状态和 zombie 回收。
- 后续 `fork/exec/wait`、线程、signal 和 job control 要有可扩展空间。

### 4. ELF loader

- ELF loader 只负责校验和映射 ELF，不负责文件系统路径策略。
- 检查 ELF header、program header、segment 权限、对齐、入口地址和用户地址范围。
- 加载器要建立用户栈、argc/argv/envp 或最小等价启动约定。
- 非法 ELF 必须返回错误，不能 panic。

### 5. Fault and signal boundary

- 用户态 page fault、invalid opcode、general protection 等异常应终止当前进程或转成后续 signal。
- kernel fault 和 user fault 策略必须分开。
- 预留 signal、kill、wait status 和 core dump/debug 信息。
- 用户态崩溃不能破坏内核调度器、VFS 或其他进程。

## Linux 参考原则

- 参考 Linux task/mm/files 分离：调度实体、地址空间和文件表不是同一个对象。
- 参考 Linux syscall ABI：syscall number 和参数规则稳定，libc 可以独立绑定。
- 参考 Linux ELF loader：VFS 提供文件，loader 校验 program header 并建立用户映射。
- 参考 Linux user access helpers：所有用户指针都要复制/检查，不能直接信任。
- 参考 Linux wait/exit/zombie 模型：进程退出和资源释放分阶段完成。

## 非玩具化约束

- syscall 表要稳定记录。
- 用户指针不能直接信任。
- 进程、线程、地址空间结构不要混成一个对象。
- 用户态崩溃不能直接拖垮 kernel。
- syscall 不能只按当前 demo 程序硬编码。
- 进程资源释放必须有明确所有权和引用计数规则。
- `fork/exec/wait`、signal、pipe、poll/select 是运行复杂 Unix 程序的关键路径，不能被路线遗漏。
- 用户态入口和 syscall 返回必须经过明确的 arch 边界。
- ELF loader 不能把文件系统、权限和地址空间策略混在一个函数里。
- 用户栈布局要文档化，不能靠某个 demo 程序猜测。
- errno/错误码语义要能被 libc 映射。
- 进程退出不能泄漏页表、VFS file、cwd、argv/envp 或 wait 状态。

## 验收方式

- 最小用户程序可以输出并退出。
- kernel 能回收用户进程资源。
- 非法用户访问能进入异常处理并终止进程。
- syscall ABI 文档化，后续可以被 libc 绑定。
- 非法 syscall number 返回明确错误。
- 用户指针越界不会导致 kernel panic。
- ELF 错误输入有测试，返回加载失败。
- 父进程能 wait 到子进程退出码。
- 用户态进程反复启动和退出后，内存和 file 引用不会持续增长。

## 当前状态

未开始。

进入本阶段前，`03-memory.md` 需要支持用户地址空间基础，`06-storage-vfs.md` 需要能读取 ELF 或 init 程序。

## 后续扩展

- `fork/exec/wait`。
- signal。
- pipe。
- shared memory。
- futex。
- poll/select。
- uid/gid、权限和 capability。
- 动态链接。
- ptrace 或基础调试接口。
