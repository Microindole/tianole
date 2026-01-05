//! 物理内存管理 - 帧分配器
//!
//! 管理物理内存页帧（4KB），使用位图追踪可用帧

use crate::serial_println;
use core::fmt;
use lazy_static::lazy_static;
use limine::memory_map::EntryType;
use limine::response::MemoryMapResponse;
use spin::Mutex;
use x86_64::structures::paging::{FrameAllocator, PhysFrame, Size4KiB};
use x86_64::PhysAddr;

/// 页帧大小 (4 KB)
pub const PAGE_SIZE: usize = 4096;

/// 物理帧号
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord)]
pub struct FrameNumber(pub usize);

impl FrameNumber {
    /// 从物理地址创建帧号
    pub fn from_phys_addr(addr: PhysAddr) -> Self {
        FrameNumber(addr.as_u64() as usize / PAGE_SIZE)
    }

    /// 转换为物理地址
    pub fn to_phys_addr(self) -> PhysAddr {
        PhysAddr::new((self.0 * PAGE_SIZE) as u64)
    }
}

/// 位图帧分配器
pub struct BitmapFrameAllocator {
    /// 位图：每个 bit 代表一个物理帧
    /// bit = 1: 可用, bit = 0: 已使用
    bitmap: &'static mut [u8],

    /// 位图覆盖的帧范围
    start_frame: FrameNumber,
    end_frame: FrameNumber,

    /// 统计信息
    total_frames: usize,
    used_frames: usize,
}

impl BitmapFrameAllocator {
    /// 创建新的帧分配器
    pub fn new(
        usable_memory_start: PhysAddr,
        usable_memory_end: PhysAddr,
        bitmap_storage: &'static mut [u8],
    ) -> Self {
        let start = FrameNumber::from_phys_addr(usable_memory_start);
        let end = FrameNumber::from_phys_addr(usable_memory_end);
        let total = end.0 - start.0;

        // 初始化位图（全部设为可用）
        for byte in bitmap_storage.iter_mut() {
            *byte = 0xFF; // 1 = 可用
        }

        Self {
            bitmap: bitmap_storage,
            start_frame: start,
            end_frame: end,
            total_frames: total,
            used_frames: 0,
        }
    }

    /// 分配一个物理帧
    pub fn alloc(&mut self) -> Option<PhysFrame> {
        // 查找第一个可用帧（bit = 1）
        for (byte_idx, &byte) in self.bitmap.iter().enumerate() {
            if byte != 0 {
                // 找到可用的 bit
                let bit = byte.trailing_zeros() as usize;
                let frame_offset = byte_idx * 8 + bit;
                let frame_num = self.start_frame.0 + frame_offset;

                if frame_num >= self.end_frame.0 {
                    return None;
                }

                // 标记为已使用
                self.bitmap[byte_idx] &= !(1 << bit);
                self.used_frames += 1;

                // 返回物理帧
                let addr = PhysAddr::new((frame_num * PAGE_SIZE) as u64);
                return Some(PhysFrame::containing_address(addr));
            }
        }
        None
    }

    /// 释放一个物理帧
    pub fn dealloc(&mut self, frame: PhysFrame) {
        let frame_num = frame.start_address().as_u64() as usize / PAGE_SIZE;

        if frame_num < self.start_frame.0 || frame_num >= self.end_frame.0 {
            return; // 超出范围
        }

        let frame_offset = frame_num - self.start_frame.0;
        let byte_idx = frame_offset / 8;
        let bit = frame_offset % 8;

        // 标记为可用
        if byte_idx < self.bitmap.len() {
            let was_used = (self.bitmap[byte_idx] & (1 << bit)) == 0;
            self.bitmap[byte_idx] |= 1 << bit;
            if was_used {
                self.used_frames -= 1;
            }
        }
    }

    /// 标记一段内存范围为已使用
    pub fn mark_range_used(&mut self, start_addr: u64, length: u64) {
        let start_frame = FrameNumber::from_phys_addr(PhysAddr::new(start_addr));
        let end_frame = FrameNumber::from_phys_addr(PhysAddr::new(start_addr + length));

        for frame_num in start_frame.0..end_frame.0 {
            if frame_num < self.start_frame.0 || frame_num >= self.end_frame.0 {
                continue;
            }

            let frame_offset = frame_num - self.start_frame.0;
            let byte_idx = frame_offset / 8;
            let bit = frame_offset % 8;

            if byte_idx < self.bitmap.len() {
                let was_free = (self.bitmap[byte_idx] & (1 << bit)) != 0;
                self.bitmap[byte_idx] &= !(1 << bit);
                if was_free {
                    self.used_frames += 1;
                }
            }
        }
    }

