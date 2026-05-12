# 06 存储、缓存与 VFS

## 目标

建立文件访问路径：块设备或 initramfs 提供数据，VFS 提供统一文件接口。

## 前置条件

- 内存管理可用。
- 锁和等待机制可用。
- 用户态之前可以先由 kernel 测试 VFS。

## 建议边界

- `drivers/block/`：块设备抽象和具体设备。
- `kernel/fs/`：VFS。
- `mm/`：页缓存或块缓存。
- `fs/`：具体文件系统实现。

## 实现内容

- 建立 block device 接口。
- 初期支持 initramfs/ramdisk。
- 建立 inode、dentry、file 等 VFS 对象。
- 建立路径解析。
- 支持 open/read/close/readdir。
- 后续再加入 write 和持久化文件系统。
- 为 page cache、block cache、writeback 和崩溃一致性预留边界。

### 1. Block layer 与数据来源

- 初期可以使用 initramfs/ramdisk，让 VFS 在没有真实磁盘驱动时先闭环。
- block device 接口要表达设备大小、块大小、read/write 请求和完成状态。
- 块设备驱动不直接解析文件系统，也不直接服务 shell。
- 后续 virtio-blk、AHCI、NVMe 应通过同一 block device 接口接入。

### 2. VFS object model

- 建立 superblock、inode、dentry、file 或等价对象边界。
- inode 表示文件对象和元数据，dentry 表示路径缓存，file 表示一次打开实例和 offset。
- 对象生命周期要有引用计数或明确所有权。
- VFS 调用方不能直接依赖某个具体文件系统结构。

### 3. Path lookup

- 路径解析要支持绝对路径、相对路径、`.`、`..` 和目录边界。
- lookup 需要区分不存在、不是目录、权限不足和 I/O 错误。
- 当前可以先不实现 mount namespace，但接口要为 mount point 和根目录切换留位置。
- 路径字符串来自用户态时，必须经过用户指针复制和长度检查。

### 4. File operations

- 先实现 `open/read/close/readdir`，再加入 `write/create/unlink/rename`。
- file operation 通过表或函数指针分发，VFS 不写死 ramfs/initramfs。
- 目录读取要有稳定迭代语义，不能只打印日志。
- offset、EOF、短读和错误码要从一开始定义清楚。

### 5. Cache 与一致性预留

- page cache/block cache 不应属于某个具体磁盘驱动。
- 只读阶段可以不做 writeback，但接口要保留 dirty、flush、invalidate。
- 后续可写文件系统需要定义崩溃一致性策略，例如 journal、copy-on-write 或简单同步写。
- VFS、MM 和 block layer 的缓存边界要清楚，避免同一数据被多套缓存重复管理。

## Linux 参考原则

- 参考 Linux VFS：上层通过通用 inode/file/dentry/superblock 语义访问不同文件系统。
- 参考 Linux block layer：文件系统发出块请求，具体设备驱动处理硬件细节。
- 参考 Linux page cache：文件数据缓存属于 MM/VFS 共同边界，而不是具体文件系统私有数组。
- 参考 Linux file operations：具体文件系统实现操作表，VFS 负责统一分发和生命周期。

## 非玩具化约束

- VFS 不依赖某个具体文件系统。
- 文件系统不直接依赖 shell 或进程加载器。
- 路径解析和引用计数要从一开始有边界。
- 缓存层不能和具体磁盘驱动耦合。
- VFS 对象生命周期必须明确，不能靠全局静态对象规避释放问题。
- 文件权限、时间戳、inode 编号等 POSIX 语义要保留位置。
- open file 和 inode 不能混成同一个对象。
- VFS 错误要能映射到后续用户态 errno。
- read/write 不能只服务 demo 文件，必须处理 offset、EOF 和短读。
- 文件系统 selftest 不能依赖 shell 已经存在。
- 后续可写路径不能要求重写只读 VFS API。

## 验收方式

- kernel 可通过 VFS 打开并读取文件。
- 可列出目录。
- 替换具体 FS 不影响 VFS 调用方。
- 后续能加入可写缓存和 writeback，而不重写 VFS 调用层。
- initramfs/ramfs 和至少一个 VFS selftest 通过同一 open/read/readdir 接口。
- 路径解析错误有明确返回值。
- file offset 行为可测试。
- 多次 open 同一文件能得到独立 file object。
- 后续用户态 `cat/ls` 可以直接复用当前 VFS 接口。

## 当前状态

未开始。

进入本阶段前，`03-memory.md` 应至少有稳定 heap，`04-time-scheduler.md` 应具备锁和等待基础。

## 后续扩展

- page cache。
- block cache。
- writeback。
- 可写 ramfs。
- FAT/ext2 等简单持久化文件系统。
- mount table。
- 权限、时间戳和 inode 编号。
- 设备文件、pipe 和 socket 文件接口。
