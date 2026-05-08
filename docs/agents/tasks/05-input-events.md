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

## 非玩具化约束

- 键盘驱动只产生事件，不直接操作 shell。
- 键盘布局要可替换。
- 输入队列接口要允许未来接 USB HID。
- 中断 handler 中不要做复杂处理。

## 验收方式

- 按键触发中断。
- kernel 能读取 key event。
- 后续 shell 可复用同一输入接口。

