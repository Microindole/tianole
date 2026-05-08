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
