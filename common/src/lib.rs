#![no_std]

// 定义系统调用号，内核和用户态都要用
#[repr(usize)]
pub enum SyscallId {
    Print = 0,
    Exit = 1,
    Fork = 2,
    Exec = 3,
}
