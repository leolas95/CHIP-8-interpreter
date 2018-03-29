CC=gcc
CSTD=c99
CFLAGS=-std=$(CSTD) -Wall -Werror -g
SDLFLAG=`sdl2-config --cflags --libs`
OBJECTS=main.o chip8.o opcode_functions.o

chip8: $(OBJECTS)
	$(CC) $(CFLAGS) -o chip8 $(OBJECTS) $(SDLFLAG)

main.o: main.c chip8.h
	$(CC) $(CFLAGS) -c main.c

chip8.o: chip8.c chip8.h opcode_functions.h
	$(CC) $(CFLAGS) -c chip8.c

opcode_functions.o: opcode_functions.c opcode_functions.h
	$(CC) $(CFLAGS) -c opcode_functions.c

debug: CFLAGS += -DDEBUG
debug: $(OBJECTS)
	$(CC) $(CFLAGS) -o debug $(OBJECTS) $(SDLFLAG)

.PHONY: clean
clean:
	rm main.o chip8.o opcode_functions.o
