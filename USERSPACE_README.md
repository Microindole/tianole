# 快速使用指南

## 用户态实现已完成！

现在Tianole OS支持在Ring 3（用户态）运行程序了！

## 快速测试

### 1. 编译所有内容
```bash
# 编译内核
make

# 编译用户程序
cd userland && make && cd ..
```

### 2. 将用户程序写入FAT16镜像
```bash
python3 tools/write_to_fat16.py hdd.img userland/hello.bin "HELLO   BIN"
```

### 3. 运行系统
```bash
make qemu-direct
```

### 4. 在Shell中测试
```bash
tianole> ls
HELLO   BIN

tianole> exec hello.bin
```

## 预期输出

```
========================================
Hello from userspace!
This is a user-mode program running in Ring 3.
========================================
Process 2 exited.
```

## 完整文档

请查看 `USERSPACE_GUIDE.md` 了解详细的实现细节。

## 主要特性

✅ **Ring 3用户态** - 完整的特权级保护
✅ **exec系统调用** - 加载并执行用户程序
✅ **用户态库** - 提供print、exit等API
✅ **内存隔离** - 独立的页表映射
✅ **TSS支持** - 安全的特权级切换

## 文件结构

```
cpu/
  gdt.c, gdt.s, gdt.h         # GDT实现
  switch_to_usermode.s        # 用户态切换
kernel/
  exec.c, exec.h              # exec系统调用
userland/
  hello.c                     # 示例用户程序
  userlib.c, userlib.h        # 用户态库
  syscall.s                   # 系统调用封装
tools/
  write_to_fat16.py           # FAT16写入工具
```

## 编写自己的用户程序

1. 在 `userland/` 创建 `myapp.c`
2. 使用 `userlib.h` 提供的API
3. 编译：`make`（修改Makefile添加新目标）
4. 写入镜像：`python3 ../tools/write_to_fat16.py ../hdd.img myapp.bin "MYAPP   BIN"`
5. 运行：`tianole> exec myapp.bin`

## 示例程序

```c
#include "userlib.h"

int main() {
    print("Hello from my app!\n");
    exit(0);
    return 0;
}
```

完整的构建和测试流程

  1. 清理所有编译产物

  make clean
  这会删除整个 build/ 目录，包括内核和用户程序的所有编译产物。

  2. 编译所有内容（一键完成）

  make all-with-userland
  这个命令会：
  - 编译内核 → build/kernel.bin
  - 编译用户程序 → build/userland/hello.bin
  - 将用户程序写入 hdd.img

  3. 运行测试

  make qemu-direct

  4. 在QEMU中测试

  系统启动后，在Shell中输入：
  tianole> ls
  tianole> exec hello.bin

  ---
  分步操作

  如果你想分步执行：

  # 1. 清理
  make clean

  # 2. 只编译内核
  make

  # 3. 只编译用户程序
  make userland

  # 4. 将用户程序写入镜像
  make install-userland

  # 5. 运行
  make qemu-direct

  ---
  完整的一条龙命令

  make clean && make all-with-userland && make qemu-direct

  ---