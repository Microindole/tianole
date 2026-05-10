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

## 非玩具化约束

- 不写死物理内存大小。
- 不把 UEFI memory type 作为长期内核内存类型。
- allocator 接口要允许后续替换为 buddy/slab。
- 小对象分配要允许后续演进到 slab/slub 或等价缓存分配器。
- 地址空间布局要文档化。
- 用户地址空间和内核地址空间的边界不能依赖调用方自觉遵守。
- 不能把 page cache 和匿名页排除在长期模型之外。

## 验收方式

- 能分配和释放多个物理页。
- 能建立和删除虚拟映射。
- page fault 能输出有效诊断。
- 内核堆能分配小对象。
- 后续能在此基础上加入 COW、`mmap`、换页和内存回收。

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
