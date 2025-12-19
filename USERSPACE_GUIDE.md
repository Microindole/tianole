# Tianole OS - 用户态实现完整指南

## 概述

本文档描述了为Tianole OS添加用户态支持的完整实现。现在操作系统支持在Ring 3（用户态）运行程序，实现了真正的进程隔离和特权级保护。

---

## 实现内容

### 1. GDT扩展（Ring 0 & Ring 3段）

**新增文件：**
- `cpu/gdt.c` - GDT管理实现
- `cpu/gdt.s` - GDT加载汇编代码
- `include/cpu/gdt.h` - GDT头文件

**功能：**
- 0x00: 空描述符
- 0x08: 内核代码段 (Ring 0, DPL=0)
- 0x10: 内核数据段 (Ring 0, DPL=0)
- 0x18: 用户代码段 (Ring 3, DPL=3)
- 0x20: 用户数据段 (Ring 3, DPL=3)
- 0x28: TSS段（用于特权级切换）

**TSS作用：**
当从Ring 3触发中断/系统调用时，CPU自动从TSS读取内核栈地址(esp0)，实现安全的特权级切换。

---

### 2. 特权级切换机制

**新增文件：**
- `cpu/switch_to_usermode.s` - 用户态切换汇编
- `include/cpu/usermode.h` - 用户态切换接口

**实现原理：**
使用 `iret` 指令模拟中断返回：
```asm
push 0x23          ; SS (用户数据段)
push user_stack    ; ESP (用户栈)
pushf              ; EFLAGS
pop ecx
or ecx, 0x200      ; 开启中断
push ecx
push 0x1B          ; CS (用户代码段)
push entry_point   ; EIP (用户程序入口)
iret               ; 切换到Ring 3
```

---

### 3. exec系统调用

**新增文件：**
- `kernel/exec.c` - exec实现
- `kernel/exec.h` - exec接口

**功能：**
1. 从FAT16文件系统读取可执行文件
2. 为用户程序分配独立的内存页（用户态可访问）
3. 将程序代码复制到0x08048000（类似Linux）
4. 分配用户栈（0xC0000000 - 4KB）
5. 设置TSS中的内核栈地址
6. 切换到用户态并执行

**系统调用号：** 3
**参数：** EBX = 文件名字符串指针

---

### 4. 用户态库（userlib）

**目录：** `userland/`

**文件：**
- `userlib.h` - 用户态库接口
- `userlib.c` - 库实现（print、字符串函数等）
- `syscall.s` - 系统调用汇编封装

**提供的API：**
```c
void exit(int status);        // 退出进程
void print(const char* str);  // 打印字符串（直接写VGA显存）
int fork(void);               // 创建子进程
int waitpid(int pid);         // 等待子进程
int exec(const char* filename); // 执行程序
int strlen(const char* str);  // 字符串长度
void strcpy(char* dest, const char* src); // 字符串复制
int strcmp(const char* s1, const char* s2); // 字符串比较
```

---

### 5. Shell命令扩展

**修改文件：** `kernel/shell.c`

**新增命令：**
```bash
exec <filename>   # 执行用户态程序
```

**示例：**
```bash
tianole> ls
HELLO   BIN

tianole> exec hello.bin
Executing user program: hello.bin
Switching to user mode, entry point: 0x08048000
========================================
Hello from userspace!
This is a user-mode program running in Ring 3.
========================================
Process 2 exited.
```

---

### 6. 构建系统

**用户程序Makefile：** `userland/Makefile`

**编译选项：**
```makefile
CFLAGS = -m32 -nostdlib -nostdinc -fno-builtin -fno-stack-protector \
         -ffreestanding -c -O0 -I.

LDFLAGS = -m elf_i386 -Ttext=0x08048000 -e main
```

**关键点：**
- `-Ttext=0x08048000`: 代码段加载地址（必须与内核一致）
- `-e main`: 入口点为main函数
- `-nostdlib`: 不链接标准库
- `-nostdinc`: 不包含标准头文件

---

### 7. 工具脚本

**文件：** `tools/write_to_fat16.py`

**功能：** 将编译好的用户程序写入FAT16镜像

**用法：**
```bash
python3 tools/write_to_fat16.py hdd.img userland/hello.bin "HELLO   BIN"
```

---

## 内存布局

### 用户态地址空间
```
0x00000000 - 0x08048000: 保留
0x08048000 - 0xBFFFFFFF: 用户程序区域
  0x08048000: 程序代码段起始地址
  ...
  0xC0000000 - 4KB: 用户栈（向下增长）

0xC0000000 - 0xFFFFFFFF: 内核空间（3GB-4GB）
```

### 页表映射
- 每个进程有独立的页目录
- 用户区：用户程序和栈（DPL=3，用户可访问）
- 内核区：内核代码和数据（DPL=0，所有进程共享）

---

## 系统调用流程

```
用户程序:              内核:
mov eax, 3
mov ebx, filename
int 0x80  --------> 保存用户态上下文（SS, ESP, EFLAGS, CS, EIP）
                    从TSS读取esp0，切换到内核栈
                    调用syscall_dispatcher()
                    根据EAX调用syscall_exec()

                    执行exec:
                    1. 读取文件
                    2. 分配用户内存
                    3. 映射页表
                    4. 切换到用户态

      <------------ iret返回（不会返回，直接跳转到新程序）

新程序入口:
0x08048000:         用户程序开始执行（Ring 3）
```

