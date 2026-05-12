# 03 内存管理

## 目标

从 boot memory map 建立 kernel 自己的内存模型，提供物理页、虚拟地址和内核堆能力。

## 前置条件

- 异常处理可用，尤其是 page fault 路径。
- early log 和 panic 可用。

## 建议边界

- `mm/`：架构无关内存管理。
- `arch/x86/mm/page_table.c`：页表格式、地址空间切换、TLB 操作和 map/unmap/query 主路径。
- `arch/x86/mm/fault.c`：page fault 诊断。
- `arch/x86/mm/page_table.h`：x86 页表子目录私有接口。
- `kernel/selftest/page_table.c`：页表 map/unmap/query 启动自测。
- `include/tianole/`：通用内存接口。

## 实现内容

- 解析 boot memory map。
- 归一化可用/保留/固件/内核占用区域。
- 建立物理页分配器。
- 建立内核虚拟地址布局。
- 建立页表 map/unmap 接口。
- 建立最小内核堆。
- 为后续匿名页、缺页加载、COW、`mmap`、page cache、换页预留接口。

### 1. Memory map 与长期区域模型

- boot memory map 只作为启动输入，不能直接成为长期内核内存类型。
- 建立内核自己的 memory region 类型：usable、reserved、kernel image、boot data、MMIO、ACPI reclaim 等。
- 记录物理地址范围、页对齐、可分配性和后续热插拔/NUMA 预留信息。
- 不写死物理内存大小、页数量或固件返回顺序。

### 2. Physical page allocator

- 当前可以先用 free list，但接口要允许替换为 buddy allocator。
- 每个物理页后续要有 page metadata，记录引用计数、状态、所属用途和链表节点。
- allocator 需要能区分单页、小批量页和连续页需求。
- 后续 DMA、page cache、匿名页、COW、swap 都要能复用同一 page model。

### 3. Virtual memory 与 page table

- arch 页表代码负责页表格式、flag 编码、TLB 操作和地址空间切换。
- 通用 MM 层负责虚拟地址区域、映射策略和对象生命周期。
- 内核地址空间布局必须文档化，用户地址空间和内核地址空间要有硬边界。
- page fault handler 要区分诊断、按需分配、权限错误和用户态进程终止。

### 4. Kernel heap 与小对象分配

- 当前 `kmalloc()` 可以简单，但不能让调用方依赖永不失败或永不释放。
- 小对象分配后续应演进到 slab/slub 或等价缓存分配器。
- heap 元数据不能和 x86 页表实现强耦合。
- 释放路径要能检测基础错误，例如 double free 或明显越界的后续调试接口。

### 5. Future VM capabilities

- 为匿名页、文件页、page cache、COW、`mmap`、换页和内存回收预留结构边界。
- VFS/page cache 不能绕过 MM 直接私有管理所有缓存页。
- 用户态出现前可以先只做 kernel mapping，但接口不能排斥多地址空间。
- 后续进程退出必须能回收地址空间、页表页、匿名页和文件映射引用。

## Linux 参考原则

- 参考 Linux buddy + slab/slub 分层：页级分配和小对象分配分开演进。
- 参考 Linux `struct page` 思路：物理页要有长期 metadata，而不是只有 free list 节点。
- 参考 Linux VMA/page fault 思路：缺页不是单纯 panic，未来要能按区域策略处理。
- 参考 Linux page cache 思路：文件缓存属于 MM/VFS 共同边界，不能被具体文件系统或块驱动私有化。

## 非玩具化约束

- 不写死物理内存大小。
- 不把 UEFI memory type 作为长期内核内存类型。
- allocator 接口要允许后续替换为 buddy/slab。
- 小对象分配要允许后续演进到 slab/slub 或等价缓存分配器。
- 地址空间布局要文档化。
- 用户地址空间和内核地址空间的边界不能依赖调用方自觉遵守。
- 不能把 page cache 和匿名页排除在长期模型之外。
- 页表实现不能混入启动 selftest 或具体业务逻辑。
- 通用 MM 接口不能暴露 x86 页表 flag 编码。
- page fault 诊断和 page fault 策略要分离。
- 内核堆失败路径必须被调用方看见，不能默认分配永远成功。

## 验收方式

- 能分配和释放多个物理页。
- 能建立和删除虚拟映射。
- page fault 能输出有效诊断。
- 内核堆能分配小对象。
- 后续能在此基础上加入 COW、`mmap`、换页和内存回收。
- 页表 selftest 不和页表主实现混在同一文件职责内。
- `kmalloc()`/`kfree()` 有基础复用验证。
- page fault 测试能区分 not-present、权限和访问类型。
- 后续添加用户地址空间时不需要重写物理页 allocator。

## 当前状态

基础完成：

- 已从 boot memory map 扫描 conventional memory。
- 已排除 kernel image、`boot_info` 和 boot memory map 占用页。
- 已建立最小物理页 free-list allocator。
- 已提供 `alloc_page()` 和 `free_page()`。
- 已加入物理页分配/释放 selftest。
- 已切换到内核自有 PML4，不再直接修改固件页表。
- 已提供最小 `map_page()`、`unmap_page()` 和 `virt_to_phys()` 接口。
- 已加入页表 map/unmap/query selftest。
- 已把 x86 页表 selftest 从 `page_table.c` 移到 `kernel/selftest/page_table.c`，避免页表主路径和启动验证逻辑混在同一目录边界。
- 已拆出 x86 page fault 诊断路径，能输出 fault address、错误码、访问类型和权限来源。
- 已建立最小内核堆，提供 `kmalloc()` 和 `kfree()`，底层通过页表按需映射物理页。
- 已加入内核堆分配、写入、释放、复用 selftest。
- `scripts/check.sh` 已验证 `physical pages free=`、物理页 selftest、页表根切换、页表 selftest、内核堆 selftest 和 page fault 日志。

后续扩展：

- 长期内存区域模型，不直接依赖 UEFI memory type。
- page metadata。
- buddy allocator。
- slab/slub 或等价小对象缓存。
- VMA 或等价虚拟区域管理。
- 用户地址空间创建、复制和销毁。
- COW、匿名页和文件页。
- page cache、writeback 和内存回收。
- swap 或等价换页机制。
- DMA 可用内存和 MMIO 映射边界。

验收依据：

- 正常启动日志包含 `physical pages free=`。
- 正常启动日志包含 `physical page allocator selftest ok`。
- 正常启动日志包含 `kernel page table root active`。
- 正常启动日志包含 `page table selftest ok`。
- 正常启动日志包含 `kernel heap initialized`。
- 正常启动日志包含 `kernel heap selftest ok`。
- page fault 测试日志包含 `page fault: address=` 和 `access=write mode=kernel reason=not-present`。

下一阶段：

- `03-memory.md` 的最小基础已经闭环。
- 下一阶段可以进入 timer/scheduler；如果继续打磨内存，则应补 page metadata、buddy allocator 和 slab/slub。
