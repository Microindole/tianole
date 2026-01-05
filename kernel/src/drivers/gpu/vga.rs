use limine::framebuffer::Framebuffer;

pub fn init(_fb: &Framebuffer) {
    // 初始化逻辑（暂空）
}

// 画一个矩形
pub fn draw_rect(fb: &Framebuffer, x: usize, y: usize, w: usize, h: usize, color: u32) {
    // 获取显存地址的原始指针
    let base_ptr = fb.addr() as *mut u8;
    let pitch = fb.pitch() as usize;
    let bpp = (fb.bpp() / 8) as usize; // 每像素字节数 (通常是 4)

    for i in 0..h {
        for j in 0..w {
            let offset = (y + i) * pitch + (x + j) * bpp;
            unsafe {
                // 写入颜色 (假设是 32位真彩色 RGB)
                // 注意：这里没有做边界检查，实际生产代码需要加上
                let pixel_ptr = base_ptr.add(offset) as *mut u32;
                *pixel_ptr = color;
            }
        }
    }
}
