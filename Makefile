CC = gcc
CFLAGS = -Wall -Wextra -std=c23

TARGET = build/executable
SOURCES = main.c

$(TARGET): $(SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -rf build

.PHONY: clean
