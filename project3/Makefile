CC = gcc
CFLAGS = -Wall -Wextra -Iinclude -g
SRC = src/main.c src/fat32.c src/lexer.c 
OBJ = $(SRC:.c=.o)
EXEC = filesys

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^

src/%.o: src/%.c include/%.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(EXEC)

.PHONY: all clean
