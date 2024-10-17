# Compiler and flags
CC = gcc
CFLAGS = -Wall -g

# Source files
SRC = main.c
OBJ = $(SRC:.c=.o)

# Executable names
EXEC_DRAGON = dragonshell
EXEC_RAND = rand

# Main target: link all object files to produce the dragonshell executable
$(EXEC_DRAGON): $(OBJ)
	$(CC) $(CFLAGS) -o $(EXEC_DRAGON) $(OBJ)

# Separate target for compiling rand.c
rand: rand.o
	$(CC) $(CFLAGS) -o $(EXEC_RAND) rand.o

# Compile target: compile source files into object files
compile: $(OBJ)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean target: remove object files and executables
clean:
	rm -f $(OBJ) $(EXEC_DRAGON) $(EXEC_RAND)
