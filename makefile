chip8: main.c chip8.c opcode_functions.c
	gcc -std=c99 -Wall -Werror -g main.c opcode_functions.c chip8.c `sdl2-config --cflags --libs` -o chip8

debug: main.c chip8.c opcode_functions.c
	gcc -std=c99 -Wall -Werror -g main.c chip8.c opcode_functions.c `sdl2-config --cflags --libs` -DDEBUG -o debug
