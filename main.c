#include <stdio.h>
#include <stdint.h>
#include "SDL2/SDL.h"
#include "chip8.h"

#define WINDOW_WIDTH 1024
#define WINDOW_HEIGHT 512

unsigned char keymap[MAX_KEYPAD_KEYS] = {
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_4,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_r,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_f,
    SDLK_z,
    SDLK_x,
    SDLK_c,
    SDLK_v,
};

static void init_graphics(SDL_Window **, SDL_Renderer **, SDL_Texture **);
static void init_SDL(void);
static void init_window(SDL_Window **, const char * const title);
static void init_renderer(SDL_Renderer **, SDL_Window *window);
static void init_texture(SDL_Texture **, SDL_Renderer **renderer);
static void update_screen(Chip8 *chip8, SDL_Texture *texture, SDL_Renderer *renderer);
/*static void draw_ascii(Chip8 *);*/

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("Usage: %s <ROM FILE>\n", *argv);
        exit(1);
    }

    SDL_Window *window = NULL;
    SDL_Renderer *renderer = NULL;
    SDL_Texture *texture = NULL;

    init_graphics(&window, &renderer, &texture);
    
    Chip8 chip8;

	init_chip8(&chip8);
    load_rom(&chip8, argv[1]);

    while (1) {
        cycle(&chip8);

        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT)
                exit(0);

            /* Handle keydown event */
            if (e.type == SDL_KEYDOWN) {
                if (e.key.keysym.sym == SDLK_ESCAPE) {
                    exit(0);
                }

                for (int i = 0; i < 16; i++) {
                    if (e.key.keysym.sym == keymap[i]) {
                        chip8.key[i] = 1;
                    }
                }
            }

            /* Handle keyup event */
            if (e.type == SDL_KEYUP) {
                for (int i = 0; i < 16; i++) {
                    if (e.key.keysym.sym == keymap[i]) {
                        chip8.key[i] = 0;
                    }
                }
            }
        }

        if (chip8.shouldDraw) {
            update_screen(&chip8, texture, renderer);
        }
        SDL_Delay(1);
    }
	return 0;
}

/* Initializes the graphics system  */
static void init_graphics(SDL_Window **window, SDL_Renderer **renderer, SDL_Texture **texture)
{
    init_SDL();

    init_window(window, "Chip 8 emulator by Leonardo Guedez");

    init_renderer(renderer, *window);
    SDL_RenderSetLogicalSize(*renderer, 1024, 512);

    init_texture(texture, renderer);
}

/* Initializes SDL */
void init_SDL(void)
{
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0) {
        fprintf(stderr, "ERROR INITIALIZING SDL\n");
        exit(1);
    } else {
        printf("SDL INIT WENT OK\n");
    }
}

static void init_window(SDL_Window **window, const char * const title)
{
    *window = SDL_CreateWindow(title, SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, WINDOW_WIDTH, WINDOW_HEIGHT,
            SDL_WINDOW_SHOWN);

    if (!*window) {
        fprintf(stderr, "ERROR CREATING WINDOW\n");
        exit(1);
    }
}

static void init_renderer(SDL_Renderer **renderer, SDL_Window *window)
{
    *renderer = SDL_CreateRenderer(window, -1, 0);

    if (!*renderer) {
        fprintf(stderr, "ERROR CREATING RENDERER\n");
        exit(1);
    }
}

static void init_texture(SDL_Texture **texture, SDL_Renderer **renderer)
{
    *texture = SDL_CreateTexture(*renderer, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, CHIP8_DISPLAY_WIDTH,
            CHIP8_DISPLAY_HEIGHT);

    if (!*texture) {
        fprintf(stderr, "ERROR CREATING TEXTURE\n");
        exit(1);
    }
}


/* Debug. ASCII draw */
/*
void draw_ascii(Chip8 *chip8)
{
    for (int i = 0; i < 64*32; i++) {
        if (i % 64 == 0) {
            putchar('\n');
        }
        if (chip8->gfx[i]) {
            putchar('1');
        } else {
            putchar(' ');
        }
    }
    putchar('\n');
}*/

static void update_screen(Chip8 *chip8, SDL_Texture *texture, SDL_Renderer *renderer)
{
    chip8->shouldDraw = false;
    uint32_t pixels[2048];

    for (int i = 0; i < CHIP8_DISPLAY_SIZE; i++) {
        uint8_t pixel_is_on = chip8->gfx[i];
        pixels[i] = pixel_is_on ? 0xFFFFFFFF : 0x0;
    }
    SDL_UpdateTexture(texture, NULL, pixels, 64 * sizeof(Uint32));
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);

}
