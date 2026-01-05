pub mod gdt;
pub mod idt;
pub mod interrupts;

/// 初始化 x86_64 架构相关的所有组件
pub fn init() {
    gdt::init();
    idt::init();
}
