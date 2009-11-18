Global Dim font(15), upscreentimer, *screen.SDL_Surface

Declare update_screen()

Procedure init_sdl()
    If SDL_Init_(#SDL_INIT_VIDEO | #SDL_INIT_TIMER) = -1
        PrintN("Cannot init SDL: " + PeekS(SDL_GetError_()))
        End 1
    EndIf

    *screen = SDL_SetVideoMode_(720, 400, 16, #SDL_HWSURFACE)
    If Not *screen
        PrintN("Cannot set video mode: " + PeekS(SDL_GetError_()))
        End 1
    Endif

    SDL_WM_SetCaption_("xemu", 0)

    RunProgram("tar", "-xjf pics.tar.bz2", GetCurrentDirectory() + "/imgs", #PB_Program_Wait)

    logo = SDL_LoadBMP_("imgs/logo.bmp")
    rcDest.SDL_Rect\x = 0
    rcDest\y = 0
    rcDest\w = 720
    rcDest\h = 400
    SDL_FillRect_(*screen, @rcDest, SDL_MapRGB_(*screen\format, 100, 100, 100, 0))
    rcDest\x = 210
    rcDest\y = 50
    SDL_BlitSurface_(logo, 0, *screen, @rcDest)
    SDL_FreeSurface_(logo)
    SDL_UpdateRect_(*screen, 0, 0, 0, 0)

    SDL_Delay_(1000)

    rcDest\x = 0
    rcDest\y = 0
    rcDest\w = 720
    rcDest\h = 400
    SDL_FillRect_(*screen, @rcDest, SDL_MapRGB_(*screen\format, 0, 0, 0, 0))
    SDL_UpdateRect_(*screen, 0, 0, 0, 0)

    For i = 0 To 15
        font(i) = SDL_LoadBMP_("imgs/font" + Str(i) + ".bmp")
    Next

    RunProgram("bash", "-c " + Chr('"') + "rm imgs/*.bmp" + Chr('"'), GetCurrentDirectory())

    upscreentimer = SDL_AddTimer_(10, @update_screen(), 0)
EndProcedure

Procedure deinit_sdl()
    SDL_RemoveTimer_(upscreentimer)

    SDL_WM_SetCaption_("xemu - Finished", 0)

    event.SDL_Event
    Repeat
        While SDL_PollEvent_(@event)
            Select event\type
                Case #SDL_QUIT
                    End 1
            EndSelect
        Wend
        SDL_Delay_(100)
    ForEver
EndProcedure

Procedure update_screen()
    rcDest.SDL_Rect
    rcSrc.SDL_Rect
    *buf = $B8000

    rcDest\x = 0
    rcDest\y = 0
    rcDest\w = 720
    rcDest\h = 400
    SDL_FillRect_(*screen, @rcDest, SDL_MapRGB_(*screen\format, 0, 0, 0, 0))
    rcSrc\w = 9
    rcSrc\h = 16
    rcDest\w = 9
    rcDest\h = 16
    For y = 0 To 24
        rcDest\y = y * 16;
        For x = 0 To 79
            rcSrc\x = (PeekB(*buf) % 32) * 9
            rcSrc\y = Int(PeekB(*buf) / 32) * 16
            rcDest\x = x * 9
            SDL_BlitSurface_(font(PeekB(*buf + 1) & $F), @rcSrc, *screen, @rcDest)
            *buf + 2
        Next
    Next
    SDL_UpdateRect_(*screen, 0, 0, 0, 0)

    ProcedureReturn 10
EndProcedure
