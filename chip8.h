#ifndef _CHIP8_HEADER_
#define _CHIP8_HEADER_

#include <stdbool.h>

#define CHIP8_DISPLAY_WIDTH  64
#define CHIP8_DISPLAY_HEIGHT 32
#define CHIP8_DISPLAY_SIZE   (CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT)
#define CHIP8_MEMSIZE        (4096)  /* 4K */
#define REGISTERS            16
#define MAX_STACK_LEVELS     16
#define MAX_KEYPAD_KEYS      16

typedef struct chip8 {
	unsigned short opcode;                   /* The current opcode */
	unsigned char  memory[CHIP8_MEMSIZE];    /* Memory (4K) */
	unsigned char  V[REGISTERS];             /* The V registers (V0-VF) */
	unsigned short I;                        /* I register (Address register). 16 bits wide */
	unsigned short pc;			 /* Program counter */
	unsigned char  gfx[CHIP8_DISPLAY_SIZE];  /* Graphics */
	unsigned char  delay_timer;
	unsigned char  sound_timer;
	unsigned short stack[MAX_STACK_LEVELS];  /* Stack. We support maximum 16 levels of nesting */
	unsigned short sp;                       /* Stack pointer. Points to the next FREE frame of the stack */
	unsigned char  key[MAX_KEYPAD_KEYS];     /* Keypad */
	bool shouldDraw;                         /* To indicate wether we have to redraw the screen or not */
} Chip8;

void init_chip8(Chip8 *);
void load_rom(Chip8 *, const char * const);
void cycle(Chip8 *);

#endif /* _CHIP8_HEADER_ */
