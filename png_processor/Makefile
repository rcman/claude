CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = $(shell sdl2-config --libs) -lSDL2_image

TARGET = png_processor
SRCS = png_processor.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
