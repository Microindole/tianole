# 13 安全、凭证与权限模型

## 目标

建立进程凭证、文件权限和能力边界，让 Tianole 的 VFS、进程和 IPC 不再默认“全系统同一个特权主体”。

## 前置条件

- 进程模型、VFS 和 syscall 基础可用。
- inode/file 已保留 owner、mode 或等价元数据位置。
- 用户态 init/shell 可运行最小程序。

## 建议边界

- `kernel/cred/`：进程凭证、uid/gid、capability 或等价权限位。
- `security/`：统一权限检查 hooks、后续 LSM 风格扩展。
- `fs/`：inode mode、ownership、permission checks。
- `kernel/process/`：进程创建、exec、setuid/setgid 边界。

## 实现内容

- `struct cred` 或等价进程凭证对象。
- uid/gid/euid/egid 或简化但可扩展的身份模型。
- 文件 mode、owner、permission check。
- capability 或最小特权位，避免所有特权都绑定 uid 0。
- exec/setuid 预留。
- chown/chmod/stat 的 ABI 位置。

### 1. Credentials

- 凭证应从进程控制块中独立出来，便于 fork/exec 和引用管理。
- 早期可以只有 root/user 两类，但结构必须支持扩展到 uid/gid/capability。
- 权限检查读取 cred，不应读取 shell 状态或全局当前用户字符串。

### 2. Filesystem permissions

- VFS lookup/open/read/write/exec 都应经过统一权限检查入口。
- inode 需要保留 mode、uid、gid、ctime/mtime 等元数据位置。
- 只读文件系统也需要表达权限，否则后续可写 FS 会返工。
- 设备文件、socket、pipe 权限应复用 VFS 权限语义。

### 3. Privilege transitions

- exec 时凭证变化、setuid 文件、capability 继承可以后续实现，但边界要预留。
- 用户态不能直接写 cred；必须通过 syscall 和权限检查。
- 权限失败要返回 errno，不应 panic。

### 4. Security hooks

- 早期不需要完整 LSM，但应有统一 hook 位置，例如 inode_permission、task_kill、socket_create。
- 后续 audit、sandbox、namespace、MAC 可以接入同一层。

## Linux 参考原则

- 参考 Linux `struct cred`、`security/`、VFS permission checks 和 capability 模型。
- 权限检查是核心路径，不是 shell 或文件系统私有逻辑。
- credentials 与 task/process 生命周期相关，但不应和 scheduler 结构混成一个不可拆对象。

## 非玩具化约束

- 不能默认所有用户态程序永远 kernel-level trusted。
- VFS 权限不能只靠文件名约定。
- `uid == 0` 特权不应散落在各子系统 if 语句中。
- 权限失败必须返回错误码。
- 权限模型不能要求重写 open/exec/socket 主路径。

## 验收方式

- 普通用户无法打开无读权限文件。
- 权限检查路径被 VFS selftest 覆盖。
- 进程凭证可在 fork/exec 后正确继承或替换。
- 权限错误能映射到用户态 errno。
- 后续添加 chmod/chown 不需要重写 VFS object model。

## 当前状态

未开始。

07/08 可以先用单一 root 凭证启动，但在引入多用户、文件权限或网络服务前必须补本阶段。

## 后续扩展

- capability 完整集合。
- LSM 风格 hook。
- audit 日志。
- namespace/cgroup 结合。
- setuid/setgid executable。
