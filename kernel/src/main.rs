#![no_std]
#![no_main]

// 引入 kernel crate 的 lib.rs 中定义的内容
use kernel::drivers;
use kernel::serial_println; // 导入串口日志宏

// Limine 0.5 的请求方式
use limine::request::{FramebufferRequest, HhdmRequest, MemoryMapRequest};

// 静态 Framebuffer 请求
static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();

// 高半核直接映射请求 (Higher Half Direct Map)
// 这会让 Limine 设置一个从物理内存到高半核虚拟地址的映射
// 注意：Limine 通过链接器符号使用这个静态变量，不是通过 Rust 代码直接引用
#[used]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();

// 内存映射请求
// 获取系统物理内存布局
#[used]
static MEMORY_MAP_REQUEST: MemoryMapRequest = MemoryMapRequest::new();

#[no_mangle]
pub extern "C" fn _start() -> ! {
    // ===== 串口初始化（通过 lazy_static 自动完成） =====
    serial_println!();
    serial_println!("====================================");
    serial_println!("    Tianole OS is booting...");
    serial_println!("====================================");
    serial_println!();

    serial_println!("[INFO] Initializing hardware...");

    // ===== 初始化 GDT 和 IDT =====
    serial_println!("[INFO] Initializing GDT...");
    kernel::arch::x86_64::gdt::init();
    serial_println!("[OK] GDT initialized!");

    serial_println!("[INFO] Initializing IDT...");
    kernel::arch::x86_64::idt::init();
    serial_println!("[OK] IDT initialized!");

    // ===== 初始化物理内存管理 =====
    serial_println!("[INFO] Setting up physical memory management...");
    if let Some(memory_map) = MEMORY_MAP_REQUEST.get_response() {
        kernel::mm::init_frame_allocator(memory_map);
        serial_println!("[OK] Physical memory management initialized!");

        // 测试分配和释放
        serial_println!("[TEST] Testing frame allocation...");
        if let Some(frame) = kernel::mm::alloc_frame() {
            serial_println!("[OK] Allocated frame at {:#x}", frame.start_address());
            kernel::mm::dealloc_frame(frame);
            serial_println!("[OK] Frame deallocated!");
        } else {
            serial_println!("[ERROR] Failed to allocate frame!");
        }
    } else {
        panic!("No memory map from bootloader!");
    }

    // ===== 测试断点异常（暂时禁用，等物理内存管理完成后再测试）=====
    // serial_println!("[TEST] Testing breakpoint exception...");
    // x86_64::instructions::interrupts::int3();
    // serial_println!("[OK] Breakpoint exception handled successfully!");

    // ===== 获取 Framebuffer =====
    serial_println!("[INFO] Setting up framebuffer...");
    if let Some(framebuffer_response) = FRAMEBUFFER_REQUEST.get_response() {
        if let Some(framebuffer) = framebuffer_response.framebuffers().next() {
            let width = framebuffer.width();
            let height = framebuffer.height();
            let bpp = framebuffer.bpp();

            serial_println!("[OK] Framebuffer initialized:");
            serial_println!("     Resolution: {}x{}", width, height);
            serial_println!("     Bits per pixel: {}", bpp);

            // 绘制紫色背景
            serial_println!("[INFO] Drawing purple background...");
            drivers::gpu::vga::draw_rect(&framebuffer, 0, 0, 200, 200, 0x00FF00FF);

            // 绘制绿色方块
            serial_println!("[INFO] Drawing green rectangle...");
            drivers::gpu::vga::draw_rect(&framebuffer, 50, 50, 100, 100, 0x0000FF00);

            serial_println!("[OK] Graphics initialized successfully!");
        } else {
            serial_println!("[ERROR] No framebuffer available!");
        }
    } else {
        serial_println!("[ERROR] Framebuffer request failed!");
    }

    serial_println!();
    serial_println!("====================================");
    serial_println!("  Kernel initialization complete!");
    serial_println!("====================================");
    serial_println!();
    serial_println!("[INFO] Entering idle loop...");

    // ===== 进入死循环 =====
    loop {
        x86_64::instructions::hlt();
    }
}