---

## 编译和运行

### 1. 编译内核
```bash
cd /home/indole/my-linux
make clean
make
```

### 2. 编译用户程序
```bash
cd userland
make
```

### 3. 将用户程序写入FAT16镜像
```bash
python3 ../tools/write_to_fat16.py ../hdd.img hello.bin "HELLO   BIN"
```

### 4. 运行QEMU
```bash
cd ..
make qemu-direct
```

### 5. 在Shell中测试
```bash
tianole> ls
HELLO   BIN

tianole> exec hello.bin
```

---

## 示例用户程序

### hello.c
```c
#include "userlib.h"

int main() {
    print("========================================\n");
    print("Hello from userspace!\n");
    print("This is a user-mode program running in Ring 3.\n");
    print("========================================\n");

    exit(0);
    return 0;
}
```

---

## 关键技术点

### 1. **特权级保护**
- 内核运行在Ring 0，拥有完全权限
- 用户程序运行在Ring 3，受限访问
- 段描述符的DPL字段控制访问权限

### 2. **TSS的作用**
- 存储内核栈地址（esp0）
- CPU在特权级切换时自动使用TSS中的栈
- 防止用户栈被内核代码使用，保证安全性

### 3. **系统调用约定**
- 使用 `int 0x80` 触发
- EAX: 系统调用号
- EBX, ECX, EDX, ESI, EDI: 参数1-5
- 返回值在EAX中

### 4. **内存隔离**
- 每个进程有独立的页目录
- 用户态内存页标记为DPL=3（用户可访问）
- 内核内存页标记为DPL=0（仅内核可访问）

---

## 文件格式（当前实现）

### Flat Binary
- 最简单的可执行文件格式
- 文件内容就是纯机器码
- 直接加载到固定地址（0x08048000）
- 无需解析文件头

### 未来扩展：ELF32
可以实现ELF32加载器以支持：
- 多个段（.text, .data, .bss）
- 动态链接
- 符号表和调试信息

---

## 调试技巧

### 1. 使用串口输出
内核通过串口输出调试信息（`serial_print()`），在QEMU中查看：
```bash
make qemu-direct  # 串口输出重定向到stdio
```

### 2. 检查页面映射
在exec.c中添加调试输出，查看页表映射是否正确：
```c
kprint("Mapped virtual 0x...\n");
```

### 3. 验证用户程序加载
确认文件成功写入FAT16：
```bash
tianole> ls
HELLO   BIN
```

### 4. 观察进程状态
```bash
tianole> ps
```

---

## 已知限制

1. **文件格式：** 仅支持flat binary，不支持ELF
2. **系统调用：** 数量有限（exit, fork, waitpid, exec）
3. **标准库：** 用户态库功能简单，仅实现基本函数
4. **多进程：** exec会替换当前进程，需要fork+exec模式
5. **错误处理：** 基础的错误处理，需要完善

---

## 后续扩展建议

### 阶段8：完善系统调用
- `sys_write(fd, buf, len)` - 写文件/终端
- `sys_read(fd, buf, len)` - 读文件/键盘
- `sys_open(path, flags)` - 打开文件
- `sys_close(fd)` - 关闭文件
- `sys_brk(addr)` - 动态内存管理

### 阶段9：ELF32加载器
- 解析ELF头部
- 加载Program Headers
- 支持多个段（.text, .data, .bss）
- 处理入口点

### 阶段10：用户态标准库
- 实现 `printf()` 格式化输出
- 完整的字符串函数库
- 基于brk的 `malloc()/free()`
- 文件操作封装

### 阶段11：进程管理增强
- 实现 `fork() + exec()` 模式
- 进程间通信（管道、信号）
- 进程优先级和调度策略

---

## 技术参考

### Intel x86架构
- **特权级：** Ring 0-3，数字越小权限越高
- **段描述符：** 控制代码/数据段的访问权限
- **TSS：** Task State Segment，用于任务切换和特权级切换

### Linux相似性
- **加载地址：** 0x08048000（与32位Linux ELF默认地址相同）
- **系统调用：** int 0x80（与旧版Linux相同）
- **地址空间：** 3GB用户空间 + 1GB内核空间

---

## 总结

现在Tianole OS已经具备完整的用户态支持：

✅ GDT配置完成（Ring 0 + Ring 3）
✅ TSS实现特权级切换
✅ exec系统调用加载用户程序
✅ 用户态库提供基本API
✅ Shell支持exec命令
✅ 内存隔离和页表映射
✅ 完整的构建和测试流程

用户程序现在真正运行在Ring 3，享受操作系统的保护和隔离！

---

## 快速开始

```bash
# 1. 编译所有组件
make clean && make
cd userland && make && cd ..

# 2. 将用户程序写入镜像
python3 tools/write_to_fat16.py hdd.img userland/hello.bin "HELLO   BIN"

# 3. 运行
make qemu-direct

# 4. 在Shell中执行
tianole> exec hello.bin
```

---

生成时间：2025-12-19
Tianole OS版本：1.0 (with userspace support)
