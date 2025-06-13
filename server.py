import socket
import csv

# Server configuration
HOST = socket.gethostbyname(socket.gethostname())  # listen on all available interfaces
PORT = 50  # the same port number used in the M5StickC Plus code

print(f"Starting TCP server on port {PORT}...")

# Setup socket
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.bind((HOST, PORT))
    s.listen(1)
    conn, addr = s.accept()
    print(f"Connected to {addr}")

    with conn, open("imu_log.csv", "w", newline="") as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(["timestamp_us", "ax", "ay", "az", "filtered_mag"])  # Header

        buffer = ""
        while True:
            data = conn.recv(1024)
            if not data:
                print("Client disconnected.")
                break

            buffer += data.decode("utf-8")

            # Split complete lines
            while '\n' in buffer:
                line, buffer = buffer.split('\n', 1)

                if line.strip() == "end":
                    print("End of transmission.")
                    break

                try:
                    row = [float(x) for x in line.strip().split(',')]
                    if len(row) == 5:
                        writer.writerow(row)
                except ValueError:
                    print(f"Skipping invalid line: {line}")

        print("Logging complete. Data saved to imu_log.csv")
