#ifndef USERLIB_H
#define USERLIB_H

// 系统调用封装函数

// 退出进程
void exit(int status);

// 打印字符串（通过写入VGA显存）
void print(const char* str);

// fork系统调用
int fork(void);

// waitpid系统调用
int waitpid(int pid);

// exec系统调用
int exec(const char* filename);

// 简单的字符串处理函数
int strlen(const char* str);
void strcpy(char* dest, const char* src);
int strcmp(const char* s1, const char* s2);

#endif
