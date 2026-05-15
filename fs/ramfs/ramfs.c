#include <stddef.h>
#include <stdint.h>

#include <tianole/errno.h>
#include <tianole/fs.h>
#include <tianole/panic.h>

struct ramfs_node;

/*
 * A ramfs directory is a fixed table of names pointing at ramfs nodes.
 * This is deliberately private to ramfs; VFS only sees struct vfs_inode.
 */
struct ramfs_dir_entry {
	const char *name;
	const struct ramfs_node *node;
};

/*
 * In-memory inode backing for the built-in read-only ramfs.
 *
 * The embedded VFS inode is the object exposed upward. The remaining fields are
 * private filesystem state, similar to Linux filesystems embedding VFS objects
 * inside filesystem-specific inode structures.
 */
struct ramfs_node {
	struct vfs_inode inode;
	const uint8_t *data;
	const struct ramfs_dir_entry *children;
	size_t child_count;
	const struct ramfs_node *parent;
};

static const struct ramfs_node *ramfs_node_from_inode(
	const struct vfs_inode *inode);
static int ramfs_lookup(const struct vfs_inode *dir,
	const char *name,
	size_t name_len,
	const struct vfs_inode **inode);
static int ramfs_parent(
	const struct vfs_inode *inode, const struct vfs_inode **parent);
static int ramfs_read(
	struct vfs_file *file, void *buffer, size_t size, size_t *read_size);
static int ramfs_readdir(struct vfs_file *file, struct vfs_dirent *dirent);

static const struct vfs_inode_operations ramfs_dir_iops = {
	.lookup = ramfs_lookup,
	.parent = ramfs_parent,
};

static const struct vfs_file_operations ramfs_file_fops = {
	.read = ramfs_read,
};

static const struct vfs_file_operations ramfs_dir_fops = {
	.readdir = ramfs_readdir,
};

static const uint8_t ramfs_hello_data[] = "hello from ramfs\n";
static const uint8_t ramfs_config_data[] =
	"rootfs=ramfs\nstage=06-storage-vfs\n";

static struct ramfs_node ramfs_hello = {
	.inode =
		{
			.name = "hello.txt",
			.type = VFS_NODE_REGULAR,
			.size = sizeof(ramfs_hello_data) - 1,
			.f_ops = &ramfs_file_fops,
		},
	.data = ramfs_hello_data,
};

static struct ramfs_node ramfs_config = {
	.inode =
		{
			.name = "config.txt",
			.type = VFS_NODE_REGULAR,
			.size = sizeof(ramfs_config_data) - 1,
			.f_ops = &ramfs_file_fops,
		},
	.data = ramfs_config_data,
};

static const struct ramfs_dir_entry ramfs_etc_entries[] = {
	{"config.txt", &ramfs_config},
};

static struct ramfs_node ramfs_etc = {
	.inode =
		{
			.name = "etc",
			.type = VFS_NODE_DIRECTORY,
			.size = 1,
			.i_ops = &ramfs_dir_iops,
			.f_ops = &ramfs_dir_fops,
		},
	.children = ramfs_etc_entries,
	.child_count = 1,
};

static const struct ramfs_dir_entry ramfs_root_entries[] = {
	{"hello.txt", &ramfs_hello},
	{"etc", &ramfs_etc},
};

static struct ramfs_node ramfs_root = {
	.inode =
		{
			.name = "/",
			.type = VFS_NODE_DIRECTORY,
			.size = 2,
			.i_ops = &ramfs_dir_iops,
			.f_ops = &ramfs_dir_fops,
		},
	.children = ramfs_root_entries,
	.child_count = 2,
};

/*
 * Recover ramfs-private state from a VFS inode.
 *
 * The current ramfs explicitly stores private_data during ramfs_init(); later
 * heap-backed inode allocation can replace that without changing VFS callers.
 */
static const struct ramfs_node *ramfs_node_from_inode(
	const struct vfs_inode *inode)
{
	return (const struct ramfs_node *)inode->private_data;
}

/*
 * Return the parent inode for VFS `..` resolution.
 *
 * This is a temporary inode-operation hook until Tianole grows dentry objects
 * that can own parent relationships in the generic VFS layer.
 */
static int ramfs_parent(
	const struct vfs_inode *inode, const struct vfs_inode **parent)
{
	const struct ramfs_node *node = ramfs_node_from_inode(inode);

	if (node == NULL || parent == NULL || node->parent == NULL) {
		return -EINVAL;
	}

	*parent = &node->parent->inode;
	return 0;
}

/*
 * Return the length of a static ramfs name.
 */
static size_t ramfs_strlen(const char *text)
{
	size_t length = 0;

	while (text[length] != '\0') {
		length++;
	}

	return length;
}

/*
 * Compare a static ramfs entry name with a VFS path component.
 *
 * The path component is not NUL-terminated, so the comparison is length based.
 */
static int ramfs_name_equal(
	const char *left, const char *right, size_t right_len)
{
	size_t index;

	if (ramfs_strlen(left) != right_len) {
		return 0;
	}

	for (index = 0; index < right_len; index++) {
		if (left[index] != right[index]) {
			return 0;
		}
	}

	return 1;
}

/*
 * Copy a ramfs entry name into the fixed-size VFS directory entry buffer.
 */
static void ramfs_copy_name(char *dest, const char *src)
{
	size_t index;

	for (index = 0; index < VFS_NAME_MAX - 1 && src[index] != '\0';
		index++) {
		dest[index] = src[index];
	}
	dest[index] = '\0';
}

/*
 * Resolve one child name below a ramfs directory.
 *
 * VFS calls this through struct vfs_inode_operations; callers never depend on
 * ramfs_node or the fixed directory table representation.
 */
static int ramfs_lookup(const struct vfs_inode *dir,
	const char *name,
	size_t name_len,
	const struct vfs_inode **inode)
{
	const struct ramfs_node *node = ramfs_node_from_inode(dir);
	size_t index;

	if (node == NULL || inode == NULL) {
		return -EINVAL;
	}

	for (index = 0; index < node->child_count; index++) {
		const struct ramfs_dir_entry *entry = &node->children[index];

		if (ramfs_name_equal(entry->name, name, name_len)) {
			*inode = &entry->node->inode;
			return 0;
		}
	}

	return -ENOENT;
}

/*
 * Read file bytes from the static ramfs payload and advance file->offset.
 *
 * EOF is reported as success with *read_size == 0, matching the VFS contract
 * used by future syscall wrappers.
 */
static int ramfs_read(
	struct vfs_file *file, void *buffer, size_t size, size_t *read_size)
{
	const struct ramfs_node *node = ramfs_node_from_inode(file->inode);
	uint8_t *bytes = buffer;
	size_t available;
	size_t count;
	size_t index;

	if (node == NULL || node->data == NULL || read_size == NULL) {
		return -EINVAL;
	}

	*read_size = 0;
	if (file->offset >= node->inode.size) {
		return 0;
	}

	available = (size_t)(node->inode.size - file->offset);
	count = size < available ? size : available;

	for (index = 0; index < count; index++) {
		bytes[index] = node->data[file->offset + index];
	}

	file->offset += count;
	*read_size = count;
	return 0;
}

/*
 * Return the next fixed directory-table entry and advance file->offset.
 */
static int ramfs_readdir(struct vfs_file *file, struct vfs_dirent *dirent)
{
	const struct ramfs_node *node = ramfs_node_from_inode(file->inode);
	const struct ramfs_dir_entry *entry;

	if (node == NULL || dirent == NULL) {
		return -EINVAL;
	}

	if (file->offset >= node->child_count) {
		return -ENOENT;
	}

	entry = &node->children[file->offset];
	ramfs_copy_name(dirent->name, entry->name);
	dirent->type = entry->node->inode.type;
	dirent->size = entry->node->inode.size;
	file->offset++;
	return 0;
}

/*
 * Mount the built-in read-only ramfs as the current root filesystem.
 *
 * The private_data and parent pointers are initialized at boot so the static
 * node descriptions stay readable while still satisfying VFS operation hooks.
 */
void ramfs_init(void)
{
	ramfs_root.inode.private_data = &ramfs_root;
	ramfs_hello.inode.private_data = &ramfs_hello;
	ramfs_etc.inode.private_data = &ramfs_etc;
	ramfs_config.inode.private_data = &ramfs_config;
	ramfs_root.parent = &ramfs_root;
	ramfs_hello.parent = &ramfs_root;
	ramfs_etc.parent = &ramfs_root;
	ramfs_config.parent = &ramfs_etc;

	if (vfs_mount_root(&ramfs_root.inode) != 0) {
		panic("ramfs mount root failed");
	}
}
