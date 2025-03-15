import serial
import time
import random
import string
import threading
import matplotlib.pyplot as plt
import numpy as np
from tqdm import tqdm

class UartBufferTest:
    def __init__(self, port, baudrate=115200, timeout=1):
        """初始化测试对象"""
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        self.ser = None
        self.results = {}
        
    def connect(self):
        """连接串口"""
        try:
            self.ser = serial.Serial(
                port=self.port,
                baudrate=self.baudrate,
                timeout=self.timeout
            )
            print(f"成功连接到 {self.port}, 波特率: {self.baudrate}")
            time.sleep(2)  # 等待MCU稳定
            # 清空缓冲区
            self.ser.reset_input_buffer()
            self.ser.reset_output_buffer()
            return True
        except Exception as e:
            print(f"连接失败: {e}")
            return False
    
    def disconnect(self):
        """关闭串口连接"""
        if self.ser and self.ser.is_open:
            self.ser.close()
            print("串口连接已关闭")
    
    def send_data(self, data, wait_for_response=True):
        """发送数据并可选择等待响应"""
        if not self.ser or not self.ser.is_open:
            print("串口未连接")
            return None
            
        # 发送数据
        self.ser.write(data)
        
        if wait_for_response:
            # 等待回显数据
            time.sleep(0.1)
            response = b''
            start_time = time.time()
            while self.ser.in_waiting > 0 or time.time() - start_time < self.timeout:
                if self.ser.in_waiting > 0:
                    chunk = self.ser.read(self.ser.in_waiting)
                    response += chunk
                    if len(response) >= len(data):
                        break
                time.sleep(0.01)
            return response
        return None
    def run_basic_echo_test(self):
        """基本回显测试"""
        print("\n===== 基本回显测试 =====")
        test_strings = [
            "Hello World!",
            "Testing UART Buffer",
            "1234567890" * 3,
            "A" * 10,
            "Z" * 60  # 接近缓冲区大小
        ]
        
        results = []
        for test_str in test_strings:
            print(f"发送: {test_str}")
            sent_data = test_str.encode('utf-8')
            response = self.send_data(sent_data)
            
            if response:
                # 修改判断逻辑：只要发送的数据包含在接收数据中即为成功
                success = sent_data in response
                print(f"接收: {response.decode('utf-8', errors='ignore')}")
                print(f"测试{'成功' if success else '失败'}\n")
                results.append(success)
            else:
                print("未收到响应\n")
                results.append(False)
        
        success_rate = sum(results) / len(results) * 100
        print(f"基本回显测试结束，成功率: {success_rate:.1f}%")
        self.results['echo_test_success_rate'] = success_rate
        return success_rate == 100
    
    def run_throughput_test(self, duration=5, chunk_size=64):
        """吞吐量测试"""
        print(f"\n===== 吞吐量测试 (持续{duration}秒) =====")
        # 创建测试数据 - 随机字符串
        data = ''.join(random.choices(string.ascii_letters + string.digits, k=chunk_size)).encode()
        
        start_time = time.time()
        total_bytes = 0
        
        print("发送数据中...")
        with tqdm(total=duration) as pbar:
            while time.time() - start_time < duration:
                sent = self.ser.write(data)
                total_bytes += sent
                
                # 读取响应以清空接收缓冲区
                time.sleep(0.01)
                if self.ser.in_waiting > 0:
                    self.ser.read(self.ser.in_waiting)
                
                # 更新进度条
                elapsed = time.time() - start_time
                pbar.update(elapsed - pbar.n)
                
        elapsed_time = time.time() - start_time
        throughput = total_bytes / elapsed_time / 1024  # KB/s
        
        print(f"发送总字节数: {total_bytes} 字节")
        print(f"耗时: {elapsed_time:.2f} 秒")
        print(f"吞吐量: {throughput:.2f} KB/s")
        
        self.results['throughput_bytes'] = total_bytes
        self.results['throughput_time'] = elapsed_time
        self.results['throughput_rate'] = throughput
        return throughput
    
    def run_boundary_test(self):
        """边界测试 - 发送超过缓冲区大小的数据"""
        print("\n===== 边界测试 =====")
        # UART_RX_BUFFER_SIZE定义为256，我们发送超过这个大小的数据
        sizes = [220, 240, 250, 256, 260, 270, 280, 300, 350, 400]
        
        results = []
        for size in sizes:
            print(f"测试大小: {size} 字节")
            # 生成测试数据，使每个字节值不同以便于识别
            test_data = bytes([i % 256 for i in range(size)])
            
            # 发送数据
            self.ser.reset_input_buffer()
            sent_time = time.time()
            self.ser.write(test_data)
            
            # 给足够的时间接收回显
            time.sleep(0.5)
            
            # 读取响应
            response = self.ser.read(self.ser.in_waiting)
            
            # 计算成功接收的字节数
            received_bytes = len(response)
            match_percentage = 0
            
            # 检查接收到的数据是否符合预期
            if received_bytes > 0:
                # 计算匹配字节数
                matching_bytes = sum(1 for a, b in zip(test_data[:received_bytes], response) if a == b)
                match_percentage = (matching_bytes / received_bytes) * 100
            
            print(f"发送: {size} 字节, 接收: {received_bytes} 字节")
            print(f"数据匹配率: {match_percentage:.1f}%\n")
            
            results.append({
                'size': size,
                'received': received_bytes,
                'match_percentage': match_percentage
            })
        
        self.results['boundary_test'] = results
        return results
    
    def run_latency_test(self, iterations=100):
        """延迟测试 - 测量RTT (Round Trip Time)"""
        print(f"\n===== 延迟测试 ({iterations}次迭代) =====")
        latencies = []
        
        test_data = b"PING"
        
        for i in range(iterations):
            # 清空输入缓冲区
            self.ser.reset_input_buffer()
            
            # 发送测试数据并测量时间
            start_time = time.time() * 1000  # 毫秒
            self.ser.write(test_data)
            
            # 等待响应
            response = b''
            while len(response) < len(test_data):
                if self.ser.in_waiting > 0:
                    response += self.ser.read(self.ser.in_waiting)
                # 超时检查
                if time.time() * 1000 - start_time > 1000:  # 1秒超时
                    break
                time.sleep(0.001)
            
            if response == test_data:
                end_time = time.time() * 1000
                latency = end_time - start_time
                latencies.append(latency)
                
            # 短暂延迟避免过载
            time.sleep(0.01)
            
            # 显示进度
            if (i + 1) % 10 == 0:
                print(f"完成 {i + 1}/{iterations} 次测试")
        
        if latencies:
            avg_latency = sum(latencies) / len(latencies)
            min_latency = min(latencies)
            max_latency = max(latencies)
            
            print(f"平均延迟: {avg_latency:.2f} ms")
            print(f"最小延迟: {min_latency:.2f} ms")
            print(f"最大延迟: {max_latency:.2f} ms")
            
            # 生成延迟分布直方图
            plt.figure(figsize=(10, 6))
            plt.hist(latencies, bins=20, color='skyblue', edgecolor='black')
            plt.title('UART通信延迟分布')
            plt.xlabel('延迟 (ms)')
            plt.ylabel('频次')
            plt.grid(True, alpha=0.3)
            plt.savefig('latency_distribution.png')
            print("延迟分布图保存为 'latency_distribution.png'")
            
            self.results['latency_avg'] = avg_latency
            self.results['latency_min'] = min_latency
            self.results['latency_max'] = max_latency
            self.results['latency_values'] = latencies
        else:
            print("无法测量延迟，未收到响应")
        
        return latencies
    
    def run_stress_test(self, duration=30):
        """压力测试 - 持续发送随机长度的数据"""
        print(f"\n===== 压力测试 (持续{duration}秒) =====")
        
        start_time = time.time()
        end_time = start_time + duration
        total_sent = 0
        total_received = 0
        errors = 0
        
        print("开始压力测试...")
        with tqdm(total=duration) as pbar:
            while time.time() < end_time:
                # 生成随机长度的随机数据
                length = random.randint(10, 150)
                data = ''.join(random.choices(string.ascii_letters + string.digits, k=length)).encode()
                
                try:
                    # 发送数据
                    sent = self.ser.write(data)
                    total_sent += sent
                    
                    # 给一点时间接收响应
                    time.sleep(0.05)
                    
                    # 读取响应
                    if self.ser.in_waiting > 0:
                        response = self.ser.read(self.ser.in_waiting)
                        total_received += len(response)
                    
                except Exception as e:
                    print(f"发送/接收错误: {e}")
                    errors += 1
                
                # 更新进度条
                elapsed = time.time() - start_time
                pbar.update(min(elapsed, duration) - pbar.n)
        
        reliability = (total_received / total_sent * 100) if total_sent > 0 else 0
        
        print(f"压力测试完成:")
        print(f"总发送: {total_sent} 字节")
        print(f"总接收: {total_received} 字节")
        print(f"可靠性: {reliability:.2f}%")
        print(f"错误次数: {errors}")
        
        self.results['stress_sent'] = total_sent
        self.results['stress_received'] = total_received
        self.results['stress_reliability'] = reliability
        self.results['stress_errors'] = errors
        
        return reliability > 95  # 可靠性超过95%视为通过
    
    def generate_report(self):
        """生成测试报告"""
        print("\n===== 测试报告 =====")
        
        if 'echo_test_success_rate' in self.results:
            print(f"基本回显测试成功率: {self.results['echo_test_success_rate']:.1f}%")
        
        if 'throughput_rate' in self.results:
            print(f"吞吐量: {self.results['throughput_rate']:.2f} KB/s")
        
        if 'latency_avg' in self.results:
            print(f"平均延迟: {self.results['latency_avg']:.2f} ms")
            print(f"延迟范围: {self.results['latency_min']:.2f} - {self.results['latency_max']:.2f} ms")
        
        if 'stress_reliability' in self.results:
            print(f"压力测试可靠性: {self.results['stress_reliability']:.2f}%")
        
        # 保存边界测试结果图形
        if 'boundary_test' in self.results:
            results = self.results['boundary_test']
            sizes = [r['size'] for r in results]
            received = [r['received'] for r in results]
            match_pct = [r['match_percentage'] for r in results]
            
            plt.figure(figsize=(12, 6))
            
            plt.subplot(1, 2, 1)
            plt.plot(sizes, received, 'o-', color='blue')
            plt.plot([min(sizes), max(sizes)], [min(sizes), max(sizes)], '--', color='gray')
            plt.title('发送vs接收字节数')
            plt.xlabel('发送字节数')
            plt.ylabel('接收字节数')
            plt.grid(True, alpha=0.3)
            
            plt.subplot(1, 2, 2)
            plt.plot(sizes, match_pct, 'o-', color='green')
            plt.title('数据匹配率')
            plt.xlabel('发送字节数')
            plt.ylabel('匹配率 (%)')
            plt.ylim(0, 105)
            plt.grid(True, alpha=0.3)
            
            plt.tight_layout()
            plt.savefig('boundary_test_results.png')
            print("边界测试结果图形保存为 'boundary_test_results.png'")
        
        # 保存结果到文件
        with open('uart_test_results.txt', 'w') as f:
            f.write("UART缓冲区测试结果\n")
            f.write("=====================\n\n")
            
            for key, value in self.results.items():
                if key != 'latency_values' and key != 'boundary_test':  # 跳过大型数据列表
                    f.write(f"{key}: {value}\n")
        
        print("\n测试报告已保存至 'uart_test_results.txt'")

    def run_all_tests(self):
        """运行所有测试"""
        if not self.connect():
            return False
            
        try:
            print("\n开始全面UART缓冲区测试...")
            
            # 运行基本回显测试
            self.run_basic_echo_test()
            
            # 运行延迟测试
            self.run_latency_test(iterations=100)
            
            # # 运行边界测试
            # self.run_boundary_test()
            
            
            # 运行吞吐量测试
            # self.run_throughput_test(duration=5)
            
            
            
            # # 运行压力测试
            # self.run_stress_test(duration=20)
            
            # # 生成测试报告
            # self.generate_report()
            
            return True
        except Exception as e:
            print(f"测试过程中出错: {e}")
            import traceback
            traceback.print_exc()
            return False
        finally:
            self.disconnect()


if __name__ == "__main__":
    # 使用正确的COM端口
    COM_PORT = "COM6"  # 请根据实际情况修改
    
    tester = UartBufferTest(port=COM_PORT, baudrate=115200)
    tester.run_all_tests()