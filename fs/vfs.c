#include <stddef.h>

#include <tianole/errno.h>
#include <tianole/fs.h>

static const struct vfs_inode *vfs_root;
static int vfs_ready;

/*
 * Reset a caller-owned file object back to the closed state.
 *
 * The early VFS does not allocate struct vfs_file internally yet, but keeping
 * one clearing point avoids stale operation tables when open fails part-way.
 */
static void vfs_clear_file(struct vfs_file *file)
{
	file->inode = NULL;
	file->offset = 0;
	file->f_ops = NULL;
	file->private_data = NULL;
}

/*
 * Return the length of a kernel-owned NUL-terminated string.
 *
 * This avoids depending on libc while the kernel still has no shared string
 * helper library. User pointers are intentionally out of scope for this stage.
 */
static size_t vfs_strlen(const char *text)
{
	size_t length = 0;

	while (text[length] != '\0') {
		length++;
	}

	return length;
}

/*
 * Test whether a path component is the current-directory component.
 */
static int vfs_segment_is_dot(const char *name, size_t name_len)
{
	return name_len == 1 && name[0] == '.';
}

/*
 * Test whether a path component is the parent-directory component.
 */
static int vfs_segment_is_dotdot(const char *name, size_t name_len)
{
	return name_len == 2 && name[0] == '.' && name[1] == '.';
}

/*
 * Dispatch one pathname component through the filesystem inode operations.
 *
 * VFS verifies the generic directory contract first, then calls the concrete
 * filesystem lookup method. This is the seam that lets ext4, ramfs, procfs, and
 * later filesystems share the same path walker.
 */
static int vfs_lookup_child(const struct vfs_inode *dir,
	const char *name,
	size_t name_len,
	const struct vfs_inode **inode)
{
	if (dir->type != VFS_NODE_DIRECTORY) {
		return -ENOTDIR;
	}

	if (dir->i_ops == NULL || dir->i_ops->lookup == NULL) {
		return -ENOTDIR;
	}

	return dir->i_ops->lookup(dir, name, name_len, inode);
}

/*
 * Resolve `..` without hard-coding a concrete filesystem.
 *
 * The mounted root is its own parent for now, matching the usual absolute-root
 * behavior. Non-root directories delegate parent lookup to the backing
 * filesystem until a real dentry cache owns parent relationships.
 */
static int vfs_lookup_parent(
	const struct vfs_inode *inode, const struct vfs_inode **parent)
{
	if (inode->type != VFS_NODE_DIRECTORY) {
		return -ENOTDIR;
	}

	if (inode == vfs_root) {
		*parent = vfs_root;
		return 0;
	}

	if (inode->i_ops == NULL || inode->i_ops->parent == NULL) {
		return -EINVAL;
	}

	return inode->i_ops->parent(inode, parent);
}

/*
 * Initialize global VFS state before any filesystem is mounted.
 */
void vfs_init(void)
{
	vfs_root = NULL;
	vfs_ready = 1;
}

/*
 * Install the root directory used by absolute path lookup.
 *
 * Mount tables and namespaces are future work; this stage keeps one root inode
 * but still routes all operations through generic VFS objects.
 */
int vfs_mount_root(const struct vfs_inode *root)
{
	if (!vfs_ready || root == NULL || root->type != VFS_NODE_DIRECTORY) {
		return -EINVAL;
	}

	vfs_root = root;
	return 0;
}

/*
 * Resolve an absolute path and initialize a caller-owned open file object.
 *
 * The path walker consumes one component at a time, handles repeated slashes,
 * `.` and `..`, and leaves concrete name resolution to inode operations.
 */
int vfs_open(const char *path, struct vfs_file *file)
{
	const struct vfs_inode *current;
	size_t index;
	size_t length;

	if (path == NULL || file == NULL || !vfs_ready || vfs_root == NULL) {
		return -EINVAL;
	}

	vfs_clear_file(file);

	length = vfs_strlen(path);
	if (length == 0 || length >= VFS_PATH_MAX || path[0] != '/') {
		return -EINVAL;
	}

	current = vfs_root;
	index = 1;
	while (index < length) {
		const struct vfs_inode *next;
		const char *name;
		size_t name_len;
		int ret;

		while (path[index] == '/') {
			index++;
		}

		if (index >= length) {
			break;
		}

		name = &path[index];
		name_len = 0;
		while (index < length && path[index] != '/') {
			index++;
			name_len++;
		}

		if (name_len == 0 || name_len >= VFS_NAME_MAX) {
			return -ENAMETOOLONG;
		}

		if (vfs_segment_is_dot(name, name_len)) {
			if (current->type != VFS_NODE_DIRECTORY) {
				return -ENOTDIR;
			}
			continue;
		}

		if (vfs_segment_is_dotdot(name, name_len)) {
			ret = vfs_lookup_parent(current, &next);
			if (ret != 0) {
				return ret;
			}
			current = next;
			continue;
		}

		next = NULL;
		ret = vfs_lookup_child(current, name, name_len, &next);
		if (ret != 0) {
			return ret;
		}

		if (next == NULL) {
			return -ENOENT;
		}
		current = next;
	}

	file->inode = current;
	file->offset = 0;
	file->f_ops = current->f_ops;
	file->private_data = NULL;
	return 0;
}

/*
 * Read bytes from an opened regular file through its file operations.
 *
 * Directories are rejected here so callers get stable errno-like behavior
 * before user-space syscalls exist. Short reads and EOF are filesystem-owned.
 */
int vfs_read(
	struct vfs_file *file, void *buffer, size_t size, size_t *read_size)
{
	if (read_size != NULL) {
		*read_size = 0;
	}

	if (file == NULL || buffer == NULL || read_size == NULL ||
		file->inode == NULL) {
		return -EINVAL;
	}

	if (file->inode->type != VFS_NODE_REGULAR) {
		return -EISDIR;
	}

	if (file->f_ops == NULL || file->f_ops->read == NULL) {
		return -EINVAL;
	}

	return file->f_ops->read(file, buffer, size, read_size);
}

/*
 * Return the next directory entry from an opened directory.
 *
 * Directory iteration state lives in file->offset, which gives each open file
 * object independent enumeration state.
 */
int vfs_readdir(struct vfs_file *file, struct vfs_dirent *dirent)
{
	if (file == NULL || dirent == NULL || file->inode == NULL) {
		return -EINVAL;
	}

	if (file->inode->type != VFS_NODE_DIRECTORY) {
		return -ENOTDIR;
	}

	if (file->f_ops == NULL || file->f_ops->readdir == NULL) {
		return -ENOTDIR;
	}

	return file->f_ops->readdir(file, dirent);
}

/*
 * Close a caller-owned file object and run the optional filesystem hook.
 */
void vfs_close(struct vfs_file *file)
{
	if (file == NULL || file->inode == NULL) {
		return;
	}

	if (file->f_ops != NULL && file->f_ops->close != NULL) {
		file->f_ops->close(file);
	}

	vfs_clear_file(file);
}
