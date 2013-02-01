#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <SDL/SDL.h>

#include "memory.h"


Uint32 update_screen(Uint32 intvl, void *param);


SDL_Surface *screen, *font;
SDL_TimerID upscreentimer;


void init_sdl(void)
{
    SDL_Surface *logo;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == -1)
    {
        fprintf(stderr, "Cannot init SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(720, 400, 16, SDL_HWSURFACE);
    if (screen == NULL)
    {
        fprintf(stderr, "Cannot set video mode: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    SDL_WM_SetCaption("xemu", NULL);

    SDL_Rect rcDest = {0, 0, 720, 400};
    SDL_FillRect(screen, &rcDest, SDL_MapRGB(screen->format, 100, 100, 100));

    rcDest.x = 0;
    rcDest.y = 0;
    rcDest.w = 720;
    rcDest.h = 400;
    SDL_FillRect(screen, &rcDest, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    font = SDL_LoadBMP("imgs/font.bmp");
    SDL_SetColorKey(font, SDL_SRCCOLORKEY, 0);

    upscreentimer = SDL_AddTimer(10, &update_screen, NULL);
}

static int colors[16][3] = {
    {   0,   0,   0 },
    {   0,   0, 176 },
    {   0, 176,   0 },
    {   0, 176, 176 },
    { 176,   0,   0 },
    { 176,   0, 176 },
    { 176, 176,   0 },
    { 176, 176, 176 },
    {  88,  88,  88 },
    {  88,  88, 255 },
    {  88, 255,  88 },
    {  88, 255, 255 },
    { 255,  88,  88 },
    { 255,  88, 255 },
    { 255, 255,  88 },
    { 255, 255, 255 }
};

Uint32 update_screen(Uint32 intvl, void *param)
{
    SDL_Rect rcDest = {0, 0, 9, 16};
    SDL_Rect rcSrc = {0, 0, 9, 16};

    // TODO: keep physical
    uint8_t *buf = (uint8_t *)adr_g2h(0xb8000);

    for (int y = 0; y < 25; y++)
    {
        rcDest.y = y * 16;
        for (int x = 0; x < 80; x++)
        {
            rcSrc.x = (buf[0] % 32) * 9;
            rcSrc.y = ((int)(buf[0] / 32)) * 16;
            rcDest.x = x * 9;

            SDL_FillRect(screen, &rcDest, SDL_MapRGB(screen->format, colors[buf[1] >> 4][0], colors[buf[1] >> 4][1], colors[buf[1] >> 4][2]));
            SDL_SetPalette(font, SDL_LOGPAL, &(SDL_Color){ colors[buf[1] & 0xF][0], colors[buf[1] & 0xF][1], colors[buf[1] & 0xF][2] }, 1, 1);
            SDL_BlitSurface(font, &rcSrc, screen, &rcDest);

            buf += 2;
        }
    }

    SDL_UpdateRect(screen, 0, 0, 0, 0);


    SDL_Event event;

    while (SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_QUIT:
                exit(EXIT_FAILURE);
        }
    }


    return 10;
}
