# 18 Namespace、Cgroup 与资源隔离

## 目标

在已有进程、VFS、网络和安全模型上建立资源隔离基础，让 Tianole 具备现代 Linux 风格容器能力的长期演进方向。

## 前置条件

- 进程模型、VFS、网络、凭证和 procfs/sysfs 基础可用。
- SMP/锁规则足够稳定。
- 用户态工具可以创建进程、挂载文件系统并配置网络。

## 建议边界

- `kernel/ns/`：pid/mount/net/uts/ipc/user namespace。
- `kernel/cgroup/`：资源控制层级和 controller。
- `fs/cgroup/`：cgroup 文件接口。
- `security/`：namespace 与 capability 的交互。
- `net/`：network namespace 接入。

## 实现内容

- pid namespace：进程号视图隔离。
- mount namespace：根目录和挂载表隔离。
- net namespace：网络设备、地址和 socket 隔离。
- user namespace：凭证映射和权限隔离。
- cgroup：CPU、memory、pids、io 的资源统计和限制。
- namespace/cgroup 的 VFS 管理接口。

### 1. Namespace model

- namespace 是对象，不是进程结构里的若干 bool。
- 进程引用 namespace set，fork/clone/exec 规则要明确。
- mount/net/user namespace 与 VFS、net、security 子系统耦合，需要统一边界。

### 2. Cgroup model

- cgroup 是层级资源控制，不是简单 per-process limit。
- controller 需要统一 attach/detach、charge/uncharge、stat 接口。
- memory cgroup 需要和 page allocator/page cache 对齐；CPU cgroup 需要和 scheduler 对齐。

### 3. User ABI

- 早期可以只读统计，但接口路径应接近 cgroupfs/procfs/sysfs 风格。
- 容器启动工具属于用户态，不应在内核硬编码“container mode”。

## Linux 参考原则

- 参考 Linux namespace、cgroup v2、procfs 和 security/capability 交互。
- 隔离能力是多个核心子系统的组合，不是单个 syscall demo。
- cgroup controller 需要从资源产生处计费，而不是事后猜测。

## 非玩具化约束

- 不能把 namespace 简化成全局当前 root path。
- 资源限制必须在资源分配路径生效。
- user namespace 和 capability 交互必须谨慎，不能默认放大权限。
- 容器用户态不能依赖 kernel 私有结构。
- cgroup 统计和限制要能通过 VFS 接口观察。

## 验收方式

- 两个进程可处于不同 pid namespace 并看到不同 pid 视图。
- mount namespace 可拥有不同 root 或 mount table。
- cgroup pids 或 memory 计数可读，并能拒绝超过限制的创建/分配。
- namespace/cgroup 对象引用释放正确。

## 当前状态

未开始。

这是长期路线，不阻塞 06-12，但不能完全缺席规划，否则后续 process/VFS/net/security 很容易写死全局假设。

## 后续扩展

- 完整 cgroup v2 controller。
- 容器 runtime 用户态工具。
- seccomp。
- checkpoint/restore。
- 更细粒度 resource accounting。
