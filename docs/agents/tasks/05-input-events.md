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
- `drivers/tty/`：tty/terminal 和早期 line discipline。
- `drivers/video/fbdev/core/`：framebuffer console 后端，参考 Linux fbcon 方向承接字符绘制、滚屏和光标位置等显示细节。
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


### 5. Temporary kernel debug command path

- 当前为了验证真实交互，允许存在一个临时 kernel-side command consumer：从 console line queue 读取完整行并执行少量只读调试命令。
- 这个 consumer 只能视为 early debug/kdb 雏形，不是正式 shell，也不是未来用户态命令解释器。
- 命名和目录应向 Linux 的 `kernel/debug/kdb/` 思路收敛，避免长期保留泛化的 `kernel/debug/` 概念。
- 临时命令只能验证输入链路和只读状态，例如 ticks、drops、echo；不要把 VFS、进程管理、内存修改等系统策略塞进这里。
- 一旦 tty、VFS、syscall 和用户态 init/shell 可用，应让正式 shell 运行在用户态，kernel debug command path 只保留为调试设施。

## Linux 参考原则

- 参考 Linux input subsystem：设备驱动上报通用 input event，上层消费者不绑定具体硬件。
- 参考 Linux serio/i8042/atkbd 分层：控制器、键盘协议和 input 事件上报分开。
- 参考 Linux `include/uapi/linux/input-event-codes.h`：key code 是稳定的输入身份命名空间，不应是随当前原型增删而改变的临时 enum。
- 参考 Linux `drivers/input/keyboard/atkbd.c`：AT/PS2 scancode 到 keycode 使用表驱动映射，驱动主路径不靠不断扩大的 switch 表达标准键盘。
- 参考 Linux `drivers/tty/vt/keyboard.c` 与 `drivers/tty/vt/defkeymap.c_shipped`：keycode 到字符/功能键语义属于 tty/vt/keymap 层，不属于硬件键盘驱动。
- 参考 Linux keymap 生成方式：默认 keymap 可以内置一份，但后续应从可替换的 keymap/Unicode/diacritic 表生成或加载；keymap 应输出 keysym/Unicode/动作语义，而不是把所有字符语义散落写死到驱动里。
- 参考 Linux `drivers/tty/vt/vt.c` 与 `drivers/video/fbdev/core/fbcon.c`：virtual terminal 维护屏幕字符缓冲、光标、滚屏和转义序列，fbcon 只作为显示后端实现字符绘制和 framebuffer 更新。
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

进行中：

- 已有全局 input event queue，支持 blocking/nonblocking read 和 dropped event 统计。
- 已接入 PS/2 keyboard IRQ1，当前能把 set-1 scancode 转成通用 key event。
- 已拆分 keyboard layout/keymap：`include/tianole/input-event-codes.h` 提供对齐 Linux 6.8 `KEY_*` 数值的完整 keycode 命名空间；PS/2 set-1 与 0xe0 extended scancode 使用表驱动映射到通用 key code；默认 US keymap 放在 `drivers/tty/keymap.c`，输出 `struct tty_keysym`，当前支持 Unicode keysym 与 F1-F12/方向键/导航键 function-string keysym。
- 已新增 `include/tianole/keysym.h`：keycode 到字符之间存在 keysym 中间层，tty line discipline 再把 Unicode keysym 编码为 UTF-8 字节，并把 function keysym 解析为默认终端功能键字节序列；后续 dead key、AltGr 和外部 keymap 加载应在这一层扩展。
- 已有标准键盘骨架：F1-F12、左右 Ctrl/Alt、CapsLock/NumLock/ScrollLock、方向键、Home/End/PageUp/PageDown、Insert/Delete、keypad 键已有 key identity；F1-F12、方向键和常见导航键已通过 tty function-string 层输出 Linux 默认 keymap 风格的 ESC 序列，modifier-only/keypad 等仍只保留 key identity。
- 已有 boot-time input selftest，覆盖小写、Shift 大写、CapsLock 大写、Shift+CapsLock 小写、标点 Shift 变体，以及 F1/方向键/Delete 的 function-string 映射。
- 早期 framebuffer console 已开始按 Linux fbcon 方向从 `arch/x86/kernel/screen.c` 拆出：x86 只负责 boot framebuffer handoff，字符绘制、滚屏和小写字体在 `drivers/video/fbdev/core/`。
- 已有 deferred work 路径，键盘 IRQ 不直接执行复杂解码和上层命令逻辑。
- 已有临时 input console line queue，支持回显、退格、回车提交、blocking line read 和 dropped line 统计。
- 已新增 `drivers/tty/` 早期 tty line discipline，line queue、回显和读行接口开始从 `input_console` 迁出。
- `kdb` 交互输入/输出已改走 `tty_read_line()`/`tty_write*()`，不再通过 `console_read_line()` 兼容层或直接依赖 `early_log`；初始化状态继续使用 `pr_info()`。
- 已有临时 kernel kdb/debug command consumer，用于验证真实输入链路。

