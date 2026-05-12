# 05 输入事件与键盘

## 目标

建立输入事件模型，让键盘成为输入设备的一种，而不是直接绑定 shell 的特殊逻辑。

## 前置条件

- 中断可用。
- 等待队列或事件队列可用。
- early log 可用于调试。

## 建议边界

- `drivers/input/`：通用输入事件。
- `drivers/input/keyboard/`：键盘设备。
- `arch/x86/`：PS/2 控制器相关 I/O。

## 实现内容

- 接入 PS/2 keyboard。
- 解码 scancode。
- 生成标准 key event。
- 建立输入事件队列。
- 为终端输入提供读取接口。

### 1. Input core

- 建立通用 input device 和 input event 结构，键盘只是 input device 的一种。
- input core 负责设备注册、事件入队、事件读取和等待队列唤醒。
- event 结构要能表达 key press/release、key code、modifier、设备来源和时间戳。
- 事件队列要有容量、溢出策略和统计信息，不能无限增长。

### 2. Keyboard driver

- PS/2 keyboard driver 只负责读取 scancode、维护按键状态、转换为通用 key event。
- scancode set、keyboard layout 和 keymap 要分层，不能把美式键盘布局写死到驱动主路径。
- modifier、repeat、release event、extended scancode 要逐步补齐。
- 键盘驱动不直接写 terminal、shell 或 VFS。

### 3. IRQ 与 deferred processing

- keyboard IRQ handler 只做短路径：读取数据、确认硬件状态、入队原始或半处理事件。
- 复杂解码、repeat 和 terminal line discipline 可以推迟到线程上下文。
- IRQ handler 不能 sleep，不能直接等待队列条件，不能调用 shell。
- 如果需要较复杂处理，先建立 deferred work 或 input worker。

### 4. Terminal input boundary

- terminal/tty 层从 input core 读取 key event，再处理行编辑、回显、控制字符和后续 job control。
- shell 只读取 terminal 字节流或行，不直接读取 keyboard scancode。
- 后续 USB HID、图形终端或串口终端应复用同一 terminal/input 边界。
- 输入路径要保留 blocking read、nonblocking read 和 poll/select 的接口位置。

## Linux 参考原则

- 参考 Linux input subsystem：设备驱动上报通用 input event，上层消费者不绑定具体硬件。
- 参考 Linux serio/i8042/atkbd 分层：控制器、键盘协议和 input 事件上报分开。
- 参考 Linux tty/line discipline 思路：键盘事件到 shell 输入之间应有 terminal 层，不让 shell 直接处理硬件事件。
- 参考 Linux IRQ bottom half/workqueue 思路：中断路径短，复杂处理推迟。

## 非玩具化约束

- 键盘驱动只产生事件，不直接操作 shell。
- 键盘布局要可替换。
- 输入队列接口要允许未来接 USB HID。
- 中断 handler 中不要做复杂处理。
- input core 不依赖 PS/2，PS/2 driver 不依赖 terminal。
- event queue 要有锁和等待语义，不能靠忙等读取。
- 事件丢失、队列满和未知 scancode 必须可观测。
- blocking read 必须通过 wait queue 或等价机制睡眠，不能轮询。
- terminal 控制字符和 shell 命令解析不能写进 keyboard driver。

## 验收方式

- 按键触发中断。
- kernel 能读取 key event。
- 后续 shell 可复用同一输入接口。
- press/release event 可区分。
- unknown scancode 不会破坏输入队列。
- 没有输入时读取方会睡眠，有输入时被唤醒。
- keyboard selftest 或 QEMU 输入测试能验证事件路径。
- 后续接入 shell 时不需要修改 keyboard driver 主逻辑。

## 当前状态

未开始。

进入本阶段前，`04-time-scheduler.md` 至少应具备可靠 wait queue 和明确的可睡眠/不可睡眠上下文规则。

## 后续扩展

- USB HID keyboard。
- 鼠标和相对/绝对坐标事件。
- key repeat。
- 多 keyboard 设备。
- terminal line discipline。
- nonblocking input、poll/select。
- session、foreground process group 和 job control。
