# Tianole

Tianole 是一个面向 `UEFI x86_64` 机器的自制操作系统项目。

当前目标不是一次性做“大而全”的系统，而是按 Linux 风格的分层思路，逐步把启动链、内核基础设施、用户态和文件系统建立起来，最终能在真机上安全启动，并逐步具备运行本地 `git` 的能力。

## 当前状态

- 已跑通 `QEMU + OVMF` 下的最小 `UEFI` 启动链
- 已拆分为 `bootloader + kernel`
- `bootloader` 已能加载独立 `kernel.elf`
- `kernel_main()` 已能实际执行

## 当前目录

```text
tianole/
  AGENTS.md
  Makefile
  arch/
  docs/
  include/
  kernel/
  scripts/
```

当前还没有直接复制 Linux 的完整目录树，但已经开始按接近 Linux 的方式分层：

- `arch/`：架构相关代码
- `kernel/`：通用内核主体逐步放这里
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

## 文档

- 路线图：[docs/roadmap.md](//wsl.localhost/Ubuntu/home/indole/tianole/docs/roadmap.md)
- 本机环境与真机启动约束：[docs/host-machine.md](//wsl.localhost/Ubuntu/home/indole/tianole/docs/host-machine.md)
- agent 入口：[AGENTS.md](//wsl.localhost/Ubuntu/home/indole/tianole/AGENTS.md)
