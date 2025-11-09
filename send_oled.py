import time
import serial
from serial.tools import list_ports

def find_port():
    for p in list_ports.comports():
        if "usbmodem" in p.device or "usbserial" in p.device:
            return p.device
    raise RuntimeError("arduino port not found; check Tools→Port in Arduino IDE or run: ls /dev/cu.usb*")

PORT = find_port()
print(f"Connecting to Arduino on {PORT}...")

try:
    ser = serial.Serial(PORT, 115200, timeout=2)
except Exception as e:
    print(f"Error opening port: {e}")
    print("Try: 1) Close Arduino IDE, 2) Unplug/replug USB, 3) Re-upload Arduino code")
    exit(1)

# Arduino auto-resets on open; give it a moment
print("Waiting for Arduino to initialize...")
time.sleep(3)

# Read the greeting line if present
try:
    greeting = ser.readline().decode(errors="ignore").strip()
    print("Arduino says:", greeting)
except Exception as e:
    print(f"Warning: Could not read greeting: {e}")
    print("Continuing anyway...")

# Send text to show on OLED (must end with \n)
msg = "Hello from Python!"
print(f"\nSending: '{msg}'")
ser.write((msg + "\n").encode())

# Read all responses (Arduino may send multiple lines)
time.sleep(0.5)
while ser.in_waiting:
    response = ser.readline().decode(errors="ignore").strip()
    print(f"  Arduino: {response}")

print("\n✓ Message should be visible on OLED display")
print("Waiting 3 seconds before clearing...")
time.sleep(3)

# Clear the display
print("\nSending: 'CLEAR'")
ser.write(b"CLEAR\n")
time.sleep(0.5)
while ser.in_waiting:
    response = ser.readline().decode(errors="ignore").strip()
    print(f"  Arduino: {response}")

print("\n✓ Display cleared")
ser.close()