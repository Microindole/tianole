#include <stddef.h>

#include <tianole/errno.h>
#include <tianole/fs.h>
#include <tianole/panic.h>
#include <tianole/printk.h>

/*
 * Fail the boot immediately when a VFS invariant is broken.
 */
static void fs_selftest_expect(int condition, const char *message)
{
	if (!condition) {
		panic(message);
	}
}

/*
 * Compare raw bytes read from a VFS file.
 */
static int fs_selftest_mem_equal(
	const char *left, const char *right, size_t length)
{
	size_t index;

	for (index = 0; index < length; index++) {
		if (left[index] != right[index]) {
			return 0;
		}
	}

	return 1;
}

/*
 * Compare NUL-terminated directory entry names in selftests.
 */
static int fs_selftest_name_equal(const char *left, const char *right)
{
	size_t index = 0;

	while (left[index] != '\0' && right[index] != '\0') {
		if (left[index] != right[index]) {
			return 0;
		}
		index++;
	}

	return left[index] == right[index];
}

/*
 * Verify regular-file reads, EOF behavior, and per-open file offsets.
 */
static void fs_selftest_read_file(void)
{
	struct vfs_file first;
	struct vfs_file second;
	char buffer[32];
	size_t read_size;

	fs_selftest_expect(vfs_open("/hello.txt", &first) == 0,
		"vfs selftest open hello failed");
	fs_selftest_expect(vfs_open("/hello.txt", &second) == 0,
		"vfs selftest second open hello failed");

	fs_selftest_expect(vfs_read(&first, buffer, 5, &read_size) == 0,
		"vfs selftest first read failed");
	fs_selftest_expect(
		read_size == 5, "vfs selftest first short count failed");
	fs_selftest_expect(fs_selftest_mem_equal(buffer, "hello", 5),
		"vfs selftest first data failed");

	fs_selftest_expect(vfs_read(&second, buffer, 5, &read_size) == 0,
		"vfs selftest second read failed");
	fs_selftest_expect(read_size == 5, "vfs selftest second count failed");
	fs_selftest_expect(fs_selftest_mem_equal(buffer, "hello", 5),
		"vfs selftest second independent offset failed");

	fs_selftest_expect(
		vfs_read(&first, buffer, sizeof(buffer), &read_size) == 0,
		"vfs selftest tail read failed");
	fs_selftest_expect(read_size == 12, "vfs selftest tail count failed");

	fs_selftest_expect(
		vfs_read(&first, buffer, sizeof(buffer), &read_size) == 0,
		"vfs selftest eof read failed");
	fs_selftest_expect(read_size == 0, "vfs selftest eof count failed");

	vfs_close(&first);
	vfs_close(&second);
}

/*
 * Verify nested path lookup and `..` handling through VFS.
 */
static void fs_selftest_read_nested_file(void)
{
	struct vfs_file file;
	char buffer[16];
	size_t read_size;

	fs_selftest_expect(vfs_open("/etc/../etc/config.txt", &file) == 0,
		"vfs selftest open nested file failed");
	fs_selftest_expect(vfs_read(&file, buffer, 7, &read_size) == 0,
		"vfs selftest nested read failed");
	fs_selftest_expect(read_size == 7, "vfs selftest nested count failed");
	fs_selftest_expect(fs_selftest_mem_equal(buffer, "rootfs=", 7),
		"vfs selftest nested data failed");
	vfs_close(&file);
}

/*
 * Verify directory iteration and directory read rejection policy.
 */
static void fs_selftest_readdir_root(void)
{
	struct vfs_file root;
	struct vfs_dirent dirent;
	size_t read_size;
	int saw_etc = 0;
	int saw_hello = 0;
	int ret;

	fs_selftest_expect(
		vfs_open("/", &root) == 0, "vfs selftest open root failed");
	fs_selftest_expect(
		vfs_read(&root, &dirent, sizeof(dirent), &read_size) == -EISDIR,
		"vfs selftest read directory policy failed");

	while ((ret = vfs_readdir(&root, &dirent)) == 0) {
		if (fs_selftest_name_equal(dirent.name, "hello.txt")) {
			saw_hello = dirent.type == VFS_NODE_REGULAR;
		}
		if (fs_selftest_name_equal(dirent.name, "etc")) {
			saw_etc = dirent.type == VFS_NODE_DIRECTORY;
		}
	}

	fs_selftest_expect(ret == -ENOENT, "vfs selftest readdir eof failed");
	fs_selftest_expect(saw_hello, "vfs selftest missing hello dirent");
	fs_selftest_expect(saw_etc, "vfs selftest missing etc dirent");
	vfs_close(&root);
}

/*
 * Verify errno-like failures for missing paths and wrong file types.
 */
static void fs_selftest_errors(void)
{
	struct vfs_file file;
	struct vfs_dirent dirent;

	fs_selftest_expect(vfs_open("/missing.txt", &file) == -ENOENT,
		"vfs selftest missing file policy failed");
	fs_selftest_expect(vfs_open("/hello.txt", &file) == 0,
		"vfs selftest open file for readdir failed");
	fs_selftest_expect(vfs_readdir(&file, &dirent) == -ENOTDIR,
		"vfs selftest readdir file policy failed");
	vfs_close(&file);
}

/*
 * Run VFS regression checks during boot before interactive input starts.
 */
void vfs_selftest(void)
{
	fs_selftest_read_file();
	fs_selftest_read_nested_file();
	fs_selftest_readdir_root();
	fs_selftest_errors();
	pr_info("vfs selftest ok\n");
}
