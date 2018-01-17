#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "chip8.h"

#define OP_FAMILY(opcode) ((opcode) & (0xF000))
#define OP_NNN(opcode) ((opcode) & (0x0FFF))
#define OP_NN(opcode)  ((opcode) & (0x00FF))
#define OP_N(opcode)   ((opcode) & (0x000F))
#define OP_X(opcode)   (((opcode) & (0x0F00)) >> 8)
#define OP_Y(opcode)   (((opcode) & (0x00F0)) >> 4)

static unsigned char chip8_fontset[80] =
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

#ifdef DEBUG
/* Prints the registers of the Chip8 to the screen */
static void debug_status(Chip8 *chip8)
{
    puts("SYSTEM STATUS:");
    for (int i = 0; i <= 0xF; i++)
        printf("V%c = %d\n", (i >= 0xA) ? ('A' + i - 10) : (chip8->V[i]), chip8->V[i]);
    printf("PC = 0x%X\n", chip8->pc);
    printf("I = 0x%X\n", chip8->I);
    printf("SP = 0x%X\n\n", *(chip8->stack + chip8->sp));
}
#endif

static long get_rom_size(FILE * const);
static inline unsigned short get_opcode(const Chip8 * const);

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

    /* Clear stack, registers V0-VF and keypad */
    for (int i = 0; i < 16; i++) {
        chip8->stack[i] = chip8->V[i] = chip8->key[i] = 0;
    }

    /* Clear memory  */
    memset(chip8->memory, 0, 4096);

    /* Load fontset */
    memcpy(chip8->memory, chip8_fontset, 80);

    /* Reset timers */
    chip8->delay_timer = chip8->sound_timer = 0;
    srand(time(NULL));
}

void load_rom(Chip8 *chip8, const char * const filename)
{
    FILE *rom = fopen(filename, "r");

    if (!rom) {
        fprintf(stderr, "ERROR OPENING ROM FILE: %s\n", filename);
        exit(1);
    } else {
        printf("ROM FILE OPENED SUCCESFULLY\n");
    }

    /* Get rom size */
    long rom_size = get_rom_size(rom);

    printf("ROM SIZE: %zu\n", rom_size);

    /* As the first 512 bytes are reserved, we only have 4096 - 512 = 3584
     * bytes for application memory. If the rom is larger than that, it
     * will not fit into memory */
    if (rom_size > (4096 - 512)) {
        fprintf(stderr, "ROM IS TOO BIG TO FIT INTO MEMORY (%ld bytes)\n", rom_size);
        exit(1);
    }

    /* Read the program into working memory. As we are reading chars, make
     * sure that the amount read equals file lenght */
    size_t bytes_read = fread(chip8->memory+0x200, sizeof(unsigned char), rom_size, rom);
    if (bytes_read != rom_size) {
        fprintf(stderr, "ERROR READING ROM FILE\n");
        fclose(rom);
        exit(1);
    }

    /* Clean up */
    fclose(rom);
}

