import os
import time
import serial
from serial.tools import list_ports
from openai import OpenAI

# Initialize OpenAI client
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
ser.reset_input_buffer()
print("✓ Connected to Arduino!\n")

# Function to send message to OLED
def display_on_oled(message):
    ser.write((message + "\n").encode())
    time.sleep(0.3)
    # Read Arduino's acknowledgment
    while ser.in_waiting:
        ser.readline()  # Clear the buffer

# Function to clear OLED
def clear_oled():
    ser.write(b"CLEAR\n")
    time.sleep(0.3)
    while ser.in_waiting:
        ser.readline()

# Conversation history for context
# System prompt to keep responses short for OLED display
conversation_history = [
    {
        "role": "system",
        "content": "You're a silly assistant living on a tiny Arduino JMDO.96D-1 OLED display. Keep all responses under 7 words to fit on a small OLED display. Be extremely concise. Do NOT use line breaks or special symbols. Do NOT use lists. You can't do anything complicated like return code, JSON, etc."
    }
]

print("=" * 60)
print("Interactive OpenAI Chat with OLED Display")
print("=" * 60)
print("Type your message and press Enter to chat with AI.")
print("The AI's response will appear on your OLED display.")
print("Type 'quit', 'exit', or 'q' to end the chat.")
print("=" * 60)
print()

try:
    while True:
        # Get user input
        user_input = input("You: ").strip()
        
        # Check for exit commands
        if user_input.lower() in ['quit', 'exit', 'q', '']:
            print("\nGoodbye!")
            clear_oled()
            break
        
        # Add user message to history
        conversation_history.append({"role": "user", "content": user_input})
        
        print("\n⏳ Thinking...")
        
        try:
            # Call OpenAI API
            response = client.chat.completions.create(
                model="gpt-4o-mini",
                messages=conversation_history,
                max_tokens=50  # Reduced for short OLED-friendly responses
            )
            
            # Get the AI response
            ai_response = response.choices[0].message.content.strip()
            
            # Add AI response to history
            conversation_history.append({"role": "assistant", "content": ai_response})
            
            # Print to console
            print(f"\nOAI: {ai_response}\n")
            
            # Display on OLED
            display_on_oled(ai_response)
            
            print("-" * 60)
            
        except Exception as e:
            print(f"\n❌ Error calling OpenAI: {e}\n")
            # Remove the user message from history if API call failed
            conversation_history.pop()

except KeyboardInterrupt:
    print("\n\nChat interrupted. Goodbye!")
    clear_oled()

finally:
    ser.close()
    print("Connection closed.")

