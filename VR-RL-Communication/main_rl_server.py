# main_rl_server.py
import socket
import json
import time
from rl_agent import RLAgent

# --- CONFIGURATION ---
UNREAL_IP = "127.0.0.1"
UNREAL_PORT_SEND = 5005   # Port to send actions TO Unreal
PYTHON_PORT_RECEIVE = 6006 # Port to receive states FROM Unreal

# --- SETUP ---
# Setup UDP socket for sending actions to Unreal
send_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# Setup UDP socket for receiving states from Unreal
recv_sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
recv_sock.bind(("0.0.0.0", PYTHON_PORT_RECEIVE))

# Instantiate our Reinforcement Learning agent
agent = RLAgent()

# Variables to hold the most recent state and reward info
last_state = None
last_action = None

def calculate_reward(state_data):
    """
    Calculates a reward based on the player's behavior (the state).
    This is where you implement your reward function.
    """
    # Example reward logic:
    # Positive reward if player shows anxiety (retreats, panics)
    # Negative reward if player shows comfort (no movement, looks at agent)
    reward = 0.0
    if state_data.get("panic_triggered") == True:
        reward += 10.0  # Strong positive reward for clear anxiety signal
    if state_data.get("distance_to_virtual_agent") == "far":
        reward += 5.0   # Positive reward for retreating
    if state_data.get("gaze_alignment") == False:
        reward += 2.0   # Small reward for looking away

    if reward == 0.0:
        reward = -1.0 # Small negative reward for neutral/habituated behavior

    print(f"[REWARD CALC] Calculated reward: {reward}")
    return reward

def main_loop():
    """Main loop that listens for state, decides action, and sends it."""
    global last_state, last_action
    
    print(f"[SERVER RUNNING] Listening for Unreal state data on port {PYTHON_PORT_RECEIVE}")
    
    while True:
        # 1. RECEIVE STATE FROM UNREAL
        data, addr = recv_sock.recvfrom(1024) # This will block until data is received
        state_json = data.decode('utf-8')
        state_data = json.loads(state_json) # Assumes Unreal sends a JSON string
        
        # Discretize the state into a simple, hashable format (a tuple of values)
        # This must match the features defined in your proposal 
        current_state = (
            state_data.get("distance_to_virtual_agent", "far"),
            state_data.get("gaze_alignment", False),
            state_data.get("head_turn_angle", "aligned"),
            state_data.get("time_since_last_reaction", "long"),
            state_data.get("panic_triggered", False)
        )
        print(f"\n[RECEIVED from Unreal] State: {current_state}")

        # 2. UPDATE Q-TABLE (if this isn't the first action)
        if last_state is not None and last_action is not None:
            reward = calculate_reward(state_data)
            agent.update_q_table(last_state, last_action, reward, current_state)

        # 3. CHOOSE NEXT ACTION
        action_to_perform = agent.choose_action(current_state)

        # 4. SEND ACTION TO UNREAL
        send_sock.sendto(action_to_perform.encode('utf-8'), (UNREAL_IP, UNREAL_PORT_SEND))
        print(f"[SENT to Unreal] Action: {action_to_perform}")

        # 5. SAVE CURRENT STATE AND ACTION FOR THE NEXT UPDATE
        last_state = current_state
        last_action = action_to_perform
        
        # Optional: Save the Q-table periodically
        # with open("q_table.json", "w") as f:
        #     json.dump({str(k): v for k, v in agent.q_table.items()}, f)


if __name__ == "__main__":
    try:
        main_loop()
    except KeyboardInterrupt:
        print("\n[SERVER STOPPED]")
    finally:
        send_sock.close()
        recv_sock.close()