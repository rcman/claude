CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image -lm

TARGET = wall_door_analyzer
SRCS = wall_door_analyzer.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
