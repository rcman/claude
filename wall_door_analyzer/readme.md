# SDL2 Wall and Door Tile Extractor<br>

Analyze an input image containing wall textures and doors
Extract a repeatable wall tile that can be used to create walls of any length
Identify and extract door tiles as separate images
Create a test image showing how these tiles can be reused to build new wall configurations

How the Program Works
Tile Extraction Process:

Wall Detection:

The program analyzes the image to determine the average color of wall sections
It identifies wall areas based on color similarity within a threshold


Door Detection:

Doors are identified as regions that differ significantly from the wall color
For each door, the program records its position and dimensions
A minimum door width helps filter out small artifacts


Wall Tile Extraction:

The program finds a suitable section of wall (between doors or at the edges)
Extracts a portion of the wall texture as a repeatable tile
Saves this as wall_tile.png


Door Tile Extraction:

For each identified door, extracts the door image
Saves each door as door_tile_X.png (where X is the door number)


Test Image Creation:

Creates a new image with twice the width of the original
Tiles the extracted wall texture across the image
Places doors at new positions to demonstrate reusability
