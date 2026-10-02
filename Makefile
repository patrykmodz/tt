CC = gcc
CFLAGS = -Wall -Wextra -std=c23

TARGET = build/tt
SOURCES = main.c editor.c file.c prompt.c

$(TARGET): $(SOURCES)
	mkdir -p build
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

clean:
	rm -rf build

.PHONY: clean
