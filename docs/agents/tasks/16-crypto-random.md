# 16 Crypto、随机数与完整性基础

## 目标

建立内核加密和随机数基础设施，为网络、安全、文件校验、签名和用户态随机源提供统一能力。

## 前置条件

- timer、interrupt、device entropy 来源可用。
- 用户态 syscall 或 VFS 设备文件可暴露随机数。
- 内存分配和锁可用。

## 建议边界

- `crypto/`：hash、cipher、AEAD、算法注册。
- `kernel/random/`：entropy pool、CSPRNG、`getrandom` 或 `/dev/urandom`。
- `security/`：签名、完整性校验、keyring 的后续接入点。
- `drivers/char/`：random device 文件或等价接口。

## 实现内容

- 随机数池和 CSPRNG。
- entropy 来源：timer jitter、interrupt timing、device events，后续硬件 RNG。
- hash：SHA-256 或 BLAKE2s 等基础算法。
- 用户态随机接口：`getrandom` 或 `/dev/urandom`。
- 算法注册表，避免调用方硬编码具体算法实现。
- 后续 TLS、磁盘校验、模块签名、Secure Boot 相关能力预留。

### 1. Random subsystem

- 随机数不能用简单计数器或 timer 直接伪装。
- 需要区分 entropy collection、CSPRNG state 和用户态输出接口。
- 早期可以保守阻塞或标记未初始化，但行为必须明确。
- 硬件 RNG 后续作为 entropy source 注册，不应绕过 random core。

### 2. Crypto API

- 调用方通过算法名字或操作表获取 hash/cipher，不直接绑定某个文件里的函数。
- 算法状态、输入输出 buffer 和错误码要规范。
- 早期优先 hash/random，不急于实现复杂 cipher。

### 3. Integrity and keys

- 内核镜像、initramfs、模块或配置签名后续会依赖 keyring/crypto。
- 不要求早期强制安全启动，但要避免把校验逻辑写死在 bootloader demo 中。

## Linux 参考原则

- 参考 Linux `crypto/`、`drivers/char/random.c`、keyring 和 integrity 子系统思路。
- 加密算法是可注册能力，随机数是核心服务，不属于某个网络或安全模块私有实现。

## 非玩具化约束

- 不能使用 `timer_ticks()` 直接作为随机数接口。
- 随机数初始化状态必须可观测。
- crypto 调用方不能硬编码单一算法实现路径。
- 用户态随机接口不能泄露未初始化内存。
- 安全相关失败不能静默降级。

## 验收方式

- random selftest 能输出非重复随机块，并报告初始化状态。
- hash selftest 对标准 test vector 通过。
- 用户态能通过 getrandom 或 `/dev/urandom` 读取随机数。
- 新 entropy source 可注册到 random core。

## 当前状态

未开始。

网络和安全阶段前应至少有随机数和 hash 基础。

## 后续扩展

- AES/ChaCha20/Poly1305。
- keyring。
- 模块签名。
- initramfs 完整性校验。
- 硬件 RNG。
