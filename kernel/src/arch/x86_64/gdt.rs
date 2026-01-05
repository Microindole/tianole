//! 全局描述符表（GDT）和任务状态段（TSS）

use lazy_static::lazy_static;
use x86_64::structures::gdt::{Descriptor, GlobalDescriptorTable, SegmentSelector};
use x86_64::structures::tss::TaskStateSegment;
use x86_64::VirtAddr;

/// 双重错误栈在 IST 中的索引
pub const DOUBLE_FAULT_IST_INDEX: u16 = 0;

lazy_static! {
    /// 任务状态段（TSS）
    ///
    /// 主要用于为双重错误提供独立的栈
    static ref TSS: TaskStateSegment = {
        let mut tss = TaskStateSegment::new();

        // 为双重错误分配独立的栈（20 KB）
        tss.interrupt_stack_table[DOUBLE_FAULT_IST_INDEX as usize] = {
            const STACK_SIZE: usize = 4096 * 5; // 5 页 = 20 KB
            static mut STACK: [u8; STACK_SIZE] = [0; STACK_SIZE];

            let stack_start = VirtAddr::from_ptr(unsafe { &STACK });
            let stack_end = stack_start + STACK_SIZE;
            stack_end  // 栈从高地址向低地址增长
        };

        tss
    };
}

lazy_static! {
    /// 全局描述符表（GDT）
    static ref GDT: (GlobalDescriptorTable, Selectors) = {
        let mut gdt = GlobalDescriptorTable::new();

        // 添加代码段和 TSS
        let code_selector = gdt.add_entry(Descriptor::kernel_code_segment());
        let tss_selector = gdt.add_entry(Descriptor::tss_segment(&TSS));

        (gdt, Selectors { code_selector, tss_selector })
    };
}

/// GDT 选择子
struct Selectors {
    code_selector: SegmentSelector,
    tss_selector: SegmentSelector,
}

/// 初始化 GDT 和 TSS
pub fn init() {
    use x86_64::instructions::segmentation::{Segment, CS};
    use x86_64::instructions::tables::load_tss;

    // 加载 GDT
    GDT.0.load();

    unsafe {
        // 重新加载代码段寄存器
        CS::set_reg(GDT.1.code_selector);

        // 加载 TSS
        load_tss(GDT.1.tss_selector);
    }
}
