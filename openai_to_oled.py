import os
import time
import serial
from serial.tools import list_ports
from openai import OpenAI

# Initialize OpenAI client (uses OPENAI_API_KEY from environment)
client = OpenAI()

# Find Arduino port
def find_port():
    for p in list_ports.comports():
        if "usbmodem" in p.device or "usbserial" in p.device:
            return p.device
    raise RuntimeError("Arduino port not found")

# Connect to Arduino
PORT = find_port()
print(f"Connecting to Arduino on {PORT}...")
ser = serial.Serial(PORT, 115200, timeout=2)
time.sleep(3)  # Wait for Arduino to initialize
print("Connected!\n")

# Clear any startup messages
ser.reset_input_buffer()

# Send prompt to OpenAI
print("=" * 50)
test_prompt = "Say hello as a plant in a 5 word sentence."
print(f"Sending to OpenAI: '{test_prompt}'")
print("=" * 50)

try:
    response = client.chat.completions.create(
        model="gpt-4o-mini",  # Using the most cost-effective model
        messages=[
            {"role": "user", "content": test_prompt}
        ],
        max_tokens=50
    )
    
    # Get the response text
    ai_response = response.choices[0].message.content.strip()
    print(f"\nOpenAI Response:\n{ai_response}\n")
    
    # Display on OLED
    print("=" * 50)
    print("Sending to OLED display...")
    print("=" * 50)
    
    ser.write((ai_response + "\n").encode())
    time.sleep(0.5)
    
    # Read Arduino's acknowledgment
    while ser.in_waiting:
        ack = ser.readline().decode(errors="ignore").strip()
        print(f"Arduino: {ack}")
    
    print("\n✓ Response displayed on OLED!")
    print("Keeping message on screen for 10 seconds...")
    time.sleep(10)
    
    # Clear display
    print("\nClearing display...")
    ser.write(b"CLEAR\n")
    time.sleep(0.5)
    while ser.in_waiting:
        ack = ser.readline().decode(errors="ignore").strip()
        print(f"Arduino: {ack}")
    
    print("✓ Done!")

except Exception as e:
    print(f"\nError: {e}")

finally:
    ser.close()
    print("Connection closed.")

