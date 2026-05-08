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

## 非玩具化约束

- 设备发现、驱动 probe、资源分配要分层。
- MMIO、PIO、IRQ、DMA 访问要有统一接口。
- 音频不是早期目标，不应阻塞内核主线能力。
- 新驱动不能要求修改 shell、VFS、调度器等无关层。
- 硬件发现、资源分配和驱动绑定必须从设备模型进入，不能由单个驱动私自扫描全部硬件。
- ACPI/PCI/Device Tree 类平台信息要隔离在平台层，不能泄漏到通用子系统。

## 验收方式

- 至少一种总线可以枚举设备。
- 至少一种块设备可接入 VFS。
- 新驱动加入时只影响对应驱动和通用驱动框架。
- 后续可接入网络、USB、音频，而不重写已有核心子系统。
