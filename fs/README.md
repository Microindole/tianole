# fs

文件系统目录。

当前代码分层：

- `fs/vfs.c`：VFS 核心，负责根挂载、路径解析、`open/read/readdir/close` 分发和 file offset 语义。
- `fs/ramfs/`：内置只读 ramfs，用 VFS inode/file operations 接入，不让调用方依赖具体文件系统。
- `fs/ext4/`：后续 ext4 实现目录，先保留 Linux 风格的具体文件系统边界。

后续这里继续放具体文件系统实现，例如 initramfs、procfs 风格虚拟文件系统，以及面向持久化磁盘的 ext4 原型。block layer、page cache、writeback 的共享边界不要放进单个具体文件系统私有实现。
