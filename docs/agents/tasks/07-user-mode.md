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

## 非玩具化约束

- syscall 表要稳定记录。
- 用户指针不能直接信任。
- 进程、线程、地址空间结构不要混成一个对象。
- 用户态崩溃不能直接拖垮 kernel。
- syscall 不能只按当前 demo 程序硬编码。
- 进程资源释放必须有明确所有权和引用计数规则。
- `fork/exec/wait`、signal、pipe、poll/select 是运行复杂 Unix 程序的关键路径，不能被路线遗漏。

## 验收方式

- 最小用户程序可以输出并退出。
- kernel 能回收用户进程资源。
- 非法用户访问能进入异常处理并终止进程。
- syscall ABI 文档化，后续可以被 libc 绑定。
