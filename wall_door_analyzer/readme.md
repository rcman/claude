SDL2 Wall and Door Analyzer
I've created a program that analyzes an image to identify wall textures and doors, then generates a new image based on the analysis, highlighting the walls, doors, and the length of wall segments between doors.
How It Works
The program:

Loads an input image using SDL2_image
Analyzes each row of pixels to identify wall and door segments
Segments are defined by color - the program looks for:

Doors: Brown color (RGB: 150, 75, 0) with some tolerance
Walls: Gray color (RGB: 100, 100, 100) with some tolerance


Creates a visualization showing:

Wall segments in light gray
Door segments in orange
Indicator bars below wall segments that change color based on the length of the wall



The Analysis Process

The program scans the image row by row
For each row, it identifies continuous segments of walls or doors
When it finds a transition (wall to door or vice versa), it records the segment
For each segment, it records:

Starting and ending X position
Y position (row)
Whether it's a wall or door
Length in pixels



The Output Image
The output visualization:

Shows all detected wall and door segments at their original positions
Thickens the segments for better visibility
Adds colored bars below wall segments to represent their length
Logs detailed segment information to the console