void cycle(Chip8 *chip8)
{
    /* Gets the next opcode */
    chip8->opcode = get_opcode(chip8);

    /* Gets the parameter used by most of the opcodes */
    unsigned short NNN = OP_NNN(chip8->opcode);
    unsigned char NN = OP_NN(chip8->opcode);
    unsigned char X = OP_X(chip8->opcode);
    unsigned char Y = OP_Y(chip8->opcode);
    bool key_pressed = false;

#ifdef DEBUG
    printf("OPCODE 0x%04X\n", chip8->opcode);
#endif

    /* WARNING: Really big switch statement. Not for the faint of heart. */
    switch (OP_FAMILY(chip8->opcode)) {
        case 0x0000:
            switch (chip8->opcode & 0x00FF) {
                /* 0x00E0: Clear the display */
                case 0xE0:
                    memset(chip8->gfx, 0, 2048);
                    chip8->shouldDraw = true;
                    break;
                
                /* 0x00EE: Return from a subroutine */
                case 0xEE:
                    chip8->pc = chip8->stack[--chip8->sp];
                    break;

                default:
                    fprintf(stderr, "[0x0000] OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
                    exit(1);
            }
            chip8->pc += 2;
            break;

        /* 1NNN: Jumps to address NNN */
        case 0x1000:
            chip8->pc = NNN;
            break;

        /* 2NNN: Calls subroutine at address NNN */
        case 0x2000:
            chip8->stack[chip8->sp] = chip8->pc;
            chip8->sp++;
            chip8->pc = NNN;
            break;

        /* 3XNN: Skips the next instruction if VX equals NN */
        case 0x3000:
            if (chip8->V[X] == NN) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;

        /* 4XNN: Skips the next instruction if VX doesn't equals NN */
        case 0x4000:
            if (chip8->V[X] != NN) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;

        /* 5XY0: Skips the next instruction if VX equals VY */
        case 0x5000:
            if (chip8->V[X] == chip8->V[Y]) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;

        /* 6XNN: Sets VX to NN */
        case 0x6000:
            chip8->V[X] = NN;
            chip8->pc += 2;
            break;

        /* 7XNN: Adds NN to VX (carry flag is not changed) */
        case 0x7000:
            chip8->V[X] += NN;
            chip8->pc += 2;
            break;

        /* 8XYN: Bitwise and math ops */
        case 0x8000:
            switch (chip8->opcode & 0x000F) {
            /* 8XY0: Sets VX to the value of VY */
            case 0x0:
                chip8->V[X] = chip8->V[Y];
                break;

            /* 8XY1: Sets VX to VX OR VY (bitwise OR) */
            case 0x1:
                chip8->V[X] |= chip8->V[Y];
                break;

            /* 8XY2: Sets VX to VX AND VY (bitwise AND) */
            case 0x2:
                chip8->V[X] &= chip8->V[Y];
                break;

            /* 8XY3: Sets VX to VX XOR VY */
            case 0x3:
                chip8->V[X] ^= chip8->V[Y];
                break;

            /* 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and
             * to 0 when there isn't */
            case 0x4:
                /* If there's carry, set the VF register to 1 */
                if ((chip8->V[X] + chip8->V[Y]) > 0xFF)
                    chip8->V[0xF] = 1;
                else
                    chip8->V[0xF] = 0;

                chip8->V[X] += chip8->V[Y];
                break;

            /* 8XY5: VY is substracted from VX. VF is set to 0 when there's
             * a borrow, and to 1 when there isn't*/
            case 0x5:
                if (chip8->V[X] > chip8->V[Y]) {
                    /* No borrow */
                    chip8->V[0xF] = 1;
                } else {
                    /* Borrow */
                    chip8->V[0xF] = 0;
                }
                chip8->V[X] -= chip8->V[Y];
                break;

            /* 8XY6: If the least significant bit of VX is 1, then VF is set
             * to 1, otherwise to 0. Then VX is divided by 2.*/
            case 0x6:
                chip8->V[0xF] = chip8->V[X] & 0x01;
                chip8->V[X] >>= 1;
                break;

            /* 8XY7: If VY > VX, then VF is set to 1, otherwise to 0. Then VX is
             * substracted from VY, and the results stored in VX */
            case 0x7:
                if (chip8->V[Y] > chip8->V[X])
                    /* No borrow */
                    chip8->V[0xF] = 1;
                else
                    /* Borrow */
                    chip8->V[0xF] = 0;
                chip8->V[X] = chip8->V[Y] - chip8->V[X];
                break;

            /* 8XYE: IF the most significant bit of VX es 1, then VF is set
             * to 1, otherwise to 0. Then VX is multiplied by 2 */
            case 0xE:
                chip8->V[0xF] = chip8->V[X] & 0x80;
                chip8->V[X] <<= 1;
                break;

            default:
                fprintf(stderr, "[8XNN] OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
                exit(1);
            } /* 8XYN switch */
            chip8->pc += 2;
            break;

        /* 9XY0: Skips the next instruction if VX doesn't equal VY */
        case 0x9000:
            if (chip8->V[X] != chip8->V[Y]) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;

        /* ANNN: Sets register I to the address NNN */
        case 0xA000:
            chip8->I = NNN;
            chip8->pc += 2;
            break;

        /* BNNN: Jumps to the address NNN plus V0 */
        case 0xB000:
            chip8->pc = NNN + chip8->V[0];
            break;

        /* CXNN: Sets VX to the result of a bitwise AND operation on a random
         * number and NN */
        case 0xC000:
            chip8->V[X] = rand() & NN;
            chip8->pc += 2;
            break;

        /* Draws a sprite at coordinates (VX, VY) that has a width of 8 pixels
         * and a height of N pixels (so, it reads N bytes, where each of these
         * bytes is a row of the sprite). Each row of 8 pixels is read as
         * bit-coded starting from memory location I; I value DOESN'T change
         * after the execution of this instruction.
         * VF is set to 1 if any screen pixels are flipped from set to unset
         * when the sprite is drawn, and to 0 if that doesn't happen */
        case 0xD000: {
            unsigned short x = chip8->V[X];
            unsigned short y = chip8->V[Y];
            unsigned short height = chip8->opcode & 0x000F;
            unsigned short pixel;

            chip8->V[0xF] = 0;
            for (int yline = 0; yline < height; yline++) {

                pixel = chip8->memory[chip8->I + yline];

                for (int xline = 0; xline < 8; xline++) {
                    if ((pixel & (0x80 >> xline)) != 0) {
                        /* E.g: Suppose that we want to convert a 2D coordinate
                         * into a 1D index. Suppose that we have the
                         * below screen matrix shown below:
                         *   _0_1_2__
                         * 0 |8 2 1|
                         * 1 |3 0 5|
                         * 2 |6 4 7|
                         * 3 |9 A B|
                         * 4 |C D E|
                         *
                         * So how do we, for example, get element 4
                         * (row = 2, column = 1)?
                         * Well, if we compact the matrix "by row" into an
                         * array, it would look like this:
                         *
                         *    0     1      2      3      4 (indices of rows in matrix)
                         * [8 2 1; 3 0 5; 6 4 7; 9 A B; C D E]
                         *
                         * We can observe that to get to an specific index,
                         * we would need to "skip by row". To do this
                         * we multiply the row that we want to get by the
                         * number of columns in the row, and add that to the
                         * column that we want. Expressing it in an ecuation:
                         *
                         * i = column + (row * no_of_columns);
                         *
                         * So in our case it would be:
                         *
                         * i = 1 + (2 * 3) = 1 + 6 = 7
                         * 
                         * We can see that array[7] is in fact 4. In the
                         * present case, we add xline and yline because
                         * we are drawing a whole sprite, not just a single
                         * pixel. So xline accounts for every column of the
                         * sprite that we want to draw, and yline for every
                         * row. If we'd just wanted to draw a pixel, it would
                         * suffice with x + (y * 64).
                         *
                         * */
                        int bit = (x + xline + ((y + yline) * 64));
                        if (chip8->gfx[bit] == 1)
                            chip8->V[0xF] = 1;
                        chip8->gfx[bit] ^= 1;
                    }
                }
            }
            chip8->shouldDraw = true;
            chip8->pc += 2;
        } /* DXYN */
        break;

        /* EXNN: Skip instruction depending on state of certain key */
        case 0xE000:

            switch (chip8->opcode & 0x00FF) {
            /* EX9E: Skip next instruction if key with the value of VX is
             * pressed*/
            case 0x009E:
                if (chip8->key[chip8->V[X]] != 0) {
                    chip8->pc += 4;
                } else {
                    chip8->pc += 2;
                }
                break;

            /* EXA1: Skip next instruction if key with the value of VX is
             * not pressed */
            case 0x00A1:
                if (chip8->key[chip8->V[X]] == 0) {
                    chip8->pc += 4;
                } else {
                    chip8->pc += 2;
                }
                break;
            default:
                fprintf(stderr, "[EXNN] OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
                exit(1);
            } /* EXNN switch */
            break;

        /* FXNN: Miscelaneous operations */
        case 0xF000:

            switch (chip8->opcode & 0x00FF) {
            /* FX07: The value of delay timer is stored into VX */
            case 0x07:
                chip8->V[OP_X(chip8->opcode)] = chip8->delay_timer;
                break;

            /* FX0A: Wait for a key press, store the value of the key in VX */
            case 0x0A:
                /* Poll the keyboard to see if a key was pressed */
                for (int i = 0; i < 16; i++) {
                    if (chip8->key[i] != 0) {
                        key_pressed = true;
                        chip8->V[OP_X(chip8->opcode)] = i;
                        break;
                    }
                }

                if (!key_pressed)
                    return /* to the main loop */;

                break;

            /* FX15: Delay timer is set equal to the value of VX */
            case 0x15:
                chip8->delay_timer = chip8->V[X];
                break;

            /* FX18: Sound timer is set equal to the value of VX */
            case 0x18:
                chip8->sound_timer = chip8->V[X];
                break;

            /* FX1E: The values of I and VX are added, and the results
             * stored in I */
            case 0x1E:

                /* VF is set to 1 when there is a range overflow, and to
                 * 0 when there isn't. Undocumented feature! */
                if (chip8->I + chip8->V[X] > 0xFFF)
                    chip8->V[0xF] = 1;
                else
                    chip8->V[0xF] = 0;
                chip8->I += chip8->V[X];
                break;

            /* FX29: The value of I is set to the location for the hexadecimal
             * sprite corresponding to the value of VX */
            case 0x29:
                /* Multiply by 5 to "skip" rows of the fontset */
                /* If VX = 4, then i = 4 * 5 = 20, so I would point to
                 * element at position 20 of the fontset array, which
                 * is where the sprite for character "4" starts */
                chip8->I = chip8->V[X] * 0x5;
                break;

            /* FX33: Store BCD representation of VX in memory locations I,
             * I+1 and I+2. Takes the decimal value of VX, and places the
             * hundreds digit in memory location memory[I], the tens digit at
             * location memory[I + 1], and the ones digit at location
             * memory[I + 2] */
            case 0x33:
                chip8->memory[chip8->I    ] = chip8->V[X] / 100;
                chip8->memory[chip8->I + 1] = (chip8->V[X] % 100) / 10;
                chip8->memory[chip8->I + 2] = chip8->V[X] % 10;
                break;

            /* FX55: Store registers V0 through VX (including VX) in memory
             * starting at location I */
            case 0x55:
                for (int i = 0; i <= X; i++) {
                    chip8->memory[chip8->I + i] = chip8->V[i];
                }
                chip8->I += X + 1;
                break;

            /* FX65: Read registers V0 through VX (including VX) from memory
             * starting at location I */
            case 0x65:
                for (int i = 0; i <= X; i++) {
                    chip8->V[i] = chip8->memory[chip8->I + i];
                }
                chip8->I += X + 1;
                break;

            default:
                fprintf(stderr, "[FXNN] OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
                exit(1);

            } /* FXNN switch */
            chip8->pc += 2;
            break;

        default:
            fprintf(stderr, "OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
            exit(1);
            break;
    }

#ifdef DEBUG
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

static long get_rom_size(FILE * const rom)
{
    fseek(rom, 0L, SEEK_END);
    long file_size = ftell(rom);
    rewind(rom);
    return file_size;
}

static inline unsigned short get_opcode(const Chip8 * const chip8)
{
    return chip8->memory[chip8->pc] << 8 | chip8->memory[chip8->pc+1];
}
