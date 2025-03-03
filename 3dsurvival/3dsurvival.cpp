#include <SDL2/SDL.h>
#include <GL/glew.h>
#include <SDL2/SDL_opengl.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <random>

// Window dimensions
const int SCREEN_WIDTH = 800;
const int SCREEN_HEIGHT = 600;

// Player properties
struct Player {
    glm::vec3 position;
    glm::vec3 direction;
    float health;
    float hunger;
    float speed;
};

// Object properties
struct GameObject {
    glm::vec3 position;
    glm::vec3 size;
    glm::vec3 color;
    bool isCollectible;
};

// Camera properties
struct Camera {
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    float yaw;
    float pitch;
};

// Shader sources
const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    
    uniform mat4 model;
    uniform mat4 view;
    uniform mat4 projection;
    
    void main() {
        gl_Position = projection * view * model * vec4(aPos, 1.0);
    }
)";

const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    
    uniform vec3 objectColor;
    
    void main() {
        FragColor = vec4(objectColor, 1.0);
    }
)";

// Function prototypes
bool initSDL(SDL_Window** window, SDL_GLContext* context);
GLuint compileShaders();
void setupGame(Player& player, Camera& camera, std::vector<GameObject>& objects);
void handleInput(SDL_Event& event, bool& quit, Player& player, Camera& camera, float deltaTime);
void updateGame(Player& player, Camera& camera, std::vector<GameObject>& objects, float deltaTime);
void renderGame(GLuint shaderProgram, Player& player, Camera& camera, std::vector<GameObject>& objects);
GLuint createCubeVAO();
bool checkCollision(const glm::vec3& playerPos, const GameObject& object, float tolerance);

