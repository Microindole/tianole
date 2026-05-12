# 10 真机启动

## 目标

在不破坏现有系统启动链的前提下，让 Tianole 在真实 UEFI x86_64 机器上启动。

## 前置条件

- QEMU + OVMF 启动稳定。
- 串口、屏幕或其他早期输出路径可用。
- 内存 map、ExitBootServices、异常处理稳定。

## 建议边界

- `docs/host-machine.md`：本机硬件和启动记录。
- `scripts/`：制作 U 盘镜像和验证脚本。
- `arch/x86/`：UEFI 与平台差异处理。

## 实现内容

- 制作 UEFI FAT 启动盘。
- 记录 OVMF 与真机 UEFI 差异。
- 验证 Secure Boot、启动项、磁盘枚举等风险。
- 先从 U 盘启动，再考虑本机启动项。

### 1. Boot media and rollback

- 先制作可移除 U 盘或镜像，不修改本机现有 EFI 启动项。
- 记录制作步骤、分区布局、FAT 文件路径和 bootloader 文件名。
- 每次真机尝试都要能回退到原系统启动。
- 不在未验证前写入内置磁盘 EFI 分区。

### 2. Hardware inventory

- 在 `docs/host-machine.md` 记录 CPU、内存、UEFI 固件版本、Secure Boot 状态、显卡、存储控制器、键盘路径和可用输出设备。
- 区分 QEMU/OVMF 行为和真机 firmware 行为。
- 记录 ACPI 表、memory map、GOP 模式、串口可用性和启动失败阶段。
- 不把某台机器的硬件常量写入通用代码。

### 3. Diagnostics first

- 真机路径优先保证 early log、panic、异常日志和阶段 marker。
- 如果没有串口，需要 framebuffer console 或屏幕阶段输出作为 fallback。
- 每个启动阶段都输出可识别 marker：bootloader entry、memory map、ExitBootServices、kernel entry、GDT/IDT、heap、timer。
- 失败报告要能判断停在 firmware、bootloader、ExitBootServices、kernel entry、异常或中断阶段。

### 4. Firmware and security differences

- Secure Boot 默认视为风险项，先记录状态，不绕过用户安全设置。
- UEFI memory map、runtime service、GOP、文件路径大小写和磁盘枚举可能和 OVMF 不同。
- 真机测试要优先只读硬件信息，避免早期驱动写危险寄存器。
- 后续 ACPI/PCI 支持进入前，真机功能目标应限于启动和诊断。

## Linux 参考原则

- 参考 Linux bring-up 流程：先保证早期日志和异常诊断，再逐步打开中断、内存、驱动。
- 参考 Linux platform quirk 思路：硬件差异要记录和隔离，不把单机 workaround 扩散到通用路径。
- 参考 Linux boot parameters/firmware handoff 思路：firmware 输入要保存、校验和转换为内核自己的模型。
- 参考 Linux console fallback 思路：serial、early console、framebuffer console 互相补位。

## 非玩具化约束

- 早期不修改本机现有启动链。
- 真机相关信息只放在 `docs/host-machine.md`。
- 真机失败必须能通过日志定位到阶段。
- 不能把某台机器的硬件假设写进通用代码。
- 不在未确认目标磁盘前写入内置磁盘或 EFI 分区。
- Secure Boot 相关操作必须记录风险，不默认要求关闭。
- 真机 workaround 必须有注释和硬件记录，不能伪装成通用逻辑。
- QEMU 测试仍然是回归基线，真机修复不能破坏 OVMF 路径。
- 真机测试脚本应尽量只生成介质，不自动执行危险写盘操作。

## 验收方式

- U 盘可进入 Tianole bootloader。
- kernel 能启动并输出阶段日志。
- 失败时能判断停在 bootloader、ExitBootServices、kernel entry 还是异常路径。
- `docs/host-machine.md` 记录一次完整尝试，包括硬件信息、固件设置、镜像版本和结果。
- OVMF 回归仍通过。
- 真机至少能输出到一种可观察通道：串口、屏幕或持久日志。
- 启动失败时有最后阶段 marker。
- 不修改本机默认启动项也能完成测试。

## 当前状态

未开始。

进入本阶段前，QEMU + OVMF 的早期日志、异常、内存和调度路径应稳定。

## 后续扩展

- 真机串口或 USB debug。
- framebuffer console。
- ACPI 表 dump。
- PCI 枚举对比。
- Secure Boot 签名流程。
- 多机器兼容性记录。
- 自动化镜像制作和校验。
