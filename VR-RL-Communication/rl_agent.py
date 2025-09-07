# rl_agent.py
import random

class RLAgent:
    """
    This class implements a Q-learning agent to learn and decide actions
    based on the principles outlined in the research proposal.
    """
    # --- THESE ARE THE NEW, UPDATED PARAMETERS FOR FASTER LEARNING ---
    def __init__(self, learning_rate=0.5, discount_factor=0.9, exploration_rate=0.8, exploration_decay=0.9, min_epsilon=0.1):
        """
        Initializes the RL agent.
        - learning_rate (alpha): How quickly the agent learns.
        - discount_factor (gamma): How much it values future rewards.
        - exploration_rate (epsilon): The initial probability of choosing a random action.
        - exploration_decay: The rate at which epsilon decreases.
        """
        self.q_table = {}  # The Q-table will be a dictionary mapping (state, action) -> value.
        self.alpha = learning_rate
        self.gamma = discount_factor
        self.epsilon = exploration_rate
        self.epsilon_decay = exploration_decay
        self.min_epsilon = min_epsilon # The minimum exploration rate
        
        # Define the action space as per my project document 
        self.actions = [
            "move_closer",
            "move_away",
            "block_door",
            "make_eye_contact",
            "go_out_of_lift",
            "come_in_lift"
        ]

    def get_q_value(self, state, action):
        """Safely retrieves a Q-value from the table, returning 0.0 if it doesn't exist."""
        return self.q_table.get((state, action), 0.0)

    def choose_action(self, state, valid_actions=None):
        """
        Chooses an action using an epsilon-greedy policy from a list of valid actions.
        """
        # --- FIX: Use the provided valid_actions list. If it's None, use the default list. ---
        # The key change is how we handle an EMPTY list.
        action_space = self.actions if valid_actions is None else valid_actions

        # --- THIS IS THE CRITICAL FIX ---
        # If the action space is empty (e.g., at Floor 0), do nothing.
        if not action_space:
            print("[RL AGENT] No valid actions available. Doing nothing.")
            return "do_nothing" # This action will be safely ignored by C++

        if random.random() < self.epsilon:
            # Exploration: choose a random action from the VALID list
            action = random.choice(action_space)
            print(f"[RL AGENT] Exploring: Chose random valid action '{action}' (epsilon: {self.epsilon:.2f})")
        else:
            # Exploitation: choose the best action from the VALID list
            q_values = {a: self.get_q_value(state, a) for a in action_space}
            max_q = max(q_values.values())
            
            best_actions = [a for a, q in q_values.items() if q == max_q]
            action = random.choice(best_actions)
            print(f"[RL AGENT] Exploiting: Chose best valid action '{action}' with Q-value {max_q:.2f}")

        return action

    def update_q_table(self, state, action, reward, next_state):
        """
        Updates the Q-table using the Bellman equation as defined in the proposal.
        Q(s,a) <- Q(s,a) + alpha * [r + gamma * max_a'(Q(s',a')) - Q(s,a)]
        """
        # Get the old Q-value for the state-action pair
        old_value = self.get_q_value(state, action)

        # Find the maximum Q-value for the next state (the agent's estimate of future reward)
        next_max_q = max([self.get_q_value(next_state, a) for a in self.actions])

        # Calculate the new Q-value
        new_value = old_value + self.alpha * (reward + self.gamma * next_max_q - old_value)

        # Update the Q-table
        self.q_table[(state, action)] = new_value
        print(f"[RL AGENT] Q-table updated for state '{state}' and action '{action}'. New value: {new_value:.2f}")

        # Decay epsilon to reduce exploration over time
        if self.epsilon > self.min_epsilon:
            self.epsilon *= self.epsilon_decay