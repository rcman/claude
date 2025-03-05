#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdio.h>
#include <stdbool.h>

typedef struct {
    SDL_Surface *surface;
    Uint32 *pixels;
    int width;
    int height;
} Image;

// Function to load a PNG and get its information
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

// Function to create a mask from an image (white where not transparent)
Image createMask(Image source) {
    Image mask = {0};
    
    // Create a new surface for the mask
    mask.surface = SDL_CreateRGBSurfaceWithFormat(0, source.width, source.height, 32, SDL_PIXELFORMAT_RGBA8888);
    if (!mask.surface) {
        printf("Error creating mask surface: %s\n", SDL_GetError());
        return mask;
    }
    
    mask.width = source.width;
    mask.height = source.height;
    mask.pixels = (Uint32 *)mask.surface->pixels;
    
    // Set white color for the mask
    Uint32 white = SDL_MapRGBA(mask.surface->format, 255, 255, 255, 255);
    Uint32 transparent = SDL_MapRGBA(mask.surface->format, 0, 0, 0, 0);
    
    // Process each pixel
    for (int y = 0; y < source.height; y++) {
        for (int x = 0; x < source.width; x++) {
            int index = y * source.width + x;
            
            // Get alpha value of source pixel
            Uint8 r, g, b, a;
            SDL_GetRGBA(source.pixels[index], source.surface->format, &r, &g, &b, &a);
            
            // If pixel is not transparent, set it to white in the mask
            if (a > 0) {
                mask.pixels[index] = white;
            } else {
                mask.pixels[index] = transparent;
            }
        }
    }
    
    return mask;
}

// Function to save images as a spritesheet
bool saveSpritesheet(Image original, Image mask, const char *filename) {
    // Create a surface for the spritesheet (two images side by side)
    int spritesheetWidth = original.width * 2;
    int spritesheetHeight = original.height;
    
    SDL_Surface *spritesheet = SDL_CreateRGBSurfaceWithFormat(0, spritesheetWidth, spritesheetHeight, 
                                                            32, SDL_PIXELFORMAT_RGBA8888);
    if (!spritesheet) {
        printf("Error creating spritesheet surface: %s\n", SDL_GetError());
        return false;
    }
    
    // Copy the original image to the left side
    SDL_Rect destRect = {0, 0, original.width, original.height};
    SDL_BlitSurface(original.surface, NULL, spritesheet, &destRect);
    
    // Copy the mask to the right side
    destRect.x = original.width;
    SDL_BlitSurface(mask.surface, NULL, spritesheet, &destRect);
    
    // Save the spritesheet
    int result = IMG_SavePNG(spritesheet, filename);
    SDL_FreeSurface(spritesheet);
    
    return (result == 0);
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc < 3) {
        printf("Usage: %s <input_png> <output_spritesheet.png>\n", argv[0]);
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
    
    // Load the original image
    Image original = loadImage(inputFile);
    if (!original.surface) {
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    printf("Image loaded: %s (%d x %d)\n", inputFile, original.width, original.height);
    
    // Create the mask
    Image mask = createMask(original);
    if (!mask.surface) {
        SDL_FreeSurface(original.surface);
        IMG_Quit();
        SDL_Quit();
        return 1;
    }
    
    printf("Mask created\n");
    
    // Save the spritesheet
    if (saveSpritesheet(original, mask, outputFile)) {
        printf("Spritesheet saved to %s\n", outputFile);
    } else {
        printf("Failed to save spritesheet: %s\n", IMG_GetError());
    }
    
    // Clean up
    SDL_FreeSurface(original.surface);
    SDL_FreeSurface(mask.surface);
    IMG_Quit();
    SDL_Quit();
    
    return 0;
}
