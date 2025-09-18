// vfs.h
#ifndef VFS_H
#define VFS_H

#include "common.h"

#define MAX_FILENAME_LEN 32
#define MAX_FILES_PER_DIR 16 // 每个目录最多包含的文件/子目录数
#define MAX_FILE_CONTENT_LEN 1024 // 每个文件的最大内容长度

// 定义节点类型：文件 或 目录
typedef enum {
    FS_FILE,
    FS_DIRECTORY
} FS_NODE_TYPE;

// 前向声明，因为 directory_t 中会引用 fs_node_t
struct fs_node;

// 目录类型结构体
typedef struct {
    struct fs_node* children[MAX_FILES_PER_DIR]; // 指向子节点的指针数组
    uint32_t num_children; // 当前子节点的数量
} directory_t;

// 文件类型结构体
typedef struct {
    char content[MAX_FILE_CONTENT_LEN];
    uint32_t length;
} file_t;

// 统一的文件系统节点 (inode) 结构体
typedef struct fs_node {
    char name[MAX_FILENAME_LEN];
    FS_NODE_TYPE type;
    union { // 使用 union 来节省空间，因为一个节点要么是文件要么是目录
        file_t file;
        directory_t directory;
    };
} fs_node_t;


// 声明文件系统初始化函数
void init_vfs();

// 声明列出当前目录内容的函数
void ls_current_dir();

// 声明创建目录的函数
void vfs_mkdir(const char* name);

// 声明创建文件的函数
void vfs_touch(const char* name);

#endif