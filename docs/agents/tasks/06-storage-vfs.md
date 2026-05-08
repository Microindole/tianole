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
- `kernel/mm/`：页缓存或块缓存。
- `fs/`：具体文件系统实现。

## 实现内容

- 建立 block device 接口。
- 初期支持 initramfs/ramdisk。
- 建立 inode、dentry、file 等 VFS 对象。
- 建立路径解析。
- 支持 open/read/close/readdir。
- 后续再加入 write 和持久化文件系统。
- 为 page cache、block cache、writeback 和崩溃一致性预留边界。

## 非玩具化约束

- VFS 不依赖某个具体文件系统。
- 文件系统不直接依赖 shell 或进程加载器。
- 路径解析和引用计数要从一开始有边界。
- 缓存层不能和具体磁盘驱动耦合。
- VFS 对象生命周期必须明确，不能靠全局静态对象规避释放问题。
- 文件权限、时间戳、inode 编号等 POSIX 语义要保留位置。

## 验收方式

- kernel 可通过 VFS 打开并读取文件。
- 可列出目录。
- 替换具体 FS 不影响 VFS 调用方。
- 后续能加入可写缓存和 writeback，而不重写 VFS 调用层。
