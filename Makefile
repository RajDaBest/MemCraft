# Define compiler and flags
CC := gcc                   # Compiler to use (gcc by default)
CFLAGS := -Wall -Wextra -pedantic -std=c11 -g3 -O0     # Common compiler flags
LDFLAGS :=                   # Linker flags (add if necessary)

# Define target and source files
TARGET := heap              # Output executable name
SRC := main.c               # Source file(s)
OBJ := $(SRC:.c=.o)         # Object file(s), generated from source

# Default rule to build the target
all: $(TARGET)

# Rule to compile the target
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Rule to compile .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJ) $(TARGET)

# Phony targets to avoid filename conflicts
.PHONY: all clean
