//! 中断和异常处理函数

use crate::serial_println;
use x86_64::structures::idt::{InterruptStackFrame, PageFaultErrorCode};

/// 除零错误处理
pub extern "x86-interrupt" fn divide_error_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: DIVIDE BY ZERO\n{:#?}", stack_frame);
}

/// 调试异常处理
pub extern "x86-interrupt" fn debug_handler(stack_frame: InterruptStackFrame) {
    serial_println!("EXCEPTION: DEBUG\n{:#?}", stack_frame);
}

/// 断点异常处理
///
/// 这个异常可以恢复，所以不 panic
pub extern "x86-interrupt" fn breakpoint_handler(stack_frame: InterruptStackFrame) {
    serial_println!(
        "EXCEPTION: BREAKPOINT at {:#x}",
        stack_frame.instruction_pointer
    );
    serial_println!("{:#?}", stack_frame);
}

/// 溢出异常处理
pub extern "x86-interrupt" fn overflow_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: OVERFLOW\n{:#?}", stack_frame);
}

/// 边界检查异常处理
pub extern "x86-interrupt" fn bound_range_exceeded_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: BOUND RANGE EXCEEDED\n{:#?}", stack_frame);
}

/// 无效操作码异常处理
pub extern "x86-interrupt" fn invalid_opcode_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: INVALID OPCODE\n{:#?}", stack_frame);
}

/// 设备不可用异常处理
pub extern "x86-interrupt" fn device_not_available_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: DEVICE NOT AVAILABLE\n{:#?}", stack_frame);
}

/// 双重错误处理
///
/// 这是一个 abort 类型的异常，无法恢复
pub extern "x86-interrupt" fn double_fault_handler(
    stack_frame: InterruptStackFrame,
    _error_code: u64,
) -> ! {
    panic!("EXCEPTION: DOUBLE FAULT\n{:#?}", stack_frame);
}

/// 无效 TSS 异常处理
pub extern "x86-interrupt" fn invalid_tss_handler(
    stack_frame: InterruptStackFrame,
    error_code: u64,
) {
    panic!(
        "EXCEPTION: INVALID TSS (error code: {})\n{:#?}",
        error_code, stack_frame
    );
}

/// 段不存在异常处理
pub extern "x86-interrupt" fn segment_not_present_handler(
    stack_frame: InterruptStackFrame,
    error_code: u64,
) {
    panic!(
        "EXCEPTION: SEGMENT NOT PRESENT (error code: {})\n{:#?}",
        error_code, stack_frame
    );
}

/// 栈段错误处理
pub extern "x86-interrupt" fn stack_segment_fault_handler(
    stack_frame: InterruptStackFrame,
    error_code: u64,
) {
    panic!(
        "EXCEPTION: STACK SEGMENT FAULT (error code: {})\n{:#?}",
        error_code, stack_frame
    );
}

/// 一般保护错误处理
pub extern "x86-interrupt" fn general_protection_fault_handler(
    stack_frame: InterruptStackFrame,
    error_code: u64,
) {
    panic!(
        "EXCEPTION: GENERAL PROTECTION FAULT (error code: {})\n{:#?}",
        error_code, stack_frame
    );
}

/// 页错误处理
pub extern "x86-interrupt" fn page_fault_handler(
    stack_frame: InterruptStackFrame,
    error_code: PageFaultErrorCode,
) {
    use x86_64::registers::control::Cr2;

    serial_println!("EXCEPTION: PAGE FAULT");
    serial_println!("Accessed Address: {:?}", Cr2::read());
    serial_println!("Error Code: {:?}", error_code);
    serial_println!("{:#?}", stack_frame);

    panic!("Page fault");
}

/// x87 浮点异常处理
pub extern "x86-interrupt" fn x87_floating_point_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: x87 FLOATING POINT\n{:#?}", stack_frame);
}

/// 对齐检查异常处理
pub extern "x86-interrupt" fn alignment_check_handler(
    stack_frame: InterruptStackFrame,
    error_code: u64,
) {
    panic!(
        "EXCEPTION: ALIGNMENT CHECK (error code: {})\n{:#?}",
        error_code, stack_frame
    );
}

/// 机器检查异常处理
pub extern "x86-interrupt" fn machine_check_handler(stack_frame: InterruptStackFrame) -> ! {
    panic!("EXCEPTION: MACHINE CHECK\n{:#?}", stack_frame);
}

/// SIMD 浮点异常处理
pub extern "x86-interrupt" fn simd_floating_point_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: SIMD FLOATING POINT\n{:#?}", stack_frame);
}

/// 虚拟化异常处理
pub extern "x86-interrupt" fn virtualization_handler(stack_frame: InterruptStackFrame) {
    panic!("EXCEPTION: VIRTUALIZATION\n{:#?}", stack_frame);
}

/// 安全异常处理
pub extern "x86-interrupt" fn security_exception_handler(
    stack_frame: InterruptStackFrame,
    error_code: u64,
) {
    panic!(
        "EXCEPTION: SECURITY (error code: {})\n{:#?}",
        error_code, stack_frame
    );
}
