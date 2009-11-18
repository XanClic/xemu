Macro SDL_BlitSurface_(p1, p2, p3, p4)
    SDL_UpperBlit_(p1, p2, p3, p4)
EndMacro

Macro SDL_LoadBMP_(file)
    SDL_LoadBMP_RW_(SDL_RWFromFile_(file, "rb"), 1)
EndMacro
