import socket
import csv
from datetime import datetime

# Server configuration
HOST = '192.168.178.97'  # Listen on all interfaces
PORT = 50

print(f"Starting TCP server on port {PORT}...")

def main():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((HOST, PORT))
        s.listen(1)
        print("Waiting for connection...")
        conn, addr = s.accept()
        print(f"Connected to {addr}")

        timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
        filename = f"imu_log_{timestamp}.csv"
        
        with conn, open(filename, "w", newline="") as csvfile:
            writer = csv.writer(csvfile)
            # Write our own header
            writer.writerow(["timestamp_us", "raw_mag", "filtered", "ax", "ay", "az"])

            buffer = ""
            while True:
                data = conn.recv(1024)
                if not data:
                    print("Client disconnected.")
                    break

                buffer += data.decode("utf-8", errors='ignore')

                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    line = line.strip()

                    if line == "end":
                        print("End of transmission received.")
                        return
                    
                    if not line:
                        continue

                    try:
                        # Split and validate the line
                        parts = line.split(',')
                        if len(parts) != 6:
                            print(f"Skipping malformed line: {line}")
                            continue
                            
                        # Convert all values to float except the timestamp
                        row = [int(parts[0])] + [float(x) for x in parts[1:]]
                        writer.writerow(row)
                    except ValueError as e:
                        print(f"Skipping invalid line ({e}): {line}")

        print(f"Logging complete. Data saved to {filename}")

if __name__ == "__main__":
    main()