use core::panic::PanicInfo;

#[panic_handler]
fn panic(_info: &PanicInfo) -> ! {
    // 这里未来可以打印错误信息到屏幕
    loop {
        // 死循环挂起 CPU
        x86_64::instructions::hlt();
    }
}