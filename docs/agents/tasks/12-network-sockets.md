# 12 网络栈与 Socket

## 目标

建立独立的网络子系统，让 Tianole 具备 loopback、本地 socket API 和后续真实网卡接入能力，而不是把网络当作某个网卡驱动的附属功能。

## 前置条件

- VFS file object 可承载 socket。
- syscall、用户指针检查和 poll/select 预留可用。
- driver model 至少能接入一种 QEMU 友好的网卡或 loopback 设备。

## 建议边界

- `net/core/`：socket core、sk_buff 或等价 packet buffer、netdevice。
- `net/ipv4/`：IPv4、ARP、ICMP、UDP、TCP。
- `net/unix/`：Unix domain socket。
- `drivers/net/`：virtio-net、e1000 或其他 NIC 驱动。
- `include/uapi/` 或等价目录：socket syscall ABI、常量和结构。

## 实现内容

- socket file operations：socket/bind/listen/accept/connect/send/recv/close。
- loopback netdevice。
- packet buffer 生命周期和引用规则。
- Ethernet/ARP/IPv4/ICMP 基础。
- UDP 先行，TCP 后续。
- Unix domain socket 作为本机 IPC 和服务通信基础。
- 网卡驱动通过 netdevice 注册，不直接服务用户态 socket。

### 1. Socket core

- socket 必须是 file descriptor，复用 VFS close/poll/read/write 或 socket syscall。
- 区分 address family、socket type、protocol 和 socket state。
- 阻塞、非阻塞、超时和错误码要从一开始定义清楚。
- send/recv buffer 需要容量、唤醒和关闭语义。

### 2. Netdevice and packet buffer

- 网卡驱动只注册 netdevice 并收发 frame，不解析用户态 socket。
- packet buffer 要明确 headroom、payload、协议层 ownership 和释放规则。
- RX 路径应从 IRQ/deferred work 进入协议栈，不能在 IRQ 中执行复杂协议处理。
- TX 路径要有队列、完成通知和错误统计。

### 3. Protocol stack

- loopback 是第一目标，便于无真实网卡时验证 socket API。
- IPv4/ARP/ICMP/UDP 是早期闭环；TCP 可独立分阶段实现。
- 路由表、接口地址、MTU、checksum 需要独立结构，不应硬编码单网卡。
- DNS 不属于内核，但内核需要给用户态 resolver 提供 UDP/socket 能力。

### 4. Driver targets

- QEMU 早期优先 virtio-net 或 e1000。
- 网卡 DMA、interrupt moderation、checksum offload 等后续加入，不能污染 net core。
- 驱动错误、丢包、队列满必须可观测。

## Linux 参考原则

- 参考 Linux `net/`、`include/uapi/linux/socket.h`、`drivers/net/`、`net/core/dev.c`。
- socket 是 file descriptor，网络协议栈独立于具体网卡。
- `sk_buff` 思路可简化，但必须有 packet buffer ownership 和层间传递规则。
- network namespace 不是早期目标，但接口不要把全局单网络栈假设写死到所有 API。

## 非玩具化约束

- 不能让用户态直接调用网卡驱动。
- 不能只实现一个 hardcoded ping demo。
- socket 错误码、阻塞、关闭和 poll 语义必须可预测。
- 协议层不能依赖单个设备全局变量。
- 网络栈不能要求 shell 或 kdb 参与数据转发。
- packet buffer 必须有明确释放规则，不能靠静态全局数组掩盖生命周期。

## 验收方式

- loopback UDP send/recv 自测通过。
- 用户态程序可以通过 socket fd 发送和接收数据。
- 至少一个 ICMP echo 或 UDP echo 场景可验证。
- 网卡驱动接入时不修改 socket core API。
- poll/select 能观察 socket 可读/可写状态。

## 当前状态

未开始。

本阶段不阻塞本地 shell/VFS 主线，但不能长期缺失，否则系统无法称为具备现代内核基本子系统。

## 后续扩展

- TCP。
- IPv6。
- DHCP client 用户态支持。
- netlink 或等价网络配置接口。
- firewall/filter hooks。
- TLS 依赖的 crypto 用户态/内核边界。
