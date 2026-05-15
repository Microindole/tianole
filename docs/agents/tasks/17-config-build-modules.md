# 17 配置、构建、模块与发布形态

## 目标

让 Tianole 不再只依赖手工 Makefile 和固定内核配置，建立可维护的配置、构建、测试矩阵和后续模块/可选功能边界。

## 前置条件

- 子系统数量增长到需要可选开关。
- QEMU 回归测试和镜像制作已经稳定。
- 基础 driver model 或链接器组织已经成型。

## 建议边界

- `Kconfig` 或等价配置描述。
- `scripts/`：配置、镜像、测试、发布脚本。
- `kernel/module/`：模块加载或静态模块注册的后续边界。
- `include/generated/`：自动生成配置头、syscall 表、keycode 表等。
- `tools/`：宿主机辅助工具。

## 实现内容

- 配置系统：启用/禁用子系统、驱动、调试功能。
- 生成头文件：配置宏、版本、构建信息。
- 测试矩阵：boot、异常、VFS、用户态、网络、SMP 等场景。
- 镜像制作：UEFI FAT、initramfs、rootfs。
- 模块边界：早期可静态链接，但要定义 initcall/driver registration 顺序。
- 发布产物：kernel.elf、EFI image、符号表、日志和测试报告。

### 1. Configuration

- 配置项应有依赖和默认值，不能靠手工改 C 宏。
- debug/test 配置和 production-like 配置要区分。
- 子系统间依赖应体现在配置，而不是编译失败后再猜。

### 2. Build generation

- 自动生成文件要有明确来源，不手工编辑 generated 文件。
- syscall table、keycode table、initcall table 等适合生成的内容应逐步迁移。
- 构建输出应包含版本、git 状态或等价元数据。

### 3. Modules and init ordering

- 可以先不支持运行时加载 `.ko`，但需要静态模块/initcall 模型。
- 驱动注册顺序不能靠 `kernel_main()` 无限增长。
- 后续热插拔和模块卸载要有对象生命周期基础。

### 4. CI and test matrix

- 每个关键子系统要有可单独运行的 check target。
- QEMU 参数组合应覆盖 debugcon、serial、SMP、不同设备。
- 测试日志要能定位失败阶段。

## Linux 参考原则

- 参考 Linux Kconfig/Kbuild、initcall、module 和 generated headers 思路。
- 配置和构建系统是工程规模能力，不是附属脚本。
- 早期静态链接可以接受，但 API 要避免阻塞后续模块化。

## 非玩具化约束

- 不再依赖手工编辑源码打开测试场景。
- 新驱动不应要求手动修改多个无关 Makefile 段落。
- generated 文件必须可复现。
- 测试矩阵不能只保留一个 boot smoke test。
- 发布产物要包含足够调试信息。

## 验收方式

- 能通过配置关闭一个非核心驱动并成功构建。
- 生成配置头被内核使用。
- 至少三个 check target 可独立运行。
- 新增驱动通过静态 initcall 注册，不直接修改主初始化函数主逻辑。
- 构建产物包含符号表或可用于调试的映射信息。

## 当前状态

未开始。

当前 Makefile 足够支撑早期主线，但在网络、SMP、多个驱动并行后必须补本阶段。

## 后续扩展

- 运行时模块加载。
- 包管理或 rootfs 构建。
- 交叉编译工具链封装。
- CI 多平台矩阵。
- ABI 检查。
