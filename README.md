# Tianole

Tianole 是一个学习型自制操作系统项目，当前面向 `UEFI x86_64` 机器。

这个项目的主要目的有三点：

- 通过亲手实现一个简化但不玩具化的 OS，理解操作系统核心机制。
- 通过参考 Linux 的目录、分层和工程纪律，建立阅读 Linux 源码的上下文。
- 长期目标是在真机上安全启动，并逐步具备运行本地 `git` 的能力。

Tianole 不宣称自己是成熟 Linux 替代品。当前阶段更准确的定位是：用一个可运行、可验证、可持续演进的代码库，系统性学习 Linux 式内核工程。

## 当前状态

- 已跑通 `QEMU + OVMF` 下的最小 `UEFI` 启动链。
- 已拆分为 `bootloader + kernel`，并能加载独立 `kernel.elf`。
- 已在进入 kernel 前调用 `ExitBootServices`。
- 已建立 early log 基础：QEMU debug port 和 COM1 串口。
- 已建立 CPU exception 基础：GDT/TSS/IDT、trap frame、invalid opcode -> panic。
- 已建立物理内存基础：从 boot memory map 初始化最小物理页 allocator。
- 还没有完成完整中断、页表、内核堆、调度、进程、VFS、用户态和 shell。

## 当前目录

```text
tianole/
  AGENTS.md
  Makefile
  arch/
  block/
  crypto/
  docs/
  drivers/
  fs/
  include/
  init/
  ipc/
  kernel/
  lib/
  mm/
  net/
  scripts/
  security/
  sound/
  tools/
  usr/
  virt/
```

当前按接近 Linux 的方式分层，但不机械复制 Linux 的所有历史目录：

- `arch/`：架构相关代码
- `block/`：通用块层
- `crypto/`：通用加密和哈希算法
- `drivers/`：设备驱动
- `fs/`：VFS 和文件系统
- `init/`：初始化流程
- `ipc/`：进程间通信
- `kernel/`：通用内核主体逐步放这里
- `lib/`：通用基础库
- `mm/`：架构无关内存管理
- `net/`：网络协议栈
- `security/`：安全模型
- `sound/`：音频子系统
- `tools/`：宿主机侧开发工具
- `usr/`：初始用户空间
- `virt/`：虚拟化支持
- `include/tianole/`：项目自有共享接口
- `docs/`：正式设计和路线图

## 开发顺序

当前建议的实现顺序是：

1. 启动链
2. 早期调试输出
3. 内存与异常基础
4. 执行流与调度
5. 用户态入口
6. 文件系统与存储
7. shell / 工具
8. 面向 `git` 的补齐
9. 真机落地

更细的阶段说明见：

- [docs/roadmap.md](//wsl.localhost/Ubuntu/home/indole/tianole/docs/roadmap.md)

## 构建与运行

在 WSL2 / Ubuntu 中：

```bash
make
make run
```

无界面调试：

```bash
make run-headless
cat build/debug.log
```

本地完整检查：

```bash
scripts/check.sh
```

GitHub Actions 会在 push 和 pull request 时运行同一个检查入口。具体检查拆在 `scripts/checks/`，共享函数放在 `scripts/lib/`。

## 文档

- 路线图：[docs/roadmap.md](//wsl.localhost/Ubuntu/home/indole/tianole/docs/roadmap.md)
- 本机环境与真机启动约束：[docs/host-machine.md](//wsl.localhost/Ubuntu/home/indole/tianole/docs/host-machine.md)
- Linux 顶层目录映射：[docs/agents/linux-layout.md](//wsl.localhost/Ubuntu/home/indole/tianole/docs/agents/linux-layout.md)
- agent 入口：[AGENTS.md](//wsl.localhost/Ubuntu/home/indole/tianole/AGENTS.md)
