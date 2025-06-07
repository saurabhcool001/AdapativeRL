import socket
import threading
import time

# CONFIGURATION
UNREAL_IP = "127.0.0.1"    # IP address of Unreal's receiver
UNREAL_PORT = 5005         # Unreal's listening port
PYTHON_PORT = 6006         # This script's own receiving port
MESSAGE = "TriggerLift"    # Message to send to Unreal

# Setup UDP socket for sending
send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Setup UDP socket for receiving
recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.bind(("0.0.0.0", PYTHON_PORT))
recv_sock.setblocking(False)

# Simulation flag
trigger_pressed = False

def simulate_trigger_press():
    """Simulates a left-hand VR trigger press at intervals."""
    global trigger_pressed
    while True:
        time.sleep(5)  # simulate trigger every 5 seconds
        trigger_pressed = True

def receive_from_unreal():
    """Receives response messages from Unreal."""
    while True:
        try:
            data, addr = recv_sock.recvfrom(1024)
            response = data.decode()
            print(f"[RECEIVED from Unreal] {response}")
        except BlockingIOError:
            time.sleep(0.01)

def main_loop():
    """Main loop that sends message on trigger press."""
    global trigger_pressed
    while True:
        if trigger_pressed:
            send_sock.sendto(MESSAGE.encode(), (UNREAL_IP, UNREAL_PORT))
            print(f"[SENT to Unreal] {MESSAGE}")
            trigger_pressed = False
        time.sleep(0.05)

# Start trigger simulation and listener thread
threading.Thread(target=simulate_trigger_press, daemon=True).start()
threading.Thread(target=receive_from_unreal, daemon=True).start()

if __name__ == "__main__":
    print(f"[RUNNING] Sending to {UNREAL_IP}:{UNREAL_PORT}, Listening on {PYTHON_PORT}")
    try:
        main_loop()
    except KeyboardInterrupt:
        print("\n[STOPPED]")