    /// 获取可用帧数
    pub fn free_frames(&self) -> usize {
        self.total_frames - self.used_frames
    }

    /// 获取已用帧数
    pub fn used_frames(&self) -> usize {
        self.used_frames
    }
}

impl fmt::Debug for BitmapFrameAllocator {
    fn fmt(&self, f: &mut fmt::Formatter) -> fmt::Result {
        f.debug_struct("BitmapFrameAllocator")
            .field("total_frames", &self.total_frames)
            .field("used_frames", &self.used_frames)
            .field("free_frames", &self.free_frames())
            .finish()
    }
}

// ===== 全局帧分配器 =====

lazy_static! {
    /// 全局帧分配器
    pub static ref FRAME_ALLOCATOR: Mutex<Option<BitmapFrameAllocator>> = Mutex::new(None);
}

/// 初始化帧分配器
pub fn init_frame_allocator(memory_map: &MemoryMapResponse) {
    serial_println!("[INFO] Initializing frame allocator...");

    // 1. 打印内存映射
    serial_println!("[INFO] Memory map:");
    for entry in memory_map.entries() {
        let entry_type_str = match entry.entry_type {
            EntryType::USABLE => "Usable",
            EntryType::RESERVED => "Reserved",
            EntryType::ACPI_RECLAIMABLE => "ACPI Reclaimable",
            EntryType::ACPI_NVS => "ACPI NVS",
            EntryType::BAD_MEMORY => "Bad Memory",
            EntryType::BOOTLOADER_RECLAIMABLE => "Bootloader Reclaimable",
            EntryType::KERNEL_AND_MODULES => "Kernel/Modules",
            EntryType::FRAMEBUFFER => "Framebuffer",
            _ => "Unknown",
        };
        serial_println!(
            "  {:#018x} - {:#018x} {}",
            entry.base,
            entry.base + entry.length,
            entry_type_str
        );
    }

    // 2. 找到最大的可用内存地址
    let mut max_addr = 0u64;
    for entry in memory_map.entries() {
        if entry.entry_type == EntryType::USABLE {
            let end = entry.base + entry.length;
            if end > max_addr {
                max_addr = end;
            }
        }
    }

    serial_println!("[INFO] Max usable address: {:#x}", max_addr);

    // 3. 计算需要多少字节的位图
    let total_frames = (max_addr as usize) / PAGE_SIZE;
    let bitmap_bytes = (total_frames + 7) / 8; // 向上取整

    serial_println!(
        "[INFO] Total frames: {}, Bitmap size: {} KB",
        total_frames,
        bitmap_bytes / 1024
    );

    // 4. 分配位图存储（从第一个大的可用区域）
    let mut bitmap_storage: Option<&'static mut [u8]> = None;
    let mut bitmap_phys_start = 0u64;

    unsafe {
        for entry in memory_map.entries() {
            if entry.entry_type == EntryType::USABLE && entry.length as usize >= bitmap_bytes {
                let bitmap_ptr = entry.base as *mut u8;
                bitmap_storage = Some(core::slice::from_raw_parts_mut(bitmap_ptr, bitmap_bytes));
                bitmap_phys_start = entry.base;
                break;
            }
        }
    }

    let bitmap_storage = bitmap_storage.expect("No memory region large enough for bitmap!");

    serial_println!("[INFO] Bitmap stored at {:#x}", bitmap_phys_start);

    // 5. 创建分配器
    let mut allocator =
        BitmapFrameAllocator::new(PhysAddr::new(0), PhysAddr::new(max_addr), bitmap_storage);

    // 6. 标记已使用的区域
    for entry in memory_map.entries() {
        if entry.entry_type != EntryType::USABLE {
            allocator.mark_range_used(entry.base, entry.length);
        }
    }

    // 7. 标记位图本身为已使用
    allocator.mark_range_used(bitmap_phys_start, bitmap_bytes as u64);

    serial_println!("[INFO] Frame allocator: {:?}", allocator);

    // 8. 保存到全局变量
    *FRAME_ALLOCATOR.lock() = Some(allocator);
}

/// 分配一个物理帧（便捷函数）
pub fn alloc_frame() -> Option<PhysFrame> {
    FRAME_ALLOCATOR.lock().as_mut()?.alloc()
}

/// 释放一个物理帧（便捷函数）
pub fn dealloc_frame(frame: PhysFrame) {
    if let Some(allocator) = FRAME_ALLOCATOR.lock().as_mut() {
        allocator.dealloc(frame);
    }
}

/// 实现 x86_64 的 FrameAllocator trait（用于页表管理）
unsafe impl FrameAllocator<Size4KiB> for &mut BitmapFrameAllocator {
    fn allocate_frame(&mut self) -> Option<PhysFrame<Size4KiB>> {
        self.alloc()
    }
}
