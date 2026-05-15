#ifndef TIANOLE_FS_H
#define TIANOLE_FS_H

#include <stddef.h>
#include <stdint.h>

/**
 * VFS_NAME_MAX - Maximum single path component or directory entry name length.
 */
#define VFS_NAME_MAX 64

/**
 * VFS_PATH_MAX - Maximum kernel-owned path length accepted by the early VFS.
 */
#define VFS_PATH_MAX 256

struct vfs_file;
struct vfs_inode;

/**
 * enum vfs_node_type - High-level file object kind exposed by VFS.
 * @VFS_NODE_REGULAR: Byte-addressable regular file.
 * @VFS_NODE_DIRECTORY: Directory that can be enumerated and used for lookup.
 */
enum vfs_node_type {
	VFS_NODE_REGULAR = 1,
	VFS_NODE_DIRECTORY = 2,
};

/**
 * struct vfs_dirent - Directory entry returned by vfs_readdir().
 * @name: NUL-terminated entry name.
 * @type: One of enum vfs_node_type.
 * @size: File size in bytes, or child count for directories.
 */
struct vfs_dirent {
	char name[VFS_NAME_MAX];
	uint16_t type;
	uint64_t size;
};

/**
 * struct vfs_inode_operations - Operations that resolve names from an inode.
 * @lookup: Resolve one child component below a directory inode.
 * @parent: Return the parent directory used by `..` path resolution.
 */
struct vfs_inode_operations {
	int (*lookup)(const struct vfs_inode *dir,
		const char *name,
		size_t name_len,
		const struct vfs_inode **inode);
	int (*parent)(
		const struct vfs_inode *inode, const struct vfs_inode **parent);
};

/**
 * struct vfs_file_operations - Operations that act on an opened file object.
 * @read: Read bytes at and advance file->offset.
 * @readdir: Return the next directory entry and advance file->offset.
 * @close: Optional release hook for an opened file object.
 */
struct vfs_file_operations {
	int (*read)(struct vfs_file *file,
		void *buffer,
		size_t size,
		size_t *read_size);
	int (*readdir)(struct vfs_file *file, struct vfs_dirent *dirent);
	void (*close)(struct vfs_file *file);
};

/**
 * struct vfs_inode - Filesystem-neutral object metadata.
 * @name: Stable debug name for the inode.
 * @type: One of enum vfs_node_type.
 * @size: File size in bytes, or child count for directories.
 * @i_ops: Name lookup operations.
 * @f_ops: Open-file operations.
 * @private_data: Filesystem-owned inode payload.
 */
struct vfs_inode {
	const char *name;
	uint16_t type;
	uint64_t size;
	const struct vfs_inode_operations *i_ops;
	const struct vfs_file_operations *f_ops;
	const void *private_data;
};

/**
 * struct vfs_file - One opened file instance with an independent offset.
 * @inode: Inode opened by this file object.
 * @offset: Current read or directory iteration offset.
 * @f_ops: Cached file operation table for dispatch.
 * @private_data: Filesystem-owned opened-file payload.
 */
struct vfs_file {
	const struct vfs_inode *inode;
	uint64_t offset;
	const struct vfs_file_operations *f_ops;
	const void *private_data;
};

/**
 * vfs_init() - Initialize the VFS core state.
 */
void vfs_init(void);

/**
 * vfs_mount_root() - Install the root inode for absolute path lookup.
 * @root: Directory inode to use as the VFS root.
 *
 * Return: 0 on success, or a negative errno value.
 */
int vfs_mount_root(const struct vfs_inode *root);

/**
 * vfs_open() - Resolve an absolute path into an opened file object.
 * @path: NUL-terminated absolute path.
 * @file: Caller-owned file object filled on success.
 *
 * Return: 0 on success, or a negative errno value.
 */
int vfs_open(const char *path, struct vfs_file *file);

/**
 * vfs_read() - Read from an opened regular file.
 * @file: Opened file object.
 * @buffer: Destination buffer.
 * @size: Maximum bytes to read.
 * @read_size: Bytes actually read, including 0 at EOF.
 *
 * Return: 0 on success, or a negative errno value.
 */
int vfs_read(
	struct vfs_file *file, void *buffer, size_t size, size_t *read_size);

/**
 * vfs_readdir() - Return the next directory entry from an opened directory.
 * @file: Opened directory file object.
 * @dirent: Destination entry.
 *
 * Return: 0 on success, -ENOENT at end of directory, or another negative errno.
 */
int vfs_readdir(struct vfs_file *file, struct vfs_dirent *dirent);

/**
 * vfs_close() - Release an opened file object.
 * @file: Opened file object to close.
 */
void vfs_close(struct vfs_file *file);

/**
 * ramfs_init() - Mount the built-in read-only ramfs as the root filesystem.
 */
void ramfs_init(void);

/**
 * vfs_selftest() - Run boot-time VFS and ramfs regression checks.
 */
void vfs_selftest(void);

#endif
