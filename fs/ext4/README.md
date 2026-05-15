# ext4

Future ext4 filesystem implementation directory.

This follows the Linux layout where each concrete filesystem owns a directory
under `fs/`, for example `fs/ext4/` and `fs/ramfs/`. Tianole does not implement
ext4 yet; this directory records the intended boundary so VFS work does not grow
ramfs-specific assumptions.

Expected future split:

- `super.c`: superblock probing and mount integration.
- `inode.c`: inode loading and file block mapping.
- `dir.c` / `namei.c`: directory iteration and path component lookup.
- `extents.c`: extent tree walking.
- `ext4.h`: ext4 on-disk structure definitions shared inside this directory.

Journaling, writeback, permissions, timestamps, and block allocation depend on
later block layer and cache work.
