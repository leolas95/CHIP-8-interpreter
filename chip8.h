#define DISPLAY_SIZE (64 * 32)
#include <stdbool.h>

typedef struct chip8 {
	unsigned short opcode;              /* The current opcode */
	unsigned char  memory[4096];        /* Memory (4K) */
	unsigned char  V[16];               /* The V registers (V0-VF) */
	unsigned short I;                   /* I register (Address register). 16 bits wide */
	unsigned short pc;                  /* Program counter */
	unsigned char  gfx[DISPLAY_SIZE];   /* Graphics */
	unsigned char  delay_timer;
	unsigned char  sound_timer;
	unsigned short stack[16];           /* Stack. We support maximum 16 levels of nesting */
	unsigned short sp;                  /* Stack pointer. Points to the next FREE frame of the stack */
	unsigned char  key[16];             /* Keypad */
	bool shouldDraw;                    /* To indicate wether we have to redraw the screen or not */
} Chip8;

void init_chip8(Chip8 *);
void load_rom(Chip8 *, const char * const);
void cycle(Chip8 *);
