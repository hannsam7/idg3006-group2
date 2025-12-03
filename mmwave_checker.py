import serial
import time

# DFRobot SEN0395 Configuration
SERIAL_PORT = '/dev/ttyAMA0'  # Change if needed: /dev/ttyUSB0, /dev/serial0
BAUD_RATE = 115200  # SEN0395 default is 115200

def send_command(ser, command):
    """Send command to SEN0395 and get response"""
    ser.write(command.encode())
    time.sleep(0.2)
    response = ""
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
    return response

def check_sen0395():
    """Check DFRobot SEN0395 mmWave sensor connection and status"""
    
    print("DFRobot SEN0395 mmWave Sensor Checker")
    print("=" * 60)
    print(f"Port: {SERIAL_PORT} | Baud: {BAUD_RATE}")
    print("-" * 60)
    
    try:
        # Open serial connection
        ser = serial.Serial(
            port=SERIAL_PORT,
            baudrate=BAUD_RATE,
            timeout=1
        )
        
        print("✓ Serial port opened successfully\n")
        
        # Clear any existing data
        ser.reset_input_buffer()
        time.sleep(0.5)
        
        # Test 1: Check for any incoming data
        print("Test 1: Checking for incoming data...")
        start_time = time.time()
        data_count = 0
        
        while time.time() - start_time < 3:
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8', errors='ignore').strip()
                if data:
                    data_count += 1
                    print(f"  → Data: {data}")
                    if data_count >= 5:  # Show first 5 messages
                        break
        
        if data_count > 0:
            print(f"✓ Received {data_count} data packets\n")
        else:
            print("✗ No data received\n")
        
        # Test 2: Try to get sensor info
        print("Test 2: Querying sensor information...")
        
        # Get hardware model
        response = send_command(ser, "getVersion\n")
        if response:
            print(f"  Version: {response.strip()}")
        
        # Get sensitivity
        response = send_command(ser, "getSensitivity\n")
        if response:
            print(f"  Sensitivity: {response.strip()}")
        
        # Get detection status
        response = send_command(ser, "sensorStatus\n")
        if response:
            print(f"  Status: {response.strip()}")
        
        print("\n" + "-" * 60)
        
        # Test 3: Monitor detection for 5 seconds
        print("Test 3: Monitoring for 5 seconds (wave your hand)...\n")
        ser.reset_input_buffer()
        start_time = time.time()
        detections = 0
        
        while time.time() - start_time < 5:
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8', errors='ignore').strip()
                if data:
                    print(f"  {data}")
                    if "target" in data.lower() or "occupy" in data.lower():
                        detections += 1
        
        print(f"\n{'✓' if detections > 0 else '○'} Detection events: {detections}")
        
        ser.close()
        print("\n" + "=" * 60)
        print("✓ Connection test complete!")
        
        if data_count == 0:
            print("\n⚠ Troubleshooting tips:")
            print("  • Check wiring (TX→RX, RX→TX, GND→GND, 5V→VCC)")
            print("  • Verify correct port (try: ls /dev/tty*)")
            print("  • Check permissions: sudo usermod -a -G dialout $USER")
            print("  • Try different baud rates: 9600, 115200, 256000")
        
    except serial.SerialException as e:
        print(f"✗ Serial port error: {e}\n")
        print("Troubleshooting:")
        print("  1. List ports: ls /dev/tty*")
        print("  2. Check permissions: sudo chmod 666 " + SERIAL_PORT)
        print("  3. Add user to dialout: sudo usermod -a -G dialout $USER")
        print("  4. Enable UART on Raspberry Pi if using GPIO pins")
        
    except KeyboardInterrupt:
        print("\n\n✓ Test interrupted by user")
        
    except Exception as e:
        print(f"✗ Unexpected error: {e}")

if __name__ == "__main__":
    check_sen0395()