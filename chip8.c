#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "chip8.h"
#include "opcode_functions.h"

#define CHIP8_FONTSET_LEN 80

#define OPCODE_FAMILY(opcode) ((opcode) & (0xF000))
#define HIGH_BYTE(chip8) (chip8->memory[chip8->pc] << 8)
#define LOW_BYTE(chip8) (chip8->memory[chip8->pc+1])

/* Fontset of the CHIP-8 interpreter */
static unsigned char chip8_fontset[CHIP8_FONTSET_LEN] =
{
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80  // F
};

/* Table of functions to call in the cycle. We'll call one of these
 * functions based on the "family" (the first nibble, most significant 4 bits)
 * of the upper byte of the opcode */
void (*opcodes_table[])(Chip8 * const) =
{
    family_0,
    opcode_1,
    opcode_2,
    opcode_3,
    opcode_4,
    opcode_5,
    opcode_6,
    opcode_7,
    family_8,
    opcode_9,
    opcode_A,
    opcode_B,
    opcode_C,
    opcode_D,
    family_E,
    family_F
};

#ifdef DEBUG
/* Prints the registers of the Chip8 to the screen */
static void debug_status(Chip8 *chip8)
{
    puts("SYSTEM STATUS:");
    for (int i = 0; i <= 0xF; i++) {
        if (i < 0xA) {
            printf("V%d = %d\n", i, chip8->V[i]);
        } else {
            printf("V%c = %d\n", 'A' + i - 10, chip8->V[i]);
        }
    }
    printf("PC = 0x%X\n", chip8->pc);
    printf("I = 0x%X\n", chip8->I);
    printf("SP = 0x%X\n\n", *(chip8->stack + chip8->sp));
}
#endif

static long get_rom_size(FILE * const);
static inline uint16_t fetch_opcode(const Chip8 * const);

/* Initializes the interpreter */
void init_chip8(Chip8 *chip8)
{
    /* Historically, the first 512 bytes (addresses 0 - 511) of memory were
     * occupied by the interpreter itself, so they were reserved and applications
     * should start at address 0x200. Currently there is no need for this,
     * but it's kept for historical reasons. As it's common, we use these
     * first 512 bytes to store font data */
    chip8->pc = 0x200;
    chip8->opcode = 0;
    chip8->I = 0;
    chip8->sp = 0;
    chip8->shouldDraw = false;

    /* Clear display */
    memset(chip8->gfx, 0, sizeof(chip8->gfx));

    /* Clear stack */
    for (int i = 0; i < MAX_STACK_LEVELS; i++) {
        chip8->stack[i] = 0;
    }

    /* Clear registers V0-VF and keypad */
    for (int i = 0; i < REGISTERS; i++) {
        chip8->V[i] = chip8->key[i] = 0;
    }

    /* Clear memory  */
    memset(chip8->memory, 0, CHIP8_MEMSIZE);

    /* Load fontset */
    memcpy(chip8->memory, chip8_fontset, CHIP8_FONTSET_LEN);

    /* Reset timers */
    chip8->delay_timer = chip8->sound_timer = 0;
    srand(time(NULL));
}

/* Loads the ROM into memory */
void load_rom(Chip8 *chip8, const char * const filename)
{
    FILE *rom = fopen(filename, "r");

    if (!rom) {
        fprintf(stderr, "%s:%d:%s(): ERROR: COULD NOT OPEN ROM FILE \'%s\'.\n", __FILE__, __LINE__, __func__, filename);
        exit(1);
    } else {
        printf("%s(): ROM FILE \'%s\' OPENED SUCCESFULLY\n", __func__, filename);
    }

    /* Get rom size */
    long rom_size = get_rom_size(rom);

    printf("ROM SIZE: %zu bytes\n", rom_size);

    /* As the first 512 bytes are reserved, we only have 4096 - 512 = 3584
     * bytes for application memory. If the rom is larger than that, it
     * will not fit into memory */
    if (rom_size > (CHIP8_MEMSIZE - 512)) {
        fprintf(stderr, "%s(): ROM IS TOO BIG TO FIT INTO MEMORY (%ld bytes)\n", __func__, rom_size);
        exit(1);
    }

    /* Read the program into working memory. As we are reading chars, make
     * sure that the amount read equals file lenght */
    size_t bytes_read = fread(chip8->memory+0x200, sizeof(uint8_t), rom_size, rom);

    if (bytes_read != rom_size) {
        fprintf(stderr, "%s(): ERROR READING ROM FILE\n", __func__);
        fclose(rom);
        exit(1);
    }

    /* Clean up */
    fclose(rom);
}

/* Function to emulate a cycle of the interpreter */
void cycle(Chip8 *chip8)
{
    /* Fetch */
    chip8->opcode = fetch_opcode(chip8);

    /* Decode */
    uint8_t family = OPCODE_FAMILY(chip8->opcode) >> 12;

    /* Execute */
    opcodes_table[family](chip8);

#ifdef DEBUG
    printf("OPCODE 0x%04X\n", chip8->opcode);
    debug_status(chip8);
#endif

    /* Updates timers */
    if (chip8->delay_timer > 0)
        --chip8->delay_timer;

    if (chip8->sound_timer > 0) {
        if (chip8->sound_timer == 1) {
            /* Makes sound */
            --chip8->sound_timer;
        }
    }
}

/* Gets the size (in bytes) of the rom file */
static long get_rom_size(FILE * const rom)
{
    fseek(rom, 0L, SEEK_END);
    long file_size = ftell(rom);
    rewind(rom);
    return file_size;
}

/* Returns the next opcode of the program */
static inline uint16_t fetch_opcode(const Chip8 * const chip8)
{
    /* Get the first byte, move it to the left to make room for the second one
     * and OR them to form a short */
    return HIGH_BYTE(chip8) | LOW_BYTE(chip8);
}
