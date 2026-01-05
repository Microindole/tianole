//! UART 16550 串口驱动
//!
//! 提供基本的串口通信功能，用于内核日志输出和调试

use core::fmt;
use lazy_static::lazy_static;
use spin::Mutex;
use x86_64::instructions::port::Port;

/// COM1 串口基地址
const SERIAL_IO_PORT: u16 = 0x3F8;

/// UART 16550 串口端口
pub struct SerialPort {
    data: Port<u8>,        // 数据寄存器 (offset 0)
    int_enable: Port<u8>,  // 中断使能 (offset 1)
    fifo_ctrl: Port<u8>,   // FIFO 控制 (offset 2)
    line_ctrl: Port<u8>,   // 线路控制 (offset 3)
    modem_ctrl: Port<u8>,  // Modem 控制 (offset 4)
    line_status: Port<u8>, // 线路状态 (offset 5)
}

impl SerialPort {
    /// 创建一个新的串口实例（未初始化）
    pub const fn new(port: u16) -> Self {
        SerialPort {
            data: Port::new(port),
            int_enable: Port::new(port + 1),
            fifo_ctrl: Port::new(port + 2),
            line_ctrl: Port::new(port + 3),
            modem_ctrl: Port::new(port + 4),
            line_status: Port::new(port + 5),
        }
    }

    /// 初始化串口
    ///
    /// 配置：
    /// - 波特率: 38400 bps
    /// - 数据位: 8 bits
    /// - 停止位: 1 bit
    /// - 校验位: None
    pub fn init(&mut self) {
        unsafe {
            // 1. 禁用所有中断
            self.int_enable.write(0x00);

            // 2. 启用 DLAB（分频器锁存访问位）以设置波特率
            self.line_ctrl.write(0x80);

            // 3. 设置波特率为 38400 (divisor = 115200 / 38400 = 3)
            self.data.write(0x03); // 低字节
            self.int_enable.write(0x00); // 高字节

            // 4. 设置数据格式：8 位，无校验，1 停止位（禁用 DLAB）
            self.line_ctrl.write(0x03);

            // 5. 启用 FIFO，清空队列，14 字节阈值
            self.fifo_ctrl.write(0xC7);

            // 6. 设置 Modem 控制寄存器：RTS、DTR
            self.modem_ctrl.write(0x0B);

            // 7. 启用中断（可选，这里暂时不启用）
            self.int_enable.write(0x00);
        }
    }

    /// 发送一个字节
    fn send_byte(&mut self, byte: u8) {
        unsafe {
            // 等待发送缓冲区为空
            while self.line_status.read() & 0x20 == 0 {
                core::hint::spin_loop();
            }
            self.data.write(byte);
        }
    }

    /// 发送字符串
    pub fn send_str(&mut self, s: &str) {
        for byte in s.bytes() {
            // 将 \n 转换为 \r\n（回车换行）
            if byte == b'\n' {
                self.send_byte(b'\r');
            }
            self.send_byte(byte);
        }
    }
}

/// 实现 fmt::Write trait，允许使用 write_fmt
impl fmt::Write for SerialPort {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.send_str(s);
        Ok(())
    }
}

// ===== 全局串口实例 =====

lazy_static! {
    /// 全局 COM1 串口实例
   pub static ref SERIAL1: Mutex<SerialPort> = {
        let mut serial = SerialPort::new(SERIAL_IO_PORT);
        serial.init();
        Mutex::new(serial)
    };
}

// ===== 日志宏 =====

/// 向串口打印（不换行）
#[macro_export]
macro_rules! serial_print {
    ($($arg:tt)*) => {
        $crate::drivers::r#char::serial::_print(format_args!($($arg)*));
    };
}

/// 向串口打印（换行）
#[macro_export]
macro_rules! serial_println {
    () => ($crate::serial_print!("\n"));
    ($($arg:tt)*) => ($crate::serial_print!("{}\n", format_args!($($arg)*)));
}

/// 内部打印函数（供宏使用）
#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    SERIAL1
        .lock()
        .write_fmt(args)
        .expect("Serial port write failed");
}
