#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// Define constants
#define MAX_SEGMENTS 100
#define DOOR_COLOR_R 150
#define DOOR_COLOR_G 75
#define DOOR_COLOR_B 0
#define DOOR_THRESHOLD 30
#define WALL_COLOR_R 100
#define WALL_COLOR_G 100
#define WALL_COLOR_B 100
#define WALL_THRESHOLD 30

typedef struct {
    SDL_Surface *surface;
    Uint32 *pixels;
    int width;
    int height;
} Image;

typedef struct {
    int start_x;
    int end_x;
    int y;
    bool is_door;
    int length;
} Segment;

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

// Function to check if a pixel is a door
bool isDoor(Uint8 r, Uint8 g, Uint8 b) {
    int diff_r = abs(r - DOOR_COLOR_R);
    int diff_g = abs(g - DOOR_COLOR_G);
    int diff_b = abs(b - DOOR_COLOR_B);
    
    return (diff_r + diff_g + diff_b < DOOR_THRESHOLD);
}

// Function to check if a pixel is a wall
bool isWall(Uint8 r, Uint8 g, Uint8 b) {
    int diff_r = abs(r - WALL_COLOR_R);
    int diff_g = abs(g - WALL_COLOR_G);
    int diff_b = abs(b - WALL_COLOR_B);
    
    return (diff_r + diff_g + diff_b < WALL_THRESHOLD);
}

// Function to analyze the image and identify wall and door segments
int analyzeImage(Image source, Segment segments[MAX_SEGMENTS]) {
    int segment_count = 0;
    
    // For each row in the image
    for (int y = 0; y < source.height; y++) {
        bool in_segment = false;
        bool is_door_segment = false;
        int segment_start = 0;
        
        // For each pixel in the row
        for (int x = 0; x < source.width; x++) {
            int index = y * source.width + x;
            
            // Get RGB values
            Uint8 r, g, b, a;
            SDL_GetRGBA(source.pixels[index], source.surface->format, &r, &g, &b, &a);
            
            bool pixel_is_door = isDoor(r, g, b);
            bool pixel_is_wall = isWall(r, g, b);
            
            // Start a new segment
            if (!in_segment && (pixel_is_door || pixel_is_wall)) {
                in_segment = true;
                segment_start = x;
                is_door_segment = pixel_is_door;
            }
            // End the current segment
            else if (in_segment && 
                    ((is_door_segment && !pixel_is_door) || 
                     (!is_door_segment && !pixel_is_wall) ||
                     x == source.width - 1)) {
                
                // End of image case - include the last pixel
                int segment_end = (x == source.width - 1 && (pixel_is_door || pixel_is_wall)) ? x : x - 1;
                
                // Only add segment if we have space
                if (segment_count < MAX_SEGMENTS) {
                    segments[segment_count].start_x = segment_start;
                    segments[segment_count].end_x = segment_end;
                    segments[segment_count].y = y;
                    segments[segment_count].is_door = is_door_segment;
                    segments[segment_count].length = segment_end - segment_start + 1;
                    segment_count++;
                }
                
                in_segment = false;
                
                // Check if this pixel starts a new segment of the other type
                if ((is_door_segment && pixel_is_wall) || (!is_door_segment && pixel_is_door)) {
                    in_segment = true;
                    segment_start = x;
                    is_door_segment = pixel_is_door;
                }
            }
            // Switch segment type
            else if (in_segment && 
                    ((is_door_segment && pixel_is_wall) || 
                     (!is_door_segment && pixel_is_door))) {
                
                // End the current segment
                if (segment_count < MAX_SEGMENTS) {
                    segments[segment_count].start_x = segment_start;
                    segments[segment_count].end_x = x - 1;
                    segments[segment_count].y = y;
                    segments[segment_count].is_door = is_door_segment;
                    segments[segment_count].length = x - 1 - segment_start + 1;
                    segment_count++;
                }
                
                // Start a new segment of the other type
                segment_start = x;
                is_door_segment = !is_door_segment;
            }
        }
    }
    
    return segment_count;
}

// Function to create an output image based on the analysis
Image createOutputImage(int width, int height, Segment segments[], int segment_count) {
    Image output = {0};
    
    // Create a surface for the output
    output.surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!output.surface) {
        printf("Error creating output surface: %s\n", SDL_GetError());
        return output;
    }
    
    output.width = width;
    output.height = height;
    output.pixels = (Uint32 *)output.surface->pixels;
    
    // Fill with black background
    SDL_FillRect(output.surface, NULL, SDL_MapRGBA(output.surface->format, 0, 0, 0, 255));
    
    // Define colors
    Uint32 wall_color = SDL_MapRGBA(output.surface->format, 200, 200, 200, 255);
    Uint32 door_color = SDL_MapRGBA(output.surface->format, 255, 120, 0, 255);
    
    // Draw segments
    for (int i = 0; i < segment_count; i++) {
        SDL_Rect rect;
        rect.x = segments[i].start_x;
        rect.y = segments[i].y;
        rect.w = segments[i].length;
        rect.h = 10;  // Thicken the segments for visibility
        
        SDL_FillRect(output.surface, &rect, segments[i].is_door ? door_color : wall_color);
        
        // Display segment length (for walls only)
        if (!segments[i].is_door) {
            // For simplicity, we'll just change the color based on length
            // In a real app, you'd want to render text or more detailed visualization
            int green_component = (segments[i].length * 2) % 255;
            Uint32 length_color = SDL_MapRGBA(output.surface->format, 100, green_component, 100, 255);
            
            rect.y += 12;  // Position below the segment
            rect.h = 5;    // Smaller indicator
            SDL_FillRect(output.surface, &rect, length_color);
        }
    }
    
    return output;
}

// Function to save the output image
bool saveImage(Image img, const char *filename) {
    int result = IMG_SavePNG(img.surface, filename);
    return (result == 0);
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc < 3) {
        printf("Usage: %s <input_image.png> <output_image.png>\n", argv[0]);
        return 1;
    }
    
    const char *inputFile = argv[1];
    const char *outputFile = argv[2];
    
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
    
    // Analyze the image
    Segment segments[MAX_SEGMENTS];
    int segment_count = analyzeImage(source, segments);
    
    printf("Analysis complete. Found %d segments.\n", segment_count);
    
    // Display segments
    for (int i = 0; i < segment_count; i++) {
        printf("Segment %d: %s at y=%d, x=%d to %d (length: %d)\n",
               i,
               segments[i].is_door ? "DOOR" : "WALL",
               segments[i].y,
               segments[i].start_x,
               segments[i].end_x,
               segments[i].length);
    }
    
    // Create output image
    Image output = createOutputImage(source.width, source.height, segments, segment_count);
    if (!output.surface) {
        SDL_FreeSurface(source.surface);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    // Save output image
    if (saveImage(output, outputFile)) {
        printf("Output image saved to %s\n", outputFile);
    } else {
        printf("Failed to save output image: %s\n", IMG_GetError());
    }
    
    // Clean up
    SDL_FreeSurface(source.surface);
    SDL_FreeSurface(output.surface);
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}