int main(int argc, char* argv[]) {
    SDL_Window* window = nullptr;
    SDL_GLContext context;
    
    // Initialize SDL and OpenGL
    if (!initSDL(&window, &context)) {
        return -1;
    }
    
    // Compile shaders
    GLuint shaderProgram = compileShaders();
    if (shaderProgram == 0) {
        SDL_GL_DeleteContext(context);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }
    
    // Create cube VAO
    GLuint cubeVAO = createCubeVAO();
    
    // Setup game
    Player player;
    Camera camera;
    std::vector<GameObject> objects;
    setupGame(player, camera, objects);
    
    // Game loop variables
    bool quit = false;
    SDL_Event event;
    Uint32 lastTime = SDL_GetTicks();
    
    // Game loop
    while (!quit) {
        // Calculate delta time
        Uint32 currentTime = SDL_GetTicks();
        float deltaTime = (currentTime - lastTime) / 1000.0f;
        lastTime = currentTime;
        
        // Handle input
        handleInput(event, quit, player, camera, deltaTime);
        
        // Update game
        updateGame(player, camera, objects, deltaTime);
        
        // Render game
        glClearColor(0.1f, 0.1f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        renderGame(shaderProgram, player, camera, objects);
        
        // Swap buffers
        SDL_GL_SwapWindow(window);
    }
    
    // Cleanup
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteProgram(shaderProgram);
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

bool initSDL(SDL_Window** window, SDL_GLContext* context) {
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "SDL could not initialize! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Set OpenGL attributes
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    
    // Create window
    *window = SDL_CreateWindow("3D Survival Game", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 
                              SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    if (*window == nullptr) {
        std::cerr << "Window could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Create OpenGL context
    *context = SDL_GL_CreateContext(*window);
    if (*context == nullptr) {
        std::cerr << "OpenGL context could not be created! SDL Error: " << SDL_GetError() << std::endl;
        return false;
    }
    
    // Initialize GLEW
    glewExperimental = GL_TRUE;
    GLenum glewError = glewInit();
    if (glewError != GLEW_OK) {
        std::cerr << "Error initializing GLEW! " << glewGetErrorString(glewError) << std::endl;
        return false;
    }
    
    // Enable vsync
    if (SDL_GL_SetSwapInterval(1) < 0) {
        std::cerr << "Warning: Unable to set VSync! SDL Error: " << SDL_GetError() << std::endl;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    
    return true;
}

GLuint compileShaders() {
    // Vertex shader
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for compilation errors
    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    
    // Fragment shader
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for compilation errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    
    // Link shaders
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
        return 0;
    }
    
    // Delete shaders as they're linked into the program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return shaderProgram;
}

GLuint createCubeVAO() {
    // Vertex data for a cube
    float vertices[] = {
        // Front face
        -0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        
        // Back face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        
        // Left face
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f, -0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        
        // Right face
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        
        // Top face
        -0.5f,  0.5f, -0.5f,
         0.5f,  0.5f, -0.5f,
         0.5f,  0.5f,  0.5f,
         0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f,  0.5f,
        -0.5f,  0.5f, -0.5f,
        
        // Bottom face
        -0.5f, -0.5f, -0.5f,
         0.5f, -0.5f, -0.5f,
         0.5f, -0.5f,  0.5f,
         0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f,  0.5f,
        -0.5f, -0.5f, -0.5f
    };
    
    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    // Bind the Vertex Array Object
    glBindVertexArray(VAO);
    
    // Bind and set vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // Set vertex attribute pointers
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Unbind VBO and VAO
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    return VAO;
}

void setupGame(Player& player, Camera& camera, std::vector<GameObject>& objects) {
    // Initialize player
    player.position = glm::vec3(0.0f, 1.0f, 5.0f);
    player.direction = glm::vec3(0.0f, 0.0f, -1.0f);
    player.health = 100.0f;
    player.hunger = 100.0f;
    player.speed = 5.0f;
    
    // Initialize camera
    camera.position = player.position;
    camera.front = glm::vec3(0.0f, 0.0f, -1.0f);
    camera.up = glm::vec3(0.0f, 1.0f, 0.0f);
    camera.yaw = -90.0f;
    camera.pitch = 0.0f;
    
    // Create a random number generator
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> posDistX(-20.0f, 20.0f);
    std::uniform_real_distribution<float> posDistZ(-20.0f, 20.0f);
    std::uniform_real_distribution<float> sizeDistWidth(0.5f, 2.0f);
    std::uniform_real_distribution<float> sizeDistHeight(0.5f, 4.0f);
    std::uniform_real_distribution<float> colorDist(0.0f, 1.0f);
    std::bernoulli_distribution isCollectible(0.3);
    
    // Create ground
    GameObject ground;
    ground.position = glm::vec3(0.0f, -0.5f, 0.0f);
    ground.size = glm::vec3(50.0f, 1.0f, 50.0f);
    ground.color = glm::vec3(0.2f, 0.5f, 0.2f); // Green for grass
    ground.isCollectible = false;
    objects.push_back(ground);
    
    // Create some trees
    for (int i = 0; i < 15; ++i) {
        // Tree trunk
        GameObject trunk;
        float x = posDistX(gen);
        float z = posDistZ(gen);
        trunk.position = glm::vec3(x, 1.5f, z);
        trunk.size = glm::vec3(0.5f, 3.0f, 0.5f);
        trunk.color = glm::vec3(0.5f, 0.3f, 0.0f); // Brown
        trunk.isCollectible = false;
        objects.push_back(trunk);
        
        // Tree leaves
        GameObject leaves;
        leaves.position = glm::vec3(x, 4.0f, z);
        leaves.size = glm::vec3(2.0f, 2.0f, 2.0f);
        leaves.color = glm::vec3(0.0f, 0.5f, 0.0f); // Dark green
        leaves.isCollectible = false;
        objects.push_back(leaves);
    }
    
    // Create some rocks
    for (int i = 0; i < 10; ++i) {
        GameObject rock;
        rock.position = glm::vec3(posDistX(gen), 0.25f, posDistZ(gen));
        float size = sizeDistWidth(gen) * 0.5f;
        rock.size = glm::vec3(size, size, size);
        rock.color = glm::vec3(0.5f, 0.5f, 0.5f); // Gray
        rock.isCollectible = isCollectible(gen);
        objects.push_back(rock);
    }
    
    // Create some resources
    for (int i = 0; i < 8; ++i) {
        GameObject resource;
        resource.position = glm::vec3(posDistX(gen), 0.5f, posDistZ(gen));
        resource.size = glm::vec3(0.5f, 0.5f, 0.5f);
        resource.color = glm::vec3(0.8f, 0.1f, 0.1f); // Red for resources
        resource.isCollectible = true;
        objects.push_back(resource);
    }
    
    // Create water pool
    GameObject water;
    water.position = glm::vec3(10.0f, -0.25f, 10.0f);
    water.size = glm::vec3(10.0f, 0.5f, 10.0f);
    water.color = glm::vec3(0.0f, 0.3f, 0.8f); // Blue
    water.isCollectible = false;
    objects.push_back(water);
}

void handleInput(SDL_Event& event, bool& quit, Player& player, Camera& camera, float deltaTime) {
    // Handle events
    while (SDL_PollEvent(&event) != 0) {
        if (event.type == SDL_QUIT) {
            quit = true;
        }
        else if (event.type == SDL_KEYDOWN) {
            if (event.key.keysym.sym == SDLK_ESCAPE) {
                quit = true;
            }
        }
        else if (event.type == SDL_MOUSEMOTION) {
            float sensitivity = 0.1f;
            camera.yaw += event.motion.xrel * sensitivity;
            camera.pitch -= event.motion.yrel * sensitivity;
            
            // Constrain pitch
            if (camera.pitch > 89.0f) {
                camera.pitch = 89.0f;
            }
            if (camera.pitch < -89.0f) {
                camera.pitch = -89.0f;
            }
            
            // Update camera front vector
            glm::vec3 front;
            front.x = cos(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            front.y = sin(glm::radians(camera.pitch));
            front.z = sin(glm::radians(camera.yaw)) * cos(glm::radians(camera.pitch));
            camera.front = glm::normalize(front);
        }
    }
    
    // Handle keyboard input for movement
    const Uint8* state = SDL_GetKeyboardState(NULL);
    float moveSpeed = player.speed * deltaTime;
    
    if (state[SDL_SCANCODE_W]) {
        player.position += moveSpeed * glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
    }
    if (state[SDL_SCANCODE_S]) {
        player.position -= moveSpeed * glm::normalize(glm::vec3(camera.front.x, 0.0f, camera.front.z));
    }
    if (state[SDL_SCANCODE_A]) {
        player.position -= glm::normalize(glm::cross(camera.front, camera.up)) * moveSpeed;
    }
    if (state[SDL_SCANCODE_D]) {
        player.position += glm::normalize(glm::cross(camera.front, camera.up)) * moveSpeed;
    }
    if (state[SDL_SCANCODE_SPACE]) {
        player.position.y += moveSpeed;
    }
    if (state[SDL_SCANCODE_LSHIFT]) {
        player.position.y -= moveSpeed;
    }
    
    // Grab the mouse
    SDL_SetRelativeMouseMode(SDL_TRUE);
}

void updateGame(Player& player, Camera& camera, std::vector<GameObject>& objects, float deltaTime) {
    // Update camera position to follow player
    camera.position = player.position;
    
    // Apply gravity (simple implementation)
    if (player.position.y > 1.0f) {
        player.position.y -= 2.0f * deltaTime;
    }
    if (player.position.y < 1.0f) {
        player.position.y = 1.0f;
    }
    
    // Boundary checks to keep player in the game area
    if (player.position.x < -25.0f) player.position.x = -25.0f;
    if (player.position.x > 25.0f) player.position.x = 25.0f;
    if (player.position.z < -25.0f) player.position.z = -25.0f;
    if (player.position.z > 25.0f) player.position.z = 25.0f;
    
    // Check for collisions with objects
    for (auto it = objects.begin(); it != objects.end();) {
        if (it->isCollectible && checkCollision(player.position, *it, 1.0f)) {
            // "Collect" the object
            std::cout << "Collected an item!" << std::endl;
            it = objects.erase(it);
            
            // Increase player stats
            player.health = std::min(player.health + 10.0f, 100.0f);
            player.hunger = std::min(player.hunger + 15.0f, 100.0f);
        } else {
            ++it;
        }
    }
    
    // Decrease hunger over time
    player.hunger -= 0.5f * deltaTime;
    if (player.hunger < 0.0f) {
        player.hunger = 0.0f;
        // When hungry, decrease health
        player.health -= 1.0f * deltaTime;
    }
    
    // Check player health
    if (player.health <= 0.0f) {
        std::cout << "Game Over! You died." << std::endl;
        player.health = 100.0f;
        player.hunger = 100.0f;
        player.position = glm::vec3(0.0f, 1.0f, 5.0f);
    }
    
    // Display player stats
    std::cout << "Health: " << player.health << " | Hunger: " << player.hunger << "\r" << std::flush;
}

void renderGame(GLuint shaderProgram, Player& player, Camera& camera, std::vector<GameObject>& objects) {
    // Use shader program
    glUseProgram(shaderProgram);
    
    // Set up view and projection matrices
    glm::mat4 view = glm::lookAt(camera.position, camera.position + camera.front, camera.up);
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 
                                           (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 
                                           0.1f, 100.0f);
    
    // Pass matrices to shader
    GLint viewLoc = glGetUniformLocation(shaderProgram, "view");
    GLint projLoc = glGetUniformLocation(shaderProgram, "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    
    // Bind VAO
    glBindVertexArray(createCubeVAO());
    
    // Render each object
    for (const auto& object : objects) {
        // Set model matrix
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, object.position);
        model = glm::scale(model, object.size);
        
        GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
        glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
        
        // Set object color
        GLint colorLoc = glGetUniformLocation(shaderProgram, "objectColor");
        glUniform3fv(colorLoc, 1, glm::value_ptr(object.color));
        
        // Draw cube
        glDrawArrays(GL_TRIANGLES, 0, 36);
    }
    
    // Unbind VAO
    glBindVertexArray(0);
}

bool checkCollision(const glm::vec3& playerPos, const GameObject& object, float tolerance) {
    // Simple box collision detection
    bool collisionX = playerPos.x >= object.position.x - object.size.x / 2 - tolerance && 
                      playerPos.x <= object.position.x + object.size.x / 2 + tolerance;
    
    bool collisionY = playerPos.y >= object.position.y - object.size.y / 2 - tolerance && 
                      playerPos.y <= object.position.y + object.size.y / 2 + tolerance;
    
    bool collisionZ = playerPos.z >= object.position.z - object.size.z / 2 - tolerance && 
                      playerPos.z <= object.position.z + object.size.z / 2 + tolerance;
    
    return collisionX && collisionY && collisionZ;
}
