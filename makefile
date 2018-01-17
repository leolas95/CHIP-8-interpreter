chip8: main.c chip8.c
	gcc -std=c99 -Wall -Werror main.c chip8.c `sdl2-config --cflags --libs` -o chip8

debug: main.c chip8.c
	gcc -std=c99 -Wall -Werror main.c chip8.c `sdl2-config --cflags --libs` -DDEBUG -o debug
