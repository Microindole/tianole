// vfs.c
#include "vfs.h"
#include "common.h"
#include <stddef.h> // for NULL

// 定义一个静态数组来存储所有的文件系统节点
// 这是最简单的内存管理方式，避免了动态内存分配
#define MAX_FS_NODES 256
static fs_node_t fs_nodes[MAX_FS_NODES];
static uint32_t next_free_node = 0; // 指向下一个可用的节点索引

// 全局指针，指向文件系统的根节点和当前工作目录
fs_node_t *fs_root = NULL;
fs_node_t *fs_current = NULL; // 我们稍后会用到这个

// 简单的字符串复制函数
void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

// 初始化虚拟文件系统
void init_vfs() {
    // 1. 从我们的静态数组中为根目录分配一个节点
    fs_node_t* root_node = &fs_nodes[next_free_node++];
    
    // 2. 配置该节点
    strcpy(root_node->name, "/");
    root_node->type = FS_DIRECTORY;
    
    // 3. 初始化它的目录特定数据
    root_node->directory.num_children = 0;
    for (int i = 0; i < MAX_FILES_PER_DIR; i++) {
        root_node->directory.children[i] = NULL;
    }
    
    // 4. 设置全局指针
    fs_root = root_node;
    fs_current = root_node; // 刚启动时，当前目录就是根目录
}

// 外部变量，来自 kernel.c
extern unsigned short* const VIDEO_MEMORY;
extern int cursor_x, cursor_y;

// 实现 ls 命令的函数
void ls_current_dir() {
    // 确保当前目录是一个目录
    if (fs_current->type != FS_DIRECTORY) {
        kprint("\nError: Current path is not a directory.");
        return;
    }

    // 如果目录为空
    if (fs_current->directory.num_children == 0) {
        kprint("\nDirectory is empty.");
        return;
    }

    // 遍历并打印所有子节点的名字
    for (uint32_t i = 0; i < fs_current->directory.num_children; i++) {
        fs_node_t* child = fs_current->directory.children[i];
        kprint("\n");
        kprint(child->name);
        // 如果是目录，在名字后面加上一个斜杠
        if (child->type == FS_DIRECTORY) {
            kputc('/');
        }
    }
}