# 09 驱动扩展

## 目标

在基础内核可用后扩展真实设备支持，逐步从 QEMU 环境走向真机。

## 前置条件

- 中断、内存、调度和锁可用。
- 基础输入、输出、存储路径可用。

## 建议边界

- `drivers/`：通用驱动。
- `drivers/pci/`：PCI。
- `drivers/acpi/`：ACPI。
- `drivers/gpu/` 或 `drivers/video/`：显示。
- `drivers/net/`：网络。
- `drivers/sound/`：音频。

## 实现内容

- 建立 bus/device/driver/resource 模型。
- PCI 枚举。
- ACPI 表解析。
- Device Tree 作为非 ACPI 平台的后续入口。
- IRQ routing。
- CPU topology。
- NUMA 拓扑。
- 电源管理。
- 热插拔。
- APIC/HPET 等平台设备。
- virtio、AHCI 或 NVMe 存储。
- framebuffer console。
- 后续再做网络和音频。
- 规划 socket API 和网络栈接入点，但本地 git 阶段不要求网络。

### 1. Device model

- 建立 device、driver、bus、resource 的通用对象模型。
- 设备发现、驱动匹配、probe/remove 和资源释放要分层。
- 驱动不能私自扫描所有硬件或直接改其他子系统内部结构。
- 设备生命周期要有引用和状态，支持 probe 失败后的清理。

### 2. Platform discovery

- ACPI 负责 x86 UEFI/PC 平台的硬件描述、IRQ routing、电源和拓扑信息。
- Device Tree 作为后续非 ACPI 平台入口，不和 ACPI 逻辑混写。
- 平台层解析表，通用驱动层只消费抽象 device/resource。
- 平台信息错误或缺失时要有诊断日志，不能静默使用危险默认值。

### 3. Bus and resource management

- PCI 枚举要生成通用 device，并记录 BAR、IRQ、DMA mask、vendor/device id。
- MMIO、PIO、IRQ、DMA resource 由统一 resource allocator 管理。
- 驱动 probe 前申请资源，remove 或失败时释放资源。
- 后续 USB、virtio、platform device 应能复用 bus/device/driver 模型。

### 4. Interrupt and timer controllers

- PIC 可以作为早期路径，后续 APIC/IOAPIC/MSI/MSI-X 要接入统一 IRQ 层。
- IRQ routing 由平台和中断控制器驱动处理，设备驱动不直接写硬编码 vector。
- HPET/APIC timer 可作为 PIT 的后续替代，不改变通用 timer API。
- spurious IRQ、共享 IRQ 和中断屏蔽策略要有文档。

### 5. Storage, network, graphics and sound

- 存储优先级高于网络和音频，因为 VFS、用户态和工具链依赖它。
- virtio-blk 是 QEMU 友好的早期目标，AHCI/NVMe 是真机存储目标。
- framebuffer console 可改善真机诊断，但不能替代 serial/early log。
- 网络栈和 socket API 需要单独路线，本地 git 目标不要求联网。
- 音频是后期设备，不应阻塞内核主线。

## Linux 参考原则

- 参考 Linux device model：device/driver/bus/resource 分层，probe/remove 管理生命周期。
- 参考 Linux PCI/ACPI 分层：平台发现和具体驱动分开，驱动通过抽象资源访问硬件。
- 参考 Linux irqchip/irqdomain 思路：设备驱动不直接管理 CPU vector。
- 参考 Linux DMA mapping 思路：DMA 地址能力和缓存一致性不能被驱动随意假设。
- 参考 Linux driver core：新驱动应插入框架，不要求修改 VFS、shell、调度器等无关层。

## 非玩具化约束

- 设备发现、驱动 probe、资源分配要分层。
- MMIO、PIO、IRQ、DMA 访问要有统一接口。
- 音频不是早期目标，不应阻塞内核主线能力。
- 新驱动不能要求修改 shell、VFS、调度器等无关层。
- 硬件发现、资源分配和驱动绑定必须从设备模型进入，不能由单个驱动私自扫描全部硬件。
- ACPI/PCI/Device Tree 类平台信息要隔离在平台层，不能泄漏到通用子系统。
- probe 失败必须释放已申请资源。
- 驱动不能长期 busy wait，应使用 IRQ、wait queue 或 deferred work。
- DMA 不能假设所有物理内存都可寻址或缓存一致。
- 平台设备不能把某台机器的 ACPI 表布局写死进通用代码。
- QEMU 专用设备支持不能破坏真机路径。

## 验收方式

- 至少一种总线可以枚举设备。
- 至少一种块设备可接入 VFS。
- 新驱动加入时只影响对应驱动和通用驱动框架。
- 后续可接入网络、USB、音频，而不重写已有核心子系统。
- PCI 枚举能列出 vendor/device id 和 BAR resource。
- 一个驱动 probe 失败路径能被测试或人工触发并正确清理。
- IRQ routing 不要求设备驱动硬编码 vector。
- virtio-blk 或等价块设备通过 block layer 接入 VFS。
- framebuffer console 和 serial log 可以同时存在。

## 当前状态

未开始。

进入本阶段前，需要中断、内存、调度、VFS 和基础用户态足够稳定。

## 后续扩展

- APIC/IOAPIC。
- MSI/MSI-X。
- HPET。
- PCIe capability。
- DMA mapping。
- USB host controller 和 HID/storage。
- virtio-net 和基础网络栈。
- framebuffer/GOP console。
- 音频。
- 电源管理、热插拔、CPU topology、NUMA。
