# Linux 顶层目录映射

这个文档记录 Tianole 参考 Linux 顶层目录时的取舍。目标是学习 Linux 的工程分层，
而不是机械复制 Linux 的全部历史目录。

## 已采用或已预留

- `arch/`：对应 Linux `arch/`，放架构相关代码。
- `block/`：对应 Linux `block/`，后续放通用块层。
- `crypto/`：对应 Linux `crypto/`，后续放通用加密和哈希算法。
- `docs/`：对应 Linux `Documentation/`，Tianole 使用小写 `docs/`。
- `drivers/`：对应 Linux `drivers/`，后续放设备驱动和总线驱动。
  - `drivers/tty/`：对应 Linux `drivers/tty/`，放 tty、terminal 和 line discipline 相关实现。
- `fs/`：对应 Linux `fs/`，后续放 VFS 和具体文件系统。
- `include/`：对应 Linux `include/`，放公共头文件。
- `init/`：对应 Linux `init/`，后续放初始化流程。
- `ipc/`：对应 Linux `ipc/`，后续放 IPC、signal、pipe、futex 等能力。
- `kernel/`：对应 Linux `kernel/`，放调度、进程和核心内核逻辑。
  - `kernel/printk/`：对应 Linux `kernel/printk/`，放 printk、日志缓冲和 console 注册逻辑。
- `lib/`：对应 Linux `lib/`，后续放通用基础库。
- `mm/`：对应 Linux `mm/`，放内存管理。
- `net/`：对应 Linux `net/`，后续放网络协议栈。
- `scripts/`：对应 Linux `scripts/`，放构建、检查和维护脚本。
- `security/`：对应 Linux `security/`，后续放权限和安全策略。
- `sound/`：对应 Linux `sound/`，后续放音频子系统。
- `tools/`：对应 Linux `tools/`，后续放宿主机侧开发工具。
- `usr/`：对应 Linux `usr/`，后续放初始用户空间。
- `virt/`：对应 Linux `virt/`，后续放虚拟化相关支持。

## 暂不采用

- `certs/`：证书和签名链路还不是当前阶段目标，后续可按安全启动需求加入。
- `io_uring/`：这是 Linux 的特定异步 I/O 子系统，Tianole 早期不引入。
- `rust/`：当前内核实现语言是 C/ASM，暂不引入 Rust 子树。
- `samples/`：示例代码暂不放顶层，后续可按教学或测试需要决定。

## 目录使用原则

- 可以预留目录，但不要在目录中放临时代码。
- 只有当一个目录对应的子系统真正开始实现时，才添加 `Makefile` 和源文件。
- 目录 README 用来说明长期职责，不替代正式设计文档。
- 如果后续发现目录边界不合理，优先更新本文件和 `docs/agents/code-style.md`。
