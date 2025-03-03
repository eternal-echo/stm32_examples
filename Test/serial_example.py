import serial
import time
import threading

def rx_thread(ser):
    while True:
        try:
            # 持续读取数据
            if ser.in_waiting:
                data = ser.readline().decode('utf-8')
                if data:
                    print(f"Received: {data}")
        except Exception as e:
            print(f"Rx Error: {e}")
            break

def tx_thread(ser):
    while True:
        try:
            # 从控制台获取输入并发送
            message = input("Enter message to send (or 'quit' to exit): ")
            if message.lower() == 'quit':
                break
            ser.write((message + '\n').encode('utf-8'))
        except Exception as e:
            print(f"Tx Error: {e}")
            break

def main():
    ser = serial.Serial('COM6', 115200)
    flag = ser.is_open
    print(flag)
    if not flag:
        print("Failed to open serial port")
        return
    
    # 创建并启动接收线程
    rx = threading.Thread(target=rx_thread, args=(ser,))
    rx.daemon = True  # 设置为守护线程，主线程结束时会自动结束
    rx.start()
    
    # 创建并启动发送线程
    tx = threading.Thread(target=tx_thread, args=(ser,))
    tx.start()
    
    # 等待发送线程结束
    tx.join()
    
    # 关闭串口
    ser.close()

if __name__ == "__main__":
    main()
