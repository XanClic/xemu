#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <SDL/SDL.h>


Uint32 update_screen(Uint32 intvl, void *param);


SDL_Surface *screen, *font[16];
SDL_TimerID upscreentimer;


void init_sdl(void)
{
    SDL_Surface *logo;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER ) == -1)
    {
        printf("Cannot init SDL: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    atexit(SDL_Quit);

    screen = SDL_SetVideoMode(720, 400, 16, SDL_HWSURFACE);
    if (screen == NULL)
    {
        printf("Cannot set video mode: %s\n", SDL_GetError());
        exit(EXIT_FAILURE);
    }

    system("cd imgs && tar -xjf pics.tar.bz2");

    SDL_WM_SetCaption("xemu", NULL);

    SDL_Rect rcDest = {0, 0, 720, 400};
    SDL_FillRect(screen, &rcDest, SDL_MapRGB(screen->format, 100, 100, 100));

    logo = SDL_LoadBMP("imgs/logo.bmp");
    rcDest.x = 210;
    rcDest.y = 50;
    SDL_BlitSurface(logo, NULL, screen, &rcDest);
    SDL_FreeSurface(logo);
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    SDL_Delay(1000);

    rcDest.x = 0;
    rcDest.y = 0;
    rcDest.w = 720;
    rcDest.h = 400;
    SDL_FillRect(screen, &rcDest, SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_UpdateRect(screen, 0, 0, 0, 0);

    for (int i = 0; i < 16; i++)
    {
        char name[20];
        sprintf(name, "imgs/font%i.bmp", i);
        font[i] = SDL_LoadBMP(name);
    }

    system("rm imgs/*.bmp");

    upscreentimer = SDL_AddTimer(10, &update_screen, NULL);
}

Uint32 update_screen(Uint32 intvl, void *param)
{
    SDL_Rect rcDest = {0, 0, 9, 16};
    SDL_Rect rcSrc = {0, 0, 9, 16};
    SDL_Rect fscreen = {0, 0, 720, 400};
    uint8_t *buf = (uint8_t *)0xB8000;

    SDL_FillRect(screen, &fscreen, SDL_MapRGB(screen->format, 0, 0, 0));
    for (int y = 0; y < 25; y++)
    {
        rcDest.y = y * 16;
        for (int x = 0; x < 80; x++)
        {
            rcSrc.x = (buf[0] % 32) * 9;
            rcSrc.y = ((int)(buf[0] / 32)) * 16;
            rcDest.x = x * 9;
            SDL_BlitSurface(font[buf[1] & 0xF], &rcSrc, screen, &rcDest);
            buf += 2;
        }
    }
    SDL_UpdateRect(screen, 0, 0, 0, 0);
    return 10;
}

void deinit_sdl(void)
{
    SDL_Event event;

    SDL_RemoveTimer(upscreentimer);

    SDL_WM_SetCaption("xemu - Finished", NULL);

    for (;;)
    {
        while (SDL_PollEvent(&event))
        {
            switch(event.type)
            {
                case SDL_QUIT:
                    exit(EXIT_FAILURE);
            }
        }
        SDL_Delay(100);
    }
}
