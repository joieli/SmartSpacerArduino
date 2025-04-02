import serial
import pandas as pd
from datetime import datetime

# Configure serial port (change 'COM3' to your Arduino's port)
SERIAL_PORT = 'COM9'  # Windows
# SERIAL_PORT = '/dev/ttyACM0'  # Linux/Mac
BAUD_RATE = 9600  # Match your Arduino's Serial.begin()

# Output file (CSV format for Excel)
OUTPUT_FILE = f"sensor_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.csv"

def main():
    # Open serial connection
    ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
    print(f"Logging data from {SERIAL_PORT} to {OUTPUT_FILE}... Press Ctrl+C to stop.")
    
    data = []
    try:
        while True:
            line = ser.readline().decode('utf-8').strip()
            if line:  # Only process non-empty lines
                print(line)  # Optional: Show live data in console
                timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S.%f')
                data.append([timestamp] + line.split(','))  # Split CSV-style data
    except KeyboardInterrupt:
        print("\nStopping logger...")
        ser.close()
        
        # Save to CSV
        df = pd.DataFrame(data, columns=['SoftwareTimestamp','hardwaretime', "Temperature",'Flow', 'Pressure', 'TimeDuration'])
        df.to_csv(OUTPUT_FILE, index=False)
        print(f"Data saved to {OUTPUT_FILE}")

if __name__ == "__main__":
    main()