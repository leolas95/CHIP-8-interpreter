/* This file contains the functions that acts as the opcodes of the
 * interpreter. This functions can be saved into an array of function
 * pointers so we can call them simply with the correct index.
 * The purpose of this is to avoid using a big switch statement to execute
 * the corresponding opcode */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "opcode_functions.h"

/* Macros to get the arguments of the opcode */
#define OPCODE_NNN(opcode) ((opcode)  & (0x0FFF))
#define OPCODE_NN(opcode)  ((opcode)  & (0x00FF))
#define OPCODE_N(opcode)   ((opcode)  & (0x000F))
#define OPCODE_X(opcode)   (((opcode) & (0x0F00)) >> 8)
#define OPCODE_Y(opcode)   (((opcode) & (0x00F0)) >> 4)

/* Prints msg to stderr and exits */
static void die(const char * const msg, uint16_t opcode)
{
    fprintf(stderr, msg, opcode);
    exit(EXIT_FAILURE);
}

void family_0(Chip8 * const chip8)
{
/* Maybe we could create another table for each opcode that has a family?
 * So functions like this and family_8, etc. could actually
 * just call another function based on the switch expression */
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
            die("[0x0000] OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
    }
    chip8->pc += 2;
}

/* 1NNN: Jumps to address NNN */
void opcode_1(Chip8 * const chip8)
{
    /* If trying to access a memory location out of range, we die */
    uint16_t addr = OPCODE_NNN(chip8->opcode);
    if (addr > 0xFFF || addr < 0x200) {
        die("ERROR: ADDRESS 0x%04X OUT OF VALID RANGE\n", addr);
    }
    chip8->pc = OPCODE_NNN(chip8->opcode);
}

/* 2NNN: Calls subroutine at address NNN */
void opcode_2(Chip8 * const chip8)
{
    uint16_t addr = OPCODE_NNN(chip8->opcode);
    if (addr > 0xFFF || addr < 0x200) {
        die("ERROR: ADDRESS 0x%04X OUT OF VALID RANGE\n", addr);
    }

    chip8->stack[chip8->sp] = chip8->pc;
    chip8->sp++;
    chip8->pc = addr;
}

/* 3XNN: Skips the next instruction if VX equals NN */
void opcode_3(Chip8 * const chip8)
{
    if (chip8->V[OPCODE_X(chip8->opcode)] == OPCODE_NN(chip8->opcode)) {
        chip8->pc += 4;
    } else {
        chip8->pc += 2;
    }
}

/* 4XNN: Skips the next instruction if VX doesn't equals NN */
void opcode_4(Chip8 * const chip8)
{
    if (chip8->V[OPCODE_X(chip8->opcode)] != OPCODE_NN(chip8->opcode)) {
        chip8->pc += 4;
    } else {
        chip8->pc += 2;
    }
}

/* 5XY0: Skips the next instruction if VX equals VY */
void opcode_5(Chip8 * const chip8)
{
    if (chip8->V[OPCODE_X(chip8->opcode)] == chip8->V[OPCODE_Y(chip8->opcode)]) {
        chip8->pc += 4;
    } else {
        chip8->pc += 2;
    }
}

/* 6XNN: Sets VX to NN */
void opcode_6(Chip8 * const chip8)
{
    chip8->V[OPCODE_X(chip8->opcode)] = OPCODE_NN(chip8->opcode);
    chip8->pc += 2;
}

/* 7XNN: Adds NN to VX (carry flag is not changed) */
void opcode_7(Chip8 * const chip8)
{
    chip8->V[OPCODE_X(chip8->opcode)] += OPCODE_NN(chip8->opcode);
    chip8->pc += 2;
}

/* 8XYN: Bitwise and math ops */
void family_8(Chip8 * const chip8)
{
    switch (chip8->opcode & 0x000F) {
        /* 8XY0: Sets VX to the value of VY */
        case 0x0:
            chip8->V[OPCODE_X(chip8->opcode)] = chip8->V[OPCODE_Y(chip8->opcode)];
            break;

        /* 8XY1: Sets VX to VX OR VY (bitwise OR) */
        case 0x1:
            chip8->V[OPCODE_X(chip8->opcode)] |= chip8->V[OPCODE_Y(chip8->opcode)];
            break;

        /* 8XY2: Sets VX to VX AND VY (bitwise AND) */
        case 0x2:
            chip8->V[OPCODE_X(chip8->opcode)] &= chip8->V[OPCODE_Y(chip8->opcode)];
            break;

        /* 8XY3: Sets VX to VX XOR VY */
        case 0x3:
            chip8->V[OPCODE_X(chip8->opcode)] ^= chip8->V[OPCODE_Y(chip8->opcode)];
            break;

        /* 8XY4: Adds VY to VX. VF is set to 1 when there's a carry, and
         * to 0 when there isn't */
        case 0x4:
            /* If there's carry, set the VF register to 1 */
            if ((chip8->V[OPCODE_X(chip8->opcode)] + chip8->V[OPCODE_Y(chip8->opcode)]) > 0xFF)
                chip8->V[0xF] = 1;
            else
                chip8->V[0xF] = 0;

            chip8->V[OPCODE_X(chip8->opcode)] += chip8->V[OPCODE_Y(chip8->opcode)];
            break;

        /* 8XY5: VX = VX - VY. Subtract the value of register VY from register
         * VX. Set VF to 0 if a borrow occurs, to 1 otherwise. */
        case 0x5:
            if (chip8->V[OPCODE_Y(chip8->opcode)] > chip8->V[OPCODE_X(chip8->opcode)]) {
                /* Borrow */
                chip8->V[0xF] = 0;
            } else {
                /* No borrow */
                chip8->V[0xF] = 1;
            }
            
            chip8->V[OPCODE_X(chip8->opcode)] -= chip8->V[OPCODE_Y(chip8->opcode)];
            break;

        /* 8XY6: VX = VY >> 1. Store the value of register VY shifted right
         * one bit in register VX. Set register VF to the least significant
         * bit prior to the shift */
        case 0x6:
            chip8->V[0xF] = chip8->V[OPCODE_X(chip8->opcode)] & 0x01;
            chip8->V[OPCODE_X(chip8->opcode)] = (chip8->V[OPCODE_Y(chip8->opcode)] >>= 1);
            break;

        /* 8XY7: VX = VY - VX. Set register VX to the value of VY minus VX
         * Set VF to 0 if a borrow occurs, to 1 otherwise. */
        case 0x7:
            if (chip8->V[OPCODE_X(chip8->opcode)] > chip8->V[OPCODE_Y(chip8->opcode)]) {
                /* Borrow */
                chip8->V[0xF] = 0;
            } else {
                /* No borrow */
                chip8->V[0xF] = 1;
            }

            chip8->V[OPCODE_X(chip8->opcode)] = chip8->V[OPCODE_Y(chip8->opcode)] - chip8->V[OPCODE_X(chip8->opcode)];
            break;

        /* 8XYE: VX = VY << 1. Store the value of register VY shifted left one
         * bit in register VX. Set register VF to the most significant bit
         * prior to the shift. */
        case 0xE:
            chip8->V[0xF] = chip8->V[OPCODE_X(chip8->opcode)] & 0x80;
            chip8->V[OPCODE_X(chip8->opcode)] = (chip8->V[OPCODE_Y(chip8->opcode)] <<= 1);
            break;

        default:
            die("[8XNN] OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
        } /* 8XYN switch */

    chip8->pc += 2;
}

/* 9XY0: Skips the next instruction if VX doesn't equal VY */
void opcode_9(Chip8 * const chip8)
{
    if (chip8->V[OPCODE_X(chip8->opcode)] != chip8->V[OPCODE_Y(chip8->opcode)]) {
        chip8->pc += 4;
    } else {
        chip8->pc += 2;
    }
}

 /* ANNN: Sets register I to the address NNN */
void opcode_A(Chip8 * const chip8)
{
    chip8->I = OPCODE_NNN(chip8->opcode);
    chip8->pc += 2;
}

/* BNNN: Jumps to the address NNN plus V0 */
void opcode_B(Chip8 * const chip8)
{
    chip8->pc = OPCODE_NNN(chip8->opcode) + chip8->V[0];
}

/* CXNN: Sets VX to the result of a bitwise AND operation on a random
         * number and NN */
void opcode_C(Chip8 * const chip8)
{
    chip8->V[OPCODE_X(chip8->opcode)] = (rand() % 0xFF) & OPCODE_NN(chip8->opcode);
    chip8->pc += 2;
}

/* Draws a sprite at coordinates (VX, VY) that has a width of 8 pixels
 * and a height of N pixels (so, it reads N bytes, where each of these
 * bytes is a row of the sprite). Each row of 8 pixels is read as
 * bit-coded starting from memory location I; I value DOESN'T change
 * after the execution of this instruction.
 * VF is set to 1 if any screen pixels are flipped from set to unset
 * when the sprite is drawn, and to 0 if that doesn't happen */
void opcode_D(Chip8 * const chip8)
{
    uint8_t x = chip8->V[OPCODE_X(chip8->opcode)];
    uint8_t y = chip8->V[OPCODE_Y(chip8->opcode)];
    uint8_t height = OPCODE_N(chip8->opcode);
    uint8_t pixel;

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
                 * column that we want. Expressing it in an equation:
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
}

/* EXNN: Skip instruction depending on state of certain key */
void family_E(Chip8 * const chip8)
{
    switch (chip8->opcode & 0x00FF) {
        /* EX9E: Skip next instruction if key with the value of VX is
         * pressed*/
        case 0x009E:
            if (chip8->key[chip8->V[OPCODE_X(chip8->opcode)]] != 0) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;

        /* EXA1: Skip next instruction if key with the value of VX is
         * not pressed */
        case 0x00A1:
            if (chip8->key[chip8->V[OPCODE_X(chip8->opcode)]] == 0) {
                chip8->pc += 4;
            } else {
                chip8->pc += 2;
            }
            break;

        default:
            die("[EXNN] OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
    } /* EXNN switch */
}

/* FXNN: Miscelaneous operations */
void family_F(Chip8 * const chip8)
{
    switch (chip8->opcode & 0x00FF) {
        /* FX07: The value of delay timer is stored into VX */
        case 0x07:
            chip8->V[OPCODE_X(chip8->opcode)] = chip8->delay_timer;
            break;

        /* FX0A: Wait for a key press, store the value of the key in VX */
        case 0x0A: {
            bool key_pressed = false;
            /* Poll the keyboard to see if a key was pressed */
            for (int i = 0; i < 16; i++) {
                if (chip8->key[i] != 0) {
                    key_pressed = true;
                    chip8->V[OPCODE_X(chip8->opcode)] = i;
                    break;
                }
            }

            if (!key_pressed)
                return /* to the main loop */;
           }
            break;

        /* FX15: Delay timer is set equal to the value of VX */
        case 0x15:
            chip8->delay_timer = chip8->V[OPCODE_X(chip8->opcode)];
            break;

        /* FX18: Sound timer is set equal to the value of VX */
        case 0x18:
            chip8->sound_timer = chip8->V[OPCODE_X(chip8->opcode)];
            break;

        /* FX1E: The values of I and VX are added, and the results
         * stored in I */
        case 0x1E:

            /* VF is set to 1 when there is a range overflow, and to
             * 0 when there isn't. Undocumented feature! */
            if (chip8->I + chip8->V[OPCODE_X(chip8->opcode)] > 0xFFF)
                chip8->V[0xF] = 1;
            else
                chip8->V[0xF] = 0;
            chip8->I += chip8->V[OPCODE_X(chip8->opcode)];
            break;

        /* FX29: The value of I is set to the location for the hexadecimal
         * sprite corresponding to the value of VX */
        case 0x29:
            /* Multiply by 5 to "skip" rows of the fontset */
            /* If VX = 4, then i = 4 * 5 = 20, so I would point to
             * element at position 20 of the fontset array, which
             * is where the sprite for character "4" starts */
            chip8->I = chip8->V[OPCODE_X(chip8->opcode)] * 0x5;
            break;

        /* FX33: Store BCD representation of VX in memory locations I,
         * I+1 and I+2. Takes the decimal value of VX, and places the
         * hundreds digit in memory location memory[I], the tens digit at
         * location memory[I + 1], and the ones digit at location
         * memory[I + 2] */
        case 0x33:
            chip8->memory[chip8->I    ] = chip8->V[OPCODE_X(chip8->opcode)] / 100;
            chip8->memory[chip8->I + 1] = (chip8->V[OPCODE_X(chip8->opcode)] % 100) / 10;
            chip8->memory[chip8->I + 2] = chip8->V[OPCODE_X(chip8->opcode)] % 10;
            break;

        /* FX55: Store the values of registers V0-VX (including) in memory
         * starting at address I. I is set to I + X + 1 after operation */
        case 0x55:
            for (int i = 0; i <= OPCODE_X(chip8->opcode); i++) {
                chip8->memory[chip8->I + i] = chip8->V[i];
            }

            chip8->I = chip8->I + OPCODE_X(chip8->opcode) + 1;
            break;

        /* FX65: Fill registers V0-VX (inclusive) with the values stored in
         * memory starting at address I. I is set to I + X + 1 after operation */
        case 0x65:
            for (int i = 0; i <= OPCODE_X(chip8->opcode); i++) {
                chip8->V[i] = chip8->memory[chip8->I + i];
            }
            chip8->I = chip8->I + OPCODE_X(chip8->opcode) + 1;
            break;

        default:
            die("[FXNN] OPCODE 0x%04X NOT RECOGNIZED\n", chip8->opcode);
        } /* FXNN switch */
    chip8->pc += 2;
}
