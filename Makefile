# Makefile for lavarnd_modern project
# Inspired by LavaRnd, updated for modern V4L2 and random number generation

# Compiler and flags
CC = gcc
# Add -D_POSIX_C_SOURCE to ensure struct timeval is defined
CFLAGS = -std=c99 -D_POSIX_C_SOURCE=200809L -Wall -o2 -Wextra
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
