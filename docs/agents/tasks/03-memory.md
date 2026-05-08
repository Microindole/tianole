# 03 内存管理

## 目标

从 boot memory map 建立 kernel 自己的内存模型，提供物理页、虚拟地址和内核堆能力。

## 前置条件

- 异常处理可用，尤其是 page fault 路径。
- early log 和 panic 可用。

## 建议边界

- `mm/`：架构无关内存管理。
- `arch/x86/mm/`：页表格式、地址空间切换、TLB 操作。
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
- `scripts/check.sh` 已验证 `physical pages free=` 和 selftest 日志。

后续扩展：

- 长期内存区域模型，不直接依赖 UEFI memory type。
- page metadata。
- buddy allocator。
- 页表 map/unmap。
- page fault 专门处理。
- 最小内核堆。
- slab/slub 或等价小对象缓存。

验收依据：

- 正常启动日志包含 `physical pages free=`。
- 正常启动日志包含 `physical page allocator selftest ok`。

下一阶段：

- 继续在 `03-memory.md` 内推进页表管理和内核堆。