当前可以回到本阶段继续推进。`02-cpu-interrupts.md` 已经补上
exception/IRQ/syscall/system vector 分层、kernel/user trap 来源判断、
future user exception policy 边界、`rsp/ss` trap-frame 预留，以及
IRQ/syscall/user-exception 共用的 trap-exit 返回边界。输入主线
接下来应把临时 input console 收敛为 tty/terminal 雏形，而不是继续扩展 kdb。

限制必须明确：当前 kdb 不是 Linux 意义上的 shell/tty，也不是用户态入口；它只是 early debug/kdb 雏形，后续要改名并收敛到 `kernel/debug/` 类边界。

## 接手入口

下一位接手者应从这些文件开始读：

- `drivers/input/input.c`、`include/tianole/input.h`：通用 input event queue。
- `include/tianole/input-event-codes.h`：对齐 Linux `KEY_*` 的稳定 keycode 命名空间。
- `include/tianole/keysym.h`：tty keymap 输出的 keysym/Unicode 中间表示。
- `drivers/input/keyboard/ps2.c`、`include/tianole/keyboard.h`：PS/2 set-1 scancode 到 key event 的表驱动转换。
- `drivers/tty/tty.c`、`include/tianole/tty.h`：早期 tty line discipline、UTF-8 编码、function-string 解析、回显、blocking line read。
- `drivers/tty/keymap.c`：当前 US keymap，把通用 key code 转成 tty keysym。
- `drivers/video/fbdev/core/fbcon.c`、`drivers/video/fbdev/core/font.c`、`include/tianole/fbcon.h`：早期 framebuffer console 后端。
- `kernel/console/input_console.c`、`include/tianole/console.h`：当前 input event 到 tty 字符流的临时桥接。
- `kernel/debug/kdb.c`、`include/tianole/kdb.h`：临时 early debug command consumer，只用于验证输入链路。
- `kernel/workqueue.c`、`include/tianole/workqueue.h`：键盘 deferred processing 依赖的线程上下文。

下一步只做 tty/terminal 雏形：

- 继续完善 `drivers/tty/` 边界，承接更多 line discipline、控制字符和读写接口。
- 让 `input_console` 只保留 input event 到 tty 字符流的临时桥接职责。
- 保持 keyboard driver 只上报 input event，不直接感知 terminal、shell 或 kdb。
- 保持 kdb 只作为 terminal/console 的临时 consumer，不继续增加系统策略命令。

建议验收：

- `make clean all`
- `scripts/check.sh boot`
- `scripts/check.sh user-exception invalid-opcode general-protection page-fault`
- 真实 QEMU 交互中确认键盘输入、回显、退格和回车提交仍可用。
- `make run-interactive` 启动图形 QEMU 后可测试常见 US 可打印键：字母、数字、空格、Tab、退格、回车、`-=[]\;',./` 及其 Shift/CapsLock 变体。

## 后续扩展

- USB HID keyboard。
- 鼠标和相对/绝对坐标事件。
- key repeat。
- 多 keyboard 设备。
- terminal line discipline。
- nonblocking input、poll/select。
- session、foreground process group 和 job control。
