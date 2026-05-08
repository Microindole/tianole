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

## 非玩具化约束

- 汇编入口只保存现场和跳转，不写策略。
- C handler 接收统一 trap frame。
- exception 和 IRQ 的入口可共享框架，但语义要区分。
- 不把中断处理写进 `kernel/main.c`。
- oops/panic 策略要和 trap 分发分离。

## 验收方式

- 主动触发 invalid opcode 或 divide error。
- 日志输出 vector、rip、rsp、error code。
- 未处理异常进入 panic。

## 当前状态

基础完成：

- x86_64 GDT 已加载。
- 最小 TSS 已建立并通过 `ltr` 加载。
- IDT 已建立，当前覆盖 CPU exception vector 0-31。
- 异常入口汇编已统一保存通用寄存器现场。
- C 层 trap dispatch 已接收统一 `trap_frame`。
- 未处理异常进入 `panic("unhandled CPU exception")`。
- `KERNEL_TEST_TRAP=1` 会通过 `ud2` 主动触发 invalid opcode。
- `scripts/check.sh` 已自动验证 invalid opcode 日志和 panic 路径。

后续扩展：

- PIC/APIC 初始化。
- 外部 IRQ 分发。
- timer interrupt。
- page fault 的专门处理策略。
- double fault 独立 IST 栈。
- 用户态异常返回。
- 可恢复异常处理。
- oops 格式和符号化输出。

验收依据：

- 正常启动日志包含 `traps initialized`。
- invalid opcode 测试能输出 vector、error code、rip、rsp、rflags。
- 未处理异常能进入 `panic("unhandled CPU exception")`。

下一阶段：

- `03-memory.md`，先建立物理页、页表和内核堆。
