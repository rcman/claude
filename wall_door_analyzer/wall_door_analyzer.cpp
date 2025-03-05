#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

// Define constants
#define MAX_DOORS 20
#define WALL_FILENAME "wall_tile.png"
#define DOOR_FILENAME_PREFIX "door_tile_"
#define WALL_COLOR_R 100
#define WALL_COLOR_G 100
#define WALL_COLOR_B 100
#define WALL_THRESHOLD 50
#define DOOR_THRESHOLD 40
#define MIN_DOOR_WIDTH 20
#define MIN_TILE_WIDTH 32
#define TILE_HEIGHT 64  // Assuming a fixed height for tiles

typedef struct {
    SDL_Surface *surface;
    Uint32 *pixels;
    int width;
    int height;
} Image;

typedef struct {
    int x;
    int y;
    int width;
    int height;
    SDL_Rect rect;
} DoorPosition;

// Function to load an image
Image loadImage(const char *filename) {
    Image img = {0};
    
    // Load the image
    img.surface = IMG_Load(filename);
    if (!img.surface) {
        printf("Error loading image %s: %s\n", filename, IMG_GetError());
        return img;
    }
    
    // Get image dimensions
    img.width = img.surface->w;
    img.height = img.surface->h;
    
    // Convert the surface to a format we can work with easily
    SDL_Surface *convertedSurface = SDL_ConvertSurfaceFormat(img.surface, SDL_PIXELFORMAT_RGBA8888, 0);
    if (convertedSurface) {
        SDL_FreeSurface(img.surface);
        img.surface = convertedSurface;
    }
    
    // Get direct access to pixels
    img.pixels = (Uint32 *)img.surface->pixels;
    
    return img;
}

// Function to check if a pixel is similar to another (within threshold)
bool isColorSimilar(Uint8 r1, Uint8 g1, Uint8 b1, Uint8 r2, Uint8 g2, Uint8 b2, int threshold) {
    int diff_r = abs(r1 - r2);
    int diff_g = abs(g1 - g2);
    int diff_b = abs(b1 - b2);
    
    return (diff_r + diff_g + diff_b < threshold);
}

// Function to check if a pixel is part of the wall
bool isWallPixel(Uint8 r, Uint8 g, Uint8 b) {
    return isColorSimilar(r, g, b, WALL_COLOR_R, WALL_COLOR_G, WALL_COLOR_B, WALL_THRESHOLD);
}

// Function to calculate the average color of a region
void getAverageColor(Image img, int start_x, int start_y, int width, int height, Uint8 *avg_r, Uint8 *avg_g, Uint8 *avg_b) {
    long sum_r = 0, sum_g = 0, sum_b = 0;
    int count = 0;
    
    for (int y = start_y; y < start_y + height && y < img.height; y++) {
        for (int x = start_x; x < start_x + width && x < img.width; x++) {
            int index = y * img.width + x;
            
            Uint8 r, g, b, a;
            SDL_GetRGBA(img.pixels[index], img.surface->format, &r, &g, &b, &a);
            
            if (a > 0) {  // Only count non-transparent pixels
                sum_r += r;
                sum_g += g;
                sum_b += b;
                count++;
            }
        }
    }
    
    if (count > 0) {
        *avg_r = sum_r / count;
        *avg_g = sum_g / count;
        *avg_b = sum_b / count;
    } else {
        *avg_r = 0;
        *avg_g = 0;
        *avg_b = 0;
    }
}

// Function to find the doors in the image
int findDoors(Image source, DoorPosition doors[MAX_DOORS]) {
    int door_count = 0;
    
    // First, try to determine what might be a door by looking for regions that differ from the wall
    // We'll look for horizontal regions that are different from the wall texture
    
    // First, get a reference wall color by sampling several wall regions
    Uint8 wall_avg_r, wall_avg_g, wall_avg_b;
    getAverageColor(source, 0, 0, source.width / 4, TILE_HEIGHT, &wall_avg_r, &wall_avg_g, &wall_avg_b);
    
    printf("Reference wall color: RGB(%d, %d, %d)\n", wall_avg_r, wall_avg_g, wall_avg_b);
    
    // Scan for doors (regions that differ significantly from the wall color)
    bool in_door = false;
    int door_start_x = 0;
    
    for (int x = 0; x < source.width; x++) {
        // Sample a vertical strip to determine if it's part of a door
        Uint8 strip_avg_r, strip_avg_g, strip_avg_b;
        getAverageColor(source, x, 0, 1, source.height, &strip_avg_r, &strip_avg_g, &strip_avg_b);
        
        bool is_door_pixel = !isColorSimilar(strip_avg_r, strip_avg_g, strip_avg_b, 
                                           wall_avg_r, wall_avg_g, wall_avg_b, 
                                           DOOR_THRESHOLD);
        
        // Starting a new door
        if (!in_door && is_door_pixel) {
            in_door = true;
            door_start_x = x;
        }
        // Ending a door
        else if (in_door && (!is_door_pixel || x == source.width - 1)) {
            in_door = false;
            
            // Calculate door width
            int door_width = x - door_start_x;
            if (x == source.width - 1 && is_door_pixel) {
                door_width++;  // Include the last pixel
            }
            
            // Only add if it's wide enough to be a door and we have space
            if (door_width >= MIN_DOOR_WIDTH && door_count < MAX_DOORS) {
                doors[door_count].x = door_start_x;
                doors[door_count].y = 0;
                doors[door_count].width = door_width;
                doors[door_count].height = source.height;
                doors[door_count].rect = (SDL_Rect){door_start_x, 0, door_width, source.height};
                door_count++;
                
                printf("Door found at x=%d with width=%d\n", door_start_x, door_width);
            }
        }
    }
    
    return door_count;
}

// Function to extract a wall tile
bool extractWallTile(Image source, DoorPosition doors[], int door_count) {
    // Find a good section of wall to use as a tile (between doors or at the edges)
    int tile_x = 0;
    int tile_width = MIN_TILE_WIDTH;
    bool tile_found = false;
    
    // Try to find a section of wall between doors
    for (int i = 0; i < door_count - 1; i++) {
        int space_between = doors[i+1].x - (doors[i].x + doors[i].width);
        if (space_between >= MIN_TILE_WIDTH) {
            tile_x = doors[i].x + doors[i].width;
            tile_width = MIN_TILE_WIDTH;  // Or use a larger value if you want
            tile_found = true;
            break;
        }
    }
    
    // If no suitable space between doors, check the beginning or end
    if (!tile_found) {
        if (door_count > 0) {
            if (doors[0].x >= MIN_TILE_WIDTH) {
                // Use beginning of wall
                tile_x = 0;
                tile_width = MIN_TILE_WIDTH;
                tile_found = true;
            } else if (source.width - (doors[door_count-1].x + doors[door_count-1].width) >= MIN_TILE_WIDTH) {
                // Use end of wall
                tile_x = doors[door_count-1].x + doors[door_count-1].width;
                tile_width = MIN_TILE_WIDTH;
                tile_found = true;
            }
        } else {
            // No doors, use beginning of wall
            tile_x = 0;
            tile_width = MIN_TILE_WIDTH;
            tile_found = true;
        }
    }
    
    if (!tile_found) {
        printf("Could not find a suitable wall section to extract a tile from.\n");
        return false;
    }
    
    printf("Extracting wall tile from x=%d with width=%d\n", tile_x, tile_width);
    
    // Create a surface for the wall tile
    SDL_Surface *tile_surface = SDL_CreateRGBSurfaceWithFormat(0, tile_width, source.height, 
                                                              32, SDL_PIXELFORMAT_RGBA8888);
    if (!tile_surface) {
        printf("Error creating wall tile surface: %s\n", SDL_GetError());
        return false;
    }
    
    // Copy the wall section to the tile
    SDL_Rect src_rect = {tile_x, 0, tile_width, source.height};
    SDL_BlitSurface(source.surface, &src_rect, tile_surface, NULL);
    
    // Save the wall tile
    int result = IMG_SavePNG(tile_surface, WALL_FILENAME);
    SDL_FreeSurface(tile_surface);
    
    if (result == 0) {
        printf("Wall tile saved to %s\n", WALL_FILENAME);
        return true;
    } else {
        printf("Failed to save wall tile: %s\n", IMG_GetError());
        return false;
    }
}

// Function to extract door tiles
bool extractDoorTiles(Image source, DoorPosition doors[], int door_count) {
    for (int i = 0; i < door_count; i++) {
        // Create a surface for the door tile
        SDL_Surface *door_surface = SDL_CreateRGBSurfaceWithFormat(0, doors[i].width, source.height, 
                                                                 32, SDL_PIXELFORMAT_RGBA8888);
        if (!door_surface) {
            printf("Error creating door tile surface: %s\n", SDL_GetError());
            continue;
        }
        
        // Copy the door to the tile
        SDL_Rect src_rect = {doors[i].x, 0, doors[i].width, source.height};
        SDL_BlitSurface(source.surface, &src_rect, door_surface, NULL);
        
        // Create the filename
        char filename[256];
        snprintf(filename, sizeof(filename), "%s%d.png", DOOR_FILENAME_PREFIX, i + 1);
        
        // Save the door tile
        int result = IMG_SavePNG(door_surface, filename);
        SDL_FreeSurface(door_surface);
        
        if (result == 0) {
            printf("Door tile %d saved to %s\n", i + 1, filename);
        } else {
            printf("Failed to save door tile %d: %s\n", i + 1, IMG_GetError());
            return false;
        }
    }
    
    return true;
}

// Function to create a test image using the extracted tiles
bool createTestImage(Image source, DoorPosition doors[], int door_count, const char *filename) {
    // Create a surface for the test image (making it wider to demonstrate tiling)
    int test_width = source.width * 2;  // Double the width to show tiling
    SDL_Surface *test_surface = SDL_CreateRGBSurfaceWithFormat(0, test_width, source.height, 
                                                              32, SDL_PIXELFORMAT_RGBA8888);
    if (!test_surface) {
        printf("Error creating test surface: %s\n", SDL_GetError());
        return false;
    }
    
    // Load the wall tile
    Image wall_tile = loadImage(WALL_FILENAME);
    if (!wall_tile.surface) {
        SDL_FreeSurface(test_surface);
        return false;
    }
    
    // Fill the test image with tiled wall
    for (int x = 0; x < test_width; x += wall_tile.width) {
        SDL_Rect dest_rect = {x, 0, wall_tile.width, wall_tile.height};
        SDL_BlitSurface(wall_tile.surface, NULL, test_surface, &dest_rect);
    }
    
    // Place doors at new positions
    for (int i = 0; i < door_count && i < 2; i++) {  // Place up to 2 doors in the test image
        char door_filename[256];
        snprintf(door_filename, sizeof(door_filename), "%s%d.png", DOOR_FILENAME_PREFIX, i + 1);
        
        Image door_tile = loadImage(door_filename);
        if (door_tile.surface) {
            // Place door at a new position
            SDL_Rect dest_rect = {(i + 1) * test_width / 3, 0, door_tile.width, door_tile.height};
            SDL_BlitSurface(door_tile.surface, NULL, test_surface, &dest_rect);
            
            SDL_FreeSurface(door_tile.surface);
        }
    }
    
    // Save the test image
    int result = IMG_SavePNG(test_surface, filename);
    
    // Clean up
    SDL_FreeSurface(wall_tile.surface);
    SDL_FreeSurface(test_surface);
    
    if (result == 0) {
        printf("Test image saved to %s\n", filename);
        return true;
    } else {
        printf("Failed to save test image: %s\n", IMG_GetError());
        return false;
    }
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc < 2) {
        printf("Usage: %s <input_image.png> [test_output.png]\n", argv[0]);
        return 1;
    }
    
    const char *inputFile = argv[1];
    const char *testFile = (argc >= 3) ? argv[2] : "test_output.png";
    
    // Initialize SDL and SDL_image
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL initialization failed: %s\n", SDL_GetError());
        return 1;
    }
    
    int imgFlags = IMG_INIT_PNG;
    if (!(IMG_Init(imgFlags) & imgFlags)) {
        printf("SDL_image initialization failed: %s\n", IMG_GetError());
        SDL_Quit();
        return 1;
    }
    
    // Load the source image
    Image source = loadImage(inputFile);
    if (!source.surface) {
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    printf("Image loaded: %s (%d x %d)\n", inputFile, source.width, source.height);
    
    // Find doors in the image
    DoorPosition doors[MAX_DOORS];
    int door_count = findDoors(source, doors);
    
    printf("Found %d door(s) in the image.\n", door_count);
    
    // Extract wall tile
    if (!extractWallTile(source, doors, door_count)) {
        printf("Failed to extract wall tile.\n");
    }
    
    // Extract door tiles
    if (door_count > 0) {
        if (!extractDoorTiles(source, doors, door_count)) {
            printf("Failed to extract some door tiles.\n");
        }
    }
    
    // Create a test image using the extracted tiles
    if (!createTestImage(source, doors, door_count, testFile)) {
        printf("Failed to create test image.\n");
    }
    
    // Clean up
    SDL_FreeSurface(source.surface);
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}
