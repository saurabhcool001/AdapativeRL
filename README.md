# AdaptiveRL: Anxiety-Inducing VR with a Reinforcement Learning Agent

This project explores how a Reinforcement Learning (RL) agent can learn to evoke anxiety-related responses in a player through adaptive interactions within a virtual reality (VR) environment. The virtual agent, controlled by the RL policy, adjusts its proximity, eye contact (staring), and movement patterns to maximize a behavioral proxy for player anxiety.

This project is built using "Unreal Engine 5.4.4", the "Meta Quest 3", and "OpenXR" to create a naturalistic and immersive VR experience.

---

## 🚀 Key Features

* **Adaptive Agent Behavior:** An RL agent dynamically controls the virtual agent's actions to create anxiety-inducing scenarios.
* **Real-time Player Monitoring:** The system tracks player behaviors like gaze aversion, head turning as indicators of their emotional state.
* **Immersive VR Experience:** Leverages the capabilities of the Meta Quest 3 and OpenXR for a more realistic and engaging simulation.
* **Dynamic Scenarios:** The primary experience takes place in a virtual elevator, a setting conducive to feelings of confinement and social anxiety.

---

## 🛠️ Getting Started

Follow these instructions to get the project up and running on your local machine.

### **Prerequisites**

Make sure you have the following software installed:

* **Unreal Engine:** Version 5.4.4
* **Visual Studio:** 2022
* **Python:** Version 3.13.3
* **Meta Quest 3:** With developer mode enabled.

### **Installation**

1.  **Clone the Repository:**
    ```bash
    git clone https://github.com/your-username/AdaptiveRL.git
    cd AdaptiveRL
    ```

2.  **Set up the Unreal Engine Project:**
    * Navigate to the `Elevator1` directory.
    * Right-click on the `Elevator1.uproject` file and select "Generate Visual Studio project files".
    * Open the generated `Elevator1.sln` file in Visual Studio 2022 and build the project.
    * Open the `Elevator1.uproject` in Unreal Engine 5.4.4.

---

## ▶️ How to Run

1.  **Start the Python RL Agent:**
    * Run the Python script that starts the UDP server for communication with Unreal Engine.
    ```bash
    python your_rl_agent_script.py
    ```

2.  **Launch the VR Experience:**
    * In the Unreal Engine editor, open the main map: `AdapativeRL/Elevator1/Content/RealElevator/Maps/ElevatorDemo.umap`.
    * Ensure your Meta Quest 3 is connected to your PC via a Link cable or Air Link.
    * Click on the "Play" button in the editor and select "VR Preview" to start the simulation.

---

## 📂 Project Structure

Here's a breakdown of the key files and directories in this project:

* `AdapativeRL/Elevator1/Content/RealElevator/Maps/ElevatorDemo.umap`: The main map where the VR experience takes place.
* `AdapativeRL/Elevator1/Content/RealElevator/Blueprints/BP_Elevator.uasset`: The Blueprint for the elevator's logic and functionality.
* `AdapativeRL/Elevator1/Content/VRTemplate/Blueprints/VRPawn.uasset`: The main Blueprint for the VR user's pawn, handling movement and interactions.
* `AdapativeRL/Elevator1/Source/Elevator1/ElevatorBase`: The C++ source code for the UDP communication between Unreal Engine and the Python RL agent.
* `AdapativeRL/Elevator1/Content/MetaHumans/Hana/BP_Hana.uasset`: The Blueprint for the virtual agent, "Hana," whose behaviors are controlled by the RL agent.

---

## Current Progress Stage

1. Scenario 
	User wear the headset -> walk towards the lift -> press a button of call a lift
	Lift comes up -> opens the door for few second -> user walk inside the lift
	User press floor label button -> Lift starts to move upside -> stop at every floor
	Lift arrive at floor 1 -> (before opening door) Spawn virtual agent between 1 and 5 -> after some seconds lift close the door and move up again and continue same process
	
2. Communication side
	2.1 Create prototype script of Q-learning algorithm in Python
	2.2 Create prototype script for UDP communication in Python
	2.3 Both script communicating with Unreal Engine 5.4.4
