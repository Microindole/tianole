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

## 非玩具化约束

- 早期不修改本机现有启动链。
- 真机相关信息只放在 `docs/host-machine.md`。
- 真机失败必须能通过日志定位到阶段。
- 不能把某台机器的硬件假设写进通用代码。

## 验收方式

- U 盘可进入 Tianole bootloader。
- kernel 能启动并输出阶段日志。
- 失败时能判断停在 bootloader、ExitBootServices、kernel entry 还是异常路径。

