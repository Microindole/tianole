# 02 CPU、异常与中断

## 目标

建立 x86_64 CPU 基础设施，让 kernel 能捕获异常并接入后续硬件中断。

## 前置条件

- early log 可在 firmware 退出后稳定输出。
- panic 路径可用。

## 建议边界

- `arch/x86/`：GDT、TSS、IDT、异常入口、IRQ 入口。
- `kernel/`：通用 trap 分发、panic 策略、调试输出。
- `include/tianole/`：必要的通用 trap 数据结构。

## 实现内容

- 建立 x86_64 GDT。
- 建立最小 TSS。
- 建立 IDT。
- 增加异常入口汇编。
- 保存寄存器到统一 trap frame。
- C 层分发 exception。
- 之后再接 PIC/APIC 和 IRQ。
- 异常路径输出异常栈、寄存器现场和符号化所需信息。

### 1. CPU descriptor tables

- GDT/TSS/IDT 初始化集中在 arch 层。
- TSS 至少要为后续 IST、double fault 和用户态栈切换留位置。
- IDT gate 属性必须集中设置，不能由每个 vector 零散指定。
- 后续进入用户态前，要补 ring 3 相关段、syscall/sysret 或 int syscall 的入口边界。

### 2. Trap frame 与入口汇编

- 所有异常和 IRQ 入口都统一构造 `trap_frame`。
- 汇编入口只负责保存现场、补齐 error code 形态、切换到 C handler。
- C handler 不应依赖某个 vector 的临时栈布局。
- trap frame 要保存后续诊断和用户态返回所需的寄存器、vector、error code、rip、rsp、rflags。

### 3. Exception policy

- CPU exception 分发和 panic/oops 策略要分离。
- page fault、invalid opcode、general protection、double fault 等常见异常要逐步拆出专门处理策略。
- 当前 kernel-mode 未处理异常可以 panic；未来 user-mode 异常应终止进程而不是拖垮 kernel。
- 异常日志要输出足够定位的信息，但不能把日志输出当成异常处理策略。

### 4. IRQ dispatch

- 外部 IRQ vector 不能和 CPU exception vector 混用。
- IRQ 分发应通过 handler 注册表，不把具体设备逻辑写进 trap dispatch。
- EOI 时机必须清楚：已处理 IRQ 要发送，未识别 IRQ 的策略要单独记录。
- 后续 APIC、IOAPIC、MSI/MSI-X 要能替换 PIC 路径，而不改通用 IRQ consumer。


### 5. IDT and trap dispatch cleanup

- 当前 `arch/x86/kernel/idt.c` 的 `idt_set_gate(0..32, ...)` 手写列表只是早期过渡实现，不能继续作为后续 IRQ、system vector、syscall 和用户态异常的基础。
- 应尽早改为表驱动：把 vector number、入口符号、是否有 error code、gate 类型、IST 需求和诊断名称集中描述。
- `arch/x86/kernel/traps.c` 不应长期依赖 `if (vector == 14)` 这类零散判断；page fault、general protection、invalid opcode、double fault 等应逐步拆成独立处理函数。
- 参考 Linux `arch/x86/include/asm/idtentry.h` 与 `arch/x86/kernel/traps.c` 的方向：入口类型用宏或统一描述表达，异常策略和通用分发分开。
- 这个重构应放在继续扩展键盘、系统调用、用户态异常之前完成，否则每增加一个入口都会反复修改 `cpu.h`、`exception_entry.S`、`idt.c` 和 `traps.c`。

## Linux 参考原则

- 参考 Linux arch entry 分层：入口汇编保存现场，通用 C 层做分发和策略。
- 参考 Linux trap/irq 区分：exception 是 CPU 同步异常，IRQ 是外部异步事件，两者共享入口框架但语义分开。
- 参考 Linux irqdomain/irqchip 思路：设备使用通用 IRQ 号和注册接口，硬件中断控制器细节留在 arch/driver 层。
- 参考 Linux IST 使用方式：double fault、NMI 等高风险异常需要独立栈，不能依赖可能已经损坏的当前栈。

## 非玩具化约束

- 汇编入口只保存现场和跳转，不写策略。
- C handler 接收统一 trap frame。
- exception 和 IRQ 的入口可共享框架，但语义要区分。
- 不把中断处理写进 `kernel/main.c`。
- oops/panic 策略要和 trap 分发分离。
- IRQ handler 注册、EOI 和调度请求不能互相混在一个设备专用路径里。
- 用户态异常策略要提前预留，不能默认所有异常都 panic。
- arch 层可以知道 vector 和 gate，通用层不应知道 IDT 编码细节。
- double fault 等严重异常不能长期依赖普通内核栈。

## 验收方式

- 主动触发 invalid opcode 或 divide error。
- 日志输出 vector、rip、rsp、error code。
- 未处理异常进入 panic。
- 外部 IRQ vector 和 CPU exception vector 不冲突。
- IRQ handler 可注册和分发，设备逻辑不在 trap dispatch 中硬编码。
- page fault 路径能输出 fault address 和访问来源。
- 后续用户态异常可以在不重写 trap frame 的前提下接入。

## 当前状态

基础完成：

- x86_64 GDT 已加载。
- 最小 TSS 已建立并通过 `ltr` 加载。
- IDT 已建立，当前覆盖 CPU exception vector 0-31，并接入 legacy PIC IRQ0/IRQ1 所需入口。
- 异常入口汇编已统一保存通用寄存器现场。
- C 层 trap dispatch 已接收统一 `trap_frame`。
- 当前 IDT 安装、入口声明和汇编入口已共享 vector 表；vector 表已包含 gate 类型、DPL 和 IST 元数据。
- 未处理异常进入 `panic("unhandled CPU exception")`。
- `KERNEL_TEST_TRAP=1` 会通过 `ud2` 主动触发 invalid opcode。
- `scripts/check.sh` 已自动验证 invalid opcode 日志和 panic 路径。

后续扩展：

- 继续扩展 IDT/trap 元数据，补系统向量、用户态返回策略和更完整的异常恢复策略。
- page fault、double fault 已拆出专门处理函数。
- 下一步必须补 `#UD` invalid opcode 和 `#GP` general protection 的独立 handler；default handler 只作为未知或暂未覆盖 vector 的兜底，不能继续承载常见异常策略。
- PIC/APIC 初始化。
- 外部 IRQ 分发。
- timer interrupt。
- page fault 的专门处理策略。
- double fault 独立 IST 栈已预留，并已有专门 fatal handler 与受控验证路径。
- 用户态异常返回。
- 可恢复异常处理。
- oops 格式和符号化输出。
- syscall/sysret 或 int syscall 入口。
- nested interrupt 和 preempt/reschedule 边界。
- NMI、spurious IRQ 和未知 vector 策略。

验收依据：

- 正常启动日志包含 `traps initialized`。
- invalid opcode 测试能输出 vector、error code、rip、rsp、rflags。
- 未处理异常能进入 `panic("unhandled CPU exception")`。

下一阶段：

- 继续 `02-cpu-interrupts.md`：先补 `#UD` invalid opcode 和 `#GP` general protection 的独立 handler，再推进系统向量、用户态异常返回边界和更完整的异常恢复策略。
