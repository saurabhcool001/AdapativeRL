# main_rl_server.py
import socket
import json
import time
from rl_agent import RLAgent
from datetime import datetime
import os
import random

# --- CONFIGURATION ---
UNREAL_IP = "127.0.0.1"
UNREAL_PORT_SEND = 5005
PYTHON_PORT_RECEIVE = 6006

# --- SETUP ---
send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.bind(("0.0.0.0", PYTHON_PORT_RECEIVE))
agent = RLAgent()

# Logging Setup
log_directory = 'data'
os.makedirs(log_directory, exist_ok=True)
timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")
log_filename = os.path.join(log_directory, f"python_rl_log_{timestamp}.jsonl")
print(f"[LOGGING] Session data will be saved to: {log_filename}")

# --- THIS IS THE NEW LINE ---
# Create an empty file immediately so it always exists.
with open(log_filename, 'w') as f:
    pass # This creates the file and then closes it.

last_state = None
last_action = None
# --- NEW: Variable to track previous joystick state ---
last_joystick_y = 0.0

# --- NEW REWARD FUNCTION BASED ON JOYSTICK INPUT ---
def calculate_joystick_reward(state_data):
    """
    Calculates a strong reward based ONLY on user joystick input.
    This provides direct, explicit feedback to the agent.
    """
    joystick_y = state_data.get("joystick_y", 0.0)
    
    # Check for significant joystick movement to avoid rewarding controller drift
    if joystick_y > 0.5:
        print(f"[REWARD CALC] User approved of last action (Joystick Up). Strong positive reward.")
        return 10.0  # Strong positive reward for pushing joystick UP
    elif joystick_y < -0.5:
        print(f"[REWARD CALC] User disapproved of last action (Joystick Down). Strong negative reward.")
        return -10.0 # Strong negative reward for pushing joystick DOWN
    else:
        return 0.0   # No reward if the joystick is not being actively used

def main_loop():
    """Main loop that listens for state, learns from joystick feedback, and decides actions."""
    global last_state, last_action, last_joystick_y
    
    print(f"[SERVER RUNNING] Listening for Unreal state data on port {PYTHON_PORT_RECEIVE}")
    
    while True:
        try:
            # 1. RECEIVE STATE
            data, addr = recv_sock.recvfrom(1024)
            state_json = data.decode('utf-8')
            if not state_json:
                continue
            state_data = json.loads(state_json)
            
            # The state representation remains the same
            num_agents = state_data.get("num_agents_in_lift", 0)
            if num_agents == 0:
                current_state = ("lift_empty",)
            else:
                current_state = (
                    state_data.get("distance_to_virtual_agent", "unknown"),
                    state_data.get("gaze_alignment", "unknown")
                )
            
            print(f"\n[RECEIVED from Unreal] State representation: {current_state}")
            
            # 2. LEARN FROM USER'S JOYSTICK FEEDBACK
            current_joystick_y = state_data.get("joystick_y", 0.0)
            
            # This condition is only True on the first frame the joystick is pushed
            user_is_giving_feedback = (abs(current_joystick_y) > 0.5) and (abs(last_joystick_y) < 0.5)

            if user_is_giving_feedback and last_state is not None and last_action is not None:
                reward = calculate_joystick_reward(state_data)
                agent.update_q_table(last_state, last_action, reward, current_state)

                # Log the learning experience
                log_entry = {
                    "timestamp": datetime.now().isoformat(),
                    "state": last_state,
                    "action": last_action,
                    "reward": reward,
                    "next_state": current_state
                }
                with open(log_filename, 'a') as f:
                    f.write(json.dumps(log_entry) + '\n')

            # 3. CHOOSE NEXT ACTION
            valid_actions = state_data.get("valid_actions")
            action_to_perform = agent.choose_action(current_state, valid_actions=valid_actions)

            # 4. SEND ACTION TO UNREAL
            send_sock.sendto(action_to_perform.encode('utf-8'), (UNREAL_IP, UNREAL_PORT_SEND))
            print(f"[SENT to Unreal] Action: {action_to_perform}")

            # 5. SAVE FOR NEXT LOOP
            last_state = current_state
            last_action = action_to_perform
            last_joystick_y = current_joystick_y # Update the joystick state for the next frame

        except Exception as e:
            print(f"[ERROR] An unexpected error occurred: {e}")
            continue

if __name__ == "__main__":
    try:
        main_loop()
    except KeyboardInterrupt:
        print("\n[SERVER STOPPED]")
    finally:
        send_sock.close()
        recv_sock.close()