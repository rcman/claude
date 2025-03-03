**his SDL2 framework implements all the features you requested for your platform game. Let me explain the key components:<br>

**Core Gameplay Features<br>

<br>
**Platform and Ladder System<br>

<br>
Platforms have collision detection for player and enemies to stand on<br>
Ladders allow vertical movement between different levels<br>
Gravity applies when not on a ladder or platform<br>
<br>

**Searchable Items<br>

Three types: Ammo, Health, and Armor pieces<br>
Search mechanic with 5-second timer and visual progress circle<br>
Space bar held to search items<br>


**Combat System<br>

Player and enemies can shoot projectiles<br>
Enemies have different health points<br>
Collision detection for projectiles<br>


**Armor System<br>

5 armor pieces to collect (head, chest, legs, arms, feet)<br>
Each complete set upgrades your armor level<br>
Armor reduces damage based on level<br>
<br>


**Technical Implementation<br>

Game Object Hierarchy: Base GameObject class with specialized classes for Player, Enemy, Platform, etc.<br>
Collision Detection: Complete system for all game objects<br>
Animation: Simple progress circle for the search mechanic<br>
Game Loop: Proper timing with deltaTime for consistent gameplay regardless of frame rate<br>
<br>

**How to Use This Code<br>

Set up SDL2 and SDL2_image libraries on your system<br>
Compile the code with appropriate linking to SDL2 libraries<br>
Run the game and use:<br>

Arrow keys or WASD to move<br>
Ctrl to shoot<br>
Space to search items<br>
Up arrow or W to jump<br>

