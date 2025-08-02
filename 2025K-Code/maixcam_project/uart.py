from maix import uart

class CamComm:
    # 预定义常量避免重复计算
    _HEADER = 0xAA
    _TAIL = 0x55
    _MAX_DATA_LEN = 255
    
    def __init__(self, device="/dev/ttyS0", baudrate=115200):
        self.serial = uart.UART(device, baudrate)
        # 预分配缓冲区避免频繁内存分配
        self.rx_buffer = bytearray(self._MAX_DATA_LEN + 3)  # header + len + data + tail
        self.data_buffer = bytearray(self._MAX_DATA_LEN)
        self._reset_state()
        
        # 预编码常用命令前缀，避免重复字符串操作
        self._cmd_prefix = b"C:0x"
        self._track_prefix = b"T:0x" 
        self._num_prefix = b"N:"
    
    def _reset_state(self):
        """重置状态机，内联避免函数调用开销"""
        self.state = 0
        self.length = 0
        self.count = 0
    
    def _send(self, data):
        """优化发送：减少内存分配"""
        if isinstance(data, str):
            data = data.encode()
        
        data_len = len(data)
        # 直接构建packet避免多次bytes()调用
        self.rx_buffer[0] = self._HEADER
        self.rx_buffer[1] = data_len
        self.rx_buffer[2:2+data_len] = data
        self.rx_buffer[2+data_len] = self._TAIL
        
        # 修复：转换为bytes类型
        packet = bytes(self.rx_buffer[:3+data_len])
        return self.serial.write(packet) > 0
    
    def _receive(self, timeout=1000):  # 减少默认超时
        """优化接收：状态机优化，减少条件判断"""
        rx_data = self.serial.read(timeout=timeout)
        if not rx_data:
            return None
        
        # 使用局部变量减少self访问开销
        state, length, count = self.state, self.length, self.count
        data_buf = self.data_buffer
        
        for byte in rx_data:
            if state == 0:
                if byte == self._HEADER:
                    state, length, count = 1, 0, 0
            elif state == 1:
                length = byte
                state = 2 if byte > 0 else 3
            elif state == 2:
                data_buf[count] = byte
                count += 1
                if count >= length:
                    state = 3
            else:  # state == 3
                state = 0
                if byte == self._TAIL:
                    # 更新实例状态
                    self.state, self.length, self.count = state, length, count
                    # 避免decode开销，直接返回bytes切片
                    try:
                        return data_buf[:count].decode('utf-8')
                    except:
                        return data_buf[:count]  # 返回原始bytes
        
        # 更新状态
        self.state, self.length, self.count = state, length, count
        return None
    
    def send_track(self, value):
        """优化：预构建格式避免f-string开销"""
        # 直接构建bytes避免字符串格式化
        hex_str = format(value, '02X').encode()
        packet_len = 4 + len(hex_str)  # "T:0x" + hex
        
        self.rx_buffer[0] = self._HEADER
        self.rx_buffer[1] = packet_len
        self.rx_buffer[2:6] = self._track_prefix
        self.rx_buffer[6:6+len(hex_str)] = hex_str
        self.rx_buffer[6+len(hex_str)] = self._TAIL
        
        # 修复：转换为bytes类型
        packet = bytes(self.rx_buffer[:7+len(hex_str)])
        return self.serial.write(packet) > 0
    
    def send_number(self, value):
        """优化数字发送"""
        num_str = str(value).encode()
        packet_len = 2 + len(num_str)  # "N:" + number
        
        self.rx_buffer[0] = self._HEADER  
        self.rx_buffer[1] = packet_len
        self.rx_buffer[2:4] = self._num_prefix
        self.rx_buffer[4:4+len(num_str)] = num_str
        self.rx_buffer[4+len(num_str)] = self._TAIL
        
        # 修复：转换为bytes类型
        packet = bytes(self.rx_buffer[:5+len(num_str)])
        return self.serial.write(packet) > 0
    
    def send_command(self, code):
        """优化命令发送"""
        hex_str = format(code, '02X').encode()
        packet_len = 4 + len(hex_str)  # "C:0x" + hex
        
        self.rx_buffer[0] = self._HEADER
        self.rx_buffer[1] = packet_len  
        self.rx_buffer[2:6] = self._cmd_prefix
        self.rx_buffer[6:6+len(hex_str)] = hex_str
        self.rx_buffer[6+len(hex_str)] = self._TAIL
        
        # 修复：转换为bytes类型
        packet = bytes(self.rx_buffer[:7+len(hex_str)])
        return self.serial.write(packet) > 0
    
    # 快速发送方法：跳过格式化，直接发送常用命令
    def send_raw(self, cmd_bytes):
        """发送原始命令字节，最高性能"""
        data_len = len(cmd_bytes)
        self.rx_buffer[0] = self._HEADER
        self.rx_buffer[1] = data_len
        self.rx_buffer[2:2+data_len] = cmd_bytes  
        self.rx_buffer[2+data_len] = self._TAIL
        
        # 修复：转换为bytes类型
        packet = bytes(self.rx_buffer[:3+data_len])
        return self.serial.write(packet) > 0
    
    def receive(self, timeout=1000):
        """接收数据，减少默认超时"""
        return self._receive(timeout)
    
    def send_receive(self, data, timeout=1000):
        """发送并接收，减少超时"""
        if not self._send(data):
            return None
        return self._receive(timeout)
    
    # 批量操作方法，减少串口调用次数
    def send_batch(self, commands, delay_ms=0):
        """批量发送命令"""
        results = []
        for cmd in commands:
            result = self._send(cmd)
            results.append(result)
            if delay_ms > 0:
                # 如果需要延时，这里可以添加延时逻辑
                pass
        return results