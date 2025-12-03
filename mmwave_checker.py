import serial
import time

# DFRobot SEN0395 Configuration
SERIAL_PORT = '/dev/ttyAMA0'  # Change if needed: /dev/ttyUSB0, /dev/serial0
BAUD_RATE = 115200  # SEN0395 default is 115200

def send_command(ser, command, wait_time=0.3):
    """Send command to SEN0395 and get response"""
    print(f"  → Sending: {command.strip()}")
    ser.write((command + '\n').encode())
    time.sleep(wait_time)
    response = ""
    if ser.in_waiting > 0:
        response = ser.read(ser.in_waiting).decode('utf-8', errors='ignore').strip()
        if response:
            print(f"  ← Response: {response}")
    return response

def setup_sen0395(ser):
    """Configure SEN0395 for first time use"""
    print("\n" + "=" * 60)
    print("CONFIGURING SENSOR (First Time Setup)")
    print("=" * 60 + "\n")
    
    # Step 1: Enter configuration mode (sensor starts in this mode when new)
    print("Step 1: Entering configuration mode...")
    send_command(ser, "sensorStop")
    time.sleep(0.5)
    
    # Step 2: Reset to factory defaults (optional but recommended)
    print("\nStep 2: Resetting to factory defaults...")
    send_command(ser, "resetSystem")
    time.sleep(2)  # Wait for reset to complete
    
    # Step 3: Set detection mode and sensitivity
    print("\nStep 3: Configuring detection parameters...")
    
    # Set output mode to latency (real-time detection)
    send_command(ser, "outputLatency")
    
    # Set sensitivity (1-9, where 9 is most sensitive, 5 is default)
    send_command(ser, "setSensitivity 7")  # Medium-high sensitivity
    
    # Set detection range (optional, 0.3m to 10m)
    send_command(ser, "setRange 0.3 6")  # Detect from 0.3m to 6m
    
    # Step 4: Save configuration
    print("\nStep 4: Saving configuration...")
    send_command(ser, "saveCfg")
    time.sleep(1)
    
    # Step 5: Start sensor in output mode
    print("\nStep 5: Starting sensor output mode...")
    send_command(ser, "sensorStart")
    time.sleep(1)
    
    print("\n✓ Configuration complete! Sensor is now in output mode.\n")

def check_sen0395():
    """Check DFRobot SEN0395 mmWave sensor connection and setup"""
    
    print("=" * 60)
    print("DFRobot SEN0395 mmWave Sensor - Setup & Checker")
    print("=" * 60)
    print(f"Port: {SERIAL_PORT} | Baud: {BAUD_RATE}\n")
    
    try:
        # Open serial connection
        ser = serial.Serial(
            port=SERIAL_PORT,
            baudrate=BAUD_RATE,
            timeout=1
        )
        
        print("✓ Serial port opened successfully")
        
        # Clear any existing data
        ser.reset_input_buffer()
        time.sleep(0.5)
        
        # Check if sensor is already outputting data
        print("\nChecking if sensor is already configured...")
        time.sleep(1)
        
        if ser.in_waiting > 0:
            data = ser.read(ser.in_waiting).decode('utf-8', errors='ignore')
            print(f"✓ Sensor is already sending data!")
            print(f"  Sample: {data[:100]}...")
            skip_setup = True
        else:
            print("○ No data detected - sensor needs configuration")
            skip_setup = False
        
        # Setup if needed
        if not skip_setup:
            user_input = input("\nConfigure sensor now? (y/n): ").lower()
            if user_input == 'y':
                setup_sen0395(ser)
            else:
                print("\nSkipping setup. You can run this script again to configure.")
                ser.close()
                return
        
        # Monitor output
        print("\n" + "=" * 60)
        print("MONITORING SENSOR OUTPUT (10 seconds)")
        print("=" * 60)
        print("Wave your hand near the sensor to test detection...\n")
        
        ser.reset_input_buffer()
        start_time = time.time()
        line_count = 0
        detection_count = 0
        
        while time.time() - start_time < 10:
            if ser.in_waiting > 0:
                data = ser.readline().decode('utf-8', errors='ignore').strip()
                if data:
                    line_count += 1
                    
                    # Parse JYBSS format: $JYBSS,presence,motion,distance,...
                    if data.startswith('$JYBSS'):
                        parts = data.split(',')
                        if len(parts) >= 3:
                            presence = parts[1]  # 0=no one, 1=someone present
                            motion = parts[2]    # 0=stationary, 1=moving
                            
                            if presence == '1':
                                detection_count += 1
                                status = "🟢 DETECTED"
                                if motion == '1':
                                    status += " (Moving)"
                                else:
                                    status += " (Stationary)"
                            else:
                                status = "○ No presence"
                            
                            print(f"  {status} | {data}")
                    else:
                        print(f"  {data}")
                    
                    if line_count >= 30:  # Limit output
                        print("  ... (output continues)")
                        break
        
        print(f"\n✓ Received {line_count} data lines")
        print(f"✓ Detections: {detection_count}")
        
        if line_count == 0:
            print("\n⚠ WARNING: No data received!")
            print("  Try running setup again or check wiring.")
        
        ser.close()
        print("\n" + "=" * 60)
        print("✓ Test complete!")
        print("=" * 60)
        
    except serial.SerialException as e:
        print(f"\n✗ Serial port error: {e}\n")
        print("Troubleshooting:")
        print("  1. List ports: ls /dev/tty*")
        print("  2. Check permissions: sudo usermod -a -G dialout $USER")
        print("  3. Enable UART: sudo raspi-config → Interface → Serial")
        print("  4. Check wiring: TX→RX, RX→TX, GND→GND, VCC→5V")
        
    except KeyboardInterrupt:
        print("\n\n✓ Test interrupted by user")
        ser.close()
        
    except Exception as e:
        print(f"\n✗ Unexpected error: {e}")

if __name__ == "__main__":
    check_sen0395()