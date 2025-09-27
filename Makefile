# Makefile for lavarnd_modern project
# Inspired by LavaRnd, updated for modern V4L2 and random number generation

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
LDFLAGS = -lcrypto -lm

# Source and output
SRC = lavarnd_modern.c
OBJ = $(SRC:.c=.o)
TARGET = lavarnd_modern

# Installation path
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

# Default target
all: $(TARGET)

# Compile object file
$(OBJ): $(SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Link object to create executable
$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Clean up
clean:
	rm -f $(OBJ) $(TARGET)

# Install the binary
install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)

# Uninstall the binary
uninstall:
	rm -f $(BINDIR)/$(TARGET)

# Phony targets
.PHONY: all clean install uninstall

