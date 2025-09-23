#include "vfs.h"
#include "common.h"
#include <stddef.h> // for NULL
#include "string.h" // for strcpy
#include "fat16.h"
#include "../mm/kheap.h"

// 全局指针，指向文件系统的根节点和当前工作目录
fs_node_t *fs_root = NULL;
fs_node_t *fs_current = NULL; 

// 初始化虚拟文件系统
void init_vfs() {
    fs_node_t* root_node = (fs_node_t*)kmalloc(sizeof(fs_node_t));

    strcpy(root_node->name, "/");
    root_node->type = FS_DIRECTORY;
    root_node->parent = NULL;
    root_node->directory.num_children = 0;
    for (int i = 0; i < MAX_FILES_PER_DIR; i++) {
        root_node->directory.children[i] = NULL;
    }

    fs_root = root_node;
    fs_current = root_node;
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

// 实现 mkdir 命令的函数
void vfs_mkdir(const char* name) {
    if (fs_current->type != FS_DIRECTORY) {
        kprint("\nError: Current path is not a directory.");
        return;
    }

    if (fs_current->directory.num_children >= MAX_FILES_PER_DIR) {
        kprint("\nError: Directory is full.");
        return;
    }

    // 检查文件名是否已存在
    for (uint32_t i = 0; i < fs_current->directory.num_children; i++) {
        if (strcmp(fs_current->directory.children[i]->name, name) == 0) {
            kprint("\nError: Directory already exists.");
            return;
        }
    }

    // 分配一个新节点
    fs_node_t* new_dir = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    strcpy(new_dir->name, name);
    new_dir->type = FS_DIRECTORY;
    new_dir->parent = fs_current;
    new_dir->directory.num_children = 0;

    for (int i = 0; i < MAX_FILES_PER_DIR; i++) {
        new_dir->directory.children[i] = NULL;
    }

    // 将新节点添加到当前目录的子节点列表中
    fs_current->directory.children[fs_current->directory.num_children] = new_dir;
    fs_current->directory.num_children++;
}

// 实现 touch 命令的函数
void vfs_touch(const char* name) {
    if (fs_current->type != FS_DIRECTORY) {
        kprint("\nError: Current path is not a directory.");
        return;
    }

    if (fs_current->directory.num_children >= MAX_FILES_PER_DIR) {
        kprint("\nError: Directory is full.");
        return;
    }

    // 检查文件名是否已存在
    for (uint32_t i = 0; i < fs_current->directory.num_children; i++) {
        if (strcmp(fs_current->directory.children[i]->name, name) == 0) {
            kprint("\nError: File or directory already exists.");
            return;
        }
    }

    // 分配一个新节点
    fs_node_t* new_file = (fs_node_t*)kmalloc(sizeof(fs_node_t));
    strcpy(new_file->name, name);
    new_file->type = FS_FILE; // 类型是文件
    new_file->file.length = 0; // 文件长度为 0
    new_file->file.content[0] = '\0'; // 内容为空

    // 将新节点添加到当前目录的子节点列表中
    fs_current->directory.children[fs_current->directory.num_children] = new_file;
    fs_current->directory.num_children++;
}

// 实现 cat 命令的函数
void vfs_cat(const char* name) {
    if (fs_current->type != FS_DIRECTORY) {
        kprint("\nError: Current path is not a directory.");
        return;
    }

    // 遍历查找文件
    for (uint32_t i = 0; i < fs_current->directory.num_children; i++) {
        fs_node_t* child = fs_current->directory.children[i];
        // 如果名字匹配
        if (strcmp(child->name, name) == 0) {
            // 如果它是一个文件
            if (child->type == FS_FILE) {
                kprint("\n");
                kprint(child->file.content); // 打印文件内容
            } else {
                kprint("\nError: '");
                kprint(name);
                kprint("' is a directory.");
            }
            return; // 找到后就退出函数
        }
    }

    // 如果循环结束还没找到
    kprint("\nError: File not found.");
}

// 实现 write 命令的函数
void vfs_write(const char* name, const char* content) {
    if (fs_current->type != FS_DIRECTORY) {
        kprint("\nError: Current path is not a directory.");
        return;
    }

    // 检查内容长度是否超出限制
    uint32_t content_len = strlen(content);
    if (content_len >= MAX_FILE_CONTENT_LEN) {
        kprint("\nError: Content is too long.");
        return;
    }

    // 遍历查找文件
    for (uint32_t i = 0; i < fs_current->directory.num_children; i++) {
        fs_node_t* child = fs_current->directory.children[i];
        if (strcmp(child->name, name) == 0) {
            if (child->type == FS_FILE) {
                strcpy(child->file.content, content);
                child->file.length = content_len;
                return; // 写入成功后退出
            } else {
                kprint("\nError: Cannot write to a directory.");
                return;
            }
        }
    }

    kprint("\nError: File not found.");
}

void vfs_cd(const char* name) {
    // 处理 "cd .."
    if (strcmp(name, "..") == 0) {
        if (fs_current->parent != NULL) {
            fs_current = fs_current->parent;
        }
        return;
    }

    // 处理 "cd ."
    if (strcmp(name, ".") == 0) {
        return; // 什么都不做
    }
    
    // 在当前目录的子节点中查找目标目录
    for (uint32_t i = 0; i < fs_current->directory.num_children; i++) {
        fs_node_t* child = fs_current->directory.children[i];
        if (child->type == FS_DIRECTORY && strcmp(child->name, name) == 0) {
            fs_current = child; // 切换当前目录
            return;
        }
    }

    // 如果没找到
    kprint("\nError: Directory not found: ");
    kprint(name);
}

// 实现获取当前路径的函数
void get_current_path(char* path_buffer) {
    // 如果当前就是根目录
    if (fs_current == fs_root) {
        strcpy(path_buffer, "/");
        return;
    }

    path_buffer[0] = '\0';
    fs_node_t* current_node = fs_current;

    // 从当前节点向上回溯，反向构建路径，例如 "dir/sub/"
    while (current_node != fs_root) {
        // strcat 会在末尾追加，所以我们先加名字再加斜杠
        strcat(path_buffer, current_node->name);
        strcat(path_buffer, "/");
        current_node = current_node->parent;
    }

    // 将 "dir/sub/" 反转为 "/bus/rid"
    strrev(path_buffer);

    // 在最前面加上根目录的斜杠
    char temp[256];
    strcpy(temp, "/");
    strcat(temp, path_buffer);
    strcpy(path_buffer, temp);
}

// 实现 append 命令的函数
void vfs_append(const char* name, const char* content) {
    if (fs_current->type != FS_DIRECTORY) {
        kprint("\nError: Current path is not a directory.");
        return;
    }

    // 遍历查找文件
    for (uint32_t i = 0; i < fs_current->directory.num_children; i++) {
        fs_node_t* child = fs_current->directory.children[i];
        if (strcmp(child->name, name) == 0) {
            if (child->type == FS_FILE) {
                // --- 核心区别在这里 ---
                uint32_t content_len = strlen(content);
                // 检查追加后的总长度是否会超出限制
                if (child->file.length + content_len >= MAX_FILE_CONTENT_LEN) {
                    kprint("\nError: Content is too long to append.");
                    return;
                }
                // 使用 strcat 而不是 strcpy 来追加内容
                strcat(child->file.content, content);
                child->file.length += content_len;
                return; // 追加成功后退出
            } else {
                kprint("\nError: Cannot append to a directory.");
                return;
            }
        }
    }

    kprint("\nError: File not found.");
}

void fat16_ls() {
    // 1. 调用我们之前写好的函数，从硬盘读取根目录
    fat16_directory_t* root_dir = fat16_get_root_directory();

    kprint("\n"); // 另起一行

    int entries_found = 0;
    // 2. 遍历所有目录条目
    for (uint32_t i = 0; i < root_dir->entry_count; i++) {
        fat16_directory_entry_t entry = root_dir->entries[i];

        // 3. 检查条目是否有效
        // 文件名的第一个字节是 0x00 表示目录区结束
        if (entry.filename[0] == 0x00) {
            break; 
        }
        // 文件名的第一个字节是 0xE5 表示这是一个已删除的文件
        if ((uint8_t)entry.filename[0] == 0xE5) {
            continue;
        }
        // 我们暂时不显示长文件名(LFN)条目和卷标
        if (entry.attributes == 0x0F || entry.attributes & 0x08) {
            continue;
        }

        entries_found++;

        // 4. 解析并打印 8.3 格式的文件名
        char name_buf[13]; // 8 + 1 + 3 + 1 = 13
        int k = 0;

        // 复制文件名 (最多8个字符)
        for (int j = 0; j < 8; j++) {
            if (entry.filename[j] == ' ') break;
            name_buf[k++] = entry.filename[j];
        }

        // 如果有扩展名，加上点和扩展名 (最多3个字符)
        if (entry.extension[0] != ' ') {
            name_buf[k++] = '.';
            for (int j = 0; j < 3; j++) {
                if (entry.extension[j] == ' ') break;
                name_buf[k++] = entry.extension[j];
            }
        }
        name_buf[k] = '\0'; // 添加字符串结束符

        kprint(name_buf);

        // 5. 如果是目录，在末尾打印一个斜杠
        if (entry.attributes & 0x10) {
            kprint("/");
        }

        kprint("  "); // 打印一些空格作为分隔
    }

    if (entries_found == 0) {
        kprint("Directory is empty.");
    }

    // 6. 释放之前为目录分配的内存，避免内存泄漏
    kfree(root_dir->entries);
    kfree(root_dir);
}