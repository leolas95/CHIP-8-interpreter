#ifndef _CHIP8_HEADER_
#define _CHIP8_HEADER_

#include <stdbool.h>
#include <stdint.h>

#define CHIP8_DISPLAY_WIDTH  64
#define CHIP8_DISPLAY_HEIGHT 32
#define CHIP8_DISPLAY_SIZE   (CHIP8_DISPLAY_WIDTH * CHIP8_DISPLAY_HEIGHT)
#define CHIP8_MEMSIZE        (4096)  /* 4K */
#define REGISTERS            16
#define MAX_STACK_LEVELS     16
#define MAX_KEYPAD_KEYS      16

typedef struct chip8 {
	uint16_t opcode;                   /* The current opcode */
	uint8_t  memory[CHIP8_MEMSIZE];    /* Memory (4K) */
	uint8_t  V[REGISTERS];             /* The V registers (V0-VF) */
	uint16_t I;                        /* I register (Address register). 16 bits wide */
	uint16_t pc;			 /* Program counter */
	uint8_t  gfx[CHIP8_DISPLAY_SIZE];  /* Graphics */
	uint8_t  delay_timer;
	uint8_t  sound_timer;
	uint16_t stack[MAX_STACK_LEVELS];  /* Stack. We support maximum 16 levels of nesting */
	uint16_t sp;                       /* Stack pointer. Points to the next FREE frame of the stack */
	uint8_t  key[MAX_KEYPAD_KEYS];     /* Keypad */
	bool shouldDraw;                         /* To indicate wether we have to redraw the screen or not */
} Chip8;

void init_chip8(Chip8 *);
void load_rom(Chip8 *, const char * const);
void cycle(Chip8 *);

#endif /* _CHIP8_HEADER_ */
