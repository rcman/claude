SDL2 PNG Transparency Processor
I've created a program that:

Loads a PNG image with transparency
Automatically detects the image dimensions
Creates a white mask where non-transparent pixels are located
Saves both the original image and the mask as a 2-image spritesheet

How It Works
The program:

Takes two command-line arguments: the input PNG file and the output spritesheet filename
Loads the PNG using SDL_image and gets its dimensions automatically
Creates an array of pixels for both the original image and the mask
Processes each pixel - if a pixel has any opacity (alpha > 0), it adds a white pixel at the same position in the mask
Saves both images side-by-side in a single spritesheet PNG
