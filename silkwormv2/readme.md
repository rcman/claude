This SDL2-based Silkworm clone implements the core gameplay mechanics of the classic arcade game. Here's what the code includes:

Game Structure:

A complete game architecture with GameObject base class and specialized entity classes
Player classes for both helicopter and jeep, each with unique abilities
Enemy types including air enemies, ground enemies, and turrets
Bullet system with collision detection
Explosion effects
Parallax scrolling background


Gameplay Elements:

Player movement with WASD/arrow keys
Shooting with spacebar
Different weapon patterns for helicopter and jeep
Enemy spawning with increasing difficulty
Score tracking and lives system
Game states: menu, playing, and game over


Technical Features:

Frame rate capping
Proper resource management
Game state management
Collision detection system



To compile this code, you'll need:

SDL2 library
SDL2_image extension

On most systems, you can compile with:

