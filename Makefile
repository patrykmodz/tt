CC = gcc
CFLAGS = -Wall -Wextra -std=c23

TARGET = build/tt
SOURCES = main.c editor.c file.c prompt.c statusbar.c
OBJECTS = $(patsubst %.c,build/%.o,$(SOURCES))

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $(TARGET)

build/%.o: %.c | build
	$(CC) $(CFLAGS) -c $< -o $@

build:
	mkdir -p build

clean:
	rm -rf build

.PHONY: clean
