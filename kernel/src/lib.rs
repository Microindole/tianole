#![no_std] // 这是一个裸机库

// 公开定义模块
pub mod arch;
pub mod drivers;
pub mod fs;
pub mod ipc;
pub mod mm;
pub mod net;
pub mod sync;
pub mod syscall;
pub mod task;

// 引入 panic handler
pub mod panic;

// 仅仅是为了让 drivers/mod.rs 能找到 gpu/vga.rs，我们需要确保 mod.rs 存在
// 你之前脚本创建了空的 mod.rs，这里不需要改动那些 mod.rs
// 但你需要去 kernel/src/drivers/mod.rs 加上 `pub mod gpu;`
// 去 kernel/src/drivers/gpu/mod.rs 加上 `pub mod vga;`
