#![no_std]
#![no_main]

// 引入 kernel crate 的 lib.rs 中定义的内容
use kernel::drivers;

// Limine 0.5 的请求方式
use limine::request::{FramebufferRequest, HhdmRequest};

// 静态 Framebuffer 请求
static FRAMEBUFFER_REQUEST: FramebufferRequest = FramebufferRequest::new();

// 高半核直接映射请求 (Higher Half Direct Map)
// 这会让 Limine 设置一个从物理内存到高半核虚拟地址的映射
// 注意：Limine 通过链接器符号使用这个静态变量，不是通过 Rust 代码直接引用
#[used]
static HHDM_REQUEST: HhdmRequest = HhdmRequest::new();

#[no_mangle]
pub extern "C" fn _start() -> ! {
    // 1. 获取 Framebuffer
    if let Some(framebuffer_response) = FRAMEBUFFER_REQUEST.get_response() {
        if let Some(framebuffer) = framebuffer_response.framebuffers().next() {
            // 2. 画个背景
            //  0x00FF00FF 是 紫色 (R=FF, G=00, B=FF)
            drivers::gpu::vga::draw_rect(&framebuffer, 0, 0, 200, 200, 0x00FF00FF);

            // 再画个绿色的块
            drivers::gpu::vga::draw_rect(&framebuffer, 50, 50, 100, 100, 0x0000FF00);
        }
    }

    // 3. 进入死循环
    loop {
        x86_64::instructions::hlt();
    }
}
