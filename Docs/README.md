# SD1 Assignment 3: Starship Prototype with Xbox Controller Support

## Overview
This repository contains the implementation of the Starship Prototype, 
including simple physics, behavior, rendering, and management of game entities. 
In this version, I extend the prototype to support Xbox Controllers and implement debris effects, 
attract mode, limited lives/waves, and real-time delta seconds for variable frame rates, enhancing the gameplay experience.

## Getting Started

### Prerequisites
- **Operating System:** Windows 10 or later
- **Dependencies:** Ensure you have all the source files from the Engine folder and the XInput API for Xbox controller support.

### Building the Project
1. **Download/Clone the Repository:**
   Clone or download the repository to your local machine.

2. **Open Solution:**
   Open the `Starship.sln` file located in the root directory with Visual Studio 2022.

3. **Build:**
   Build the project using the Release configuration. 
   Ensure no errors occur during the build process.

### Running the Game
- **Executable Location:** After a successful build, you can find the executable at `Release/Starship_x64.exe`.

- **Controls:**
  - **Keyboard Controls:**
    - **S/F**: Turn the PlayerShip left/right.
    - **E**: Thrust PlayerShip forward.
    - **Space**: Fire a bullet.
    - **N**: Respawn PlayerShip if dead. / Exit attract mode.
    - **I**: Spawn a new asteroid (if under limit).
    - **F1**: Toggle developer mode (shows debug info).
    - **F4**: Cheat code to enter next wave.
    - **F8**: Hard-restart the game.
    - **P**: Toggle pause/unpause.
    - **T**: Slow down time to 10% normal speed.
    - **O**: Unpause for one frame and then pause.
    - **Q**: Quit the app and close the window.

  - **Xbox Controller Controls:**
    - **Left Joystick**: Move the PlayerShip.
    - **A Button**: Fire a bullet.
    - **Down Button**: Cheat code to enter next wave.
    - **Back Button**: Go back to attract mode. / If already in attract mode, exit the game.
    - **Start Button**: Exit attract mode. / Respawn PlayerShip if dead.
    

### Features Implemented
- **Entities Management:**
  - Game manages all entities including PlayerShip, Asteroids, Bullets, and Debris.
  - Entities interact based on game rules (e.g., collision, health).

- **PlayerShip Controls:**
  - Turn with S/F keys or Left Joystick.
  - Accelerate with E key or Left Joystick.
  - Fire bullets with Space key or A Button.
  - Respawn with N key or Start Button.

- **Asteroids and Bullets:**
  - Asteroids spawn at random positions outside of the screen and drift with fixed speed.
  - Bullets are fired from the PlayerShip and move in its forward direction.

- **Debris Effects:**
  - Upon collisions, asteroids and the PlayerShip generate debris.
  - Debris includes small fragments that scatter and drift based on physics.
  - Debris features animated effects, such as fading and rotation, to enhance visual feedback.

- **Attract Mode:**
  - The game includes an attract mode that displays animated visuals to entice players to start a new game.

- **Limited Lives/Waves:**
  - Players start with 4 lives.
  - The game progresses through waves of increasing difficulty, with each wave introducing new challenges and enemies.

- **Variable FPS Real-Time Delta Seconds:**
  - The game uses real-time delta seconds to accommodate variable frame rates.
  - This ensures smooth gameplay by adjusting movement and animations based on the actual time elapsed between frames.

- **Debug/Developer Features:**
  - Developer mode displays additional information about entities.
  - Error dialogues for exceeding entity limits.

- **Xbox Controller Support:**
  - Support for up to 4 Xbox Controllers using Microsoft’s XInput API.
  - Each controller manages its buttons, triggers, and joystick states.
  - Includes dead-zone corrections for analog sticks.

## Implementation Details

1. **Input System:**
   - An `InputSystem` class is responsible for managing input from the Xbox Controllers and keyboard.
   - Each controller is represented by an `XboxController` class, which handles its state and updates.

2. **Controller Update Logic:**
   - The `InputSystem::BeginFrame()` method updates the state of each Xbox Controller every frame.
   - The `XboxController::Update()` method checks for connection status and updates input states accordingly.

3. **Debris Generation:**
   - When collisions occur, the game creates debris objects that simulate realistic scattering.
   - Each debris piece has properties for size, speed, and lifetime, with animated effects for a more immersive experience.

4. **Attract Mode Logic:**
   - If player's life is zero, the game enters attract mode after a specified idle period.
   - Player input exits the attract mode and starts the game.

5. **Limited Lives/Waves Management:**
   - Players have a fixed number of lives that decrease upon destruction of the PlayerShip.
   - The game transitions through waves, with increased difficulty and new types of enemies appearing in subsequent waves.

6. **Delta Time Calculation:**
   - The game calculates the time elapsed since the last frame (delta time) to ensure consistent movement and animations across varying frame rates.

7. **Gameplay Integration:**
   - Gameplay code can easily access controller states and respond to input events, providing a seamless experience for players.

## Conclusion

This enhanced version of the Starship Prototype integrates Xbox Controller support, debris effects, attract mode, limited lives/waves, and real-time delta seconds for variable frame rates, providing a more flexible control scheme and immersive gameplay experience. With a well-structured input management system and realistic debris animations, this project demonstrates the capability to handle various input devices and enrich the player experience.
