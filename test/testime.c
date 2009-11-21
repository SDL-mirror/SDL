/* A simple program to test the Input Method support in the SDL library (1.3+) */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SDL.h"
#ifdef HAVE_SDL_TTF
#include "SDL_ttf.h"
#endif

#define DEFAULT_PTSIZE  30
#ifdef __QNXNTO__
    #define DEFAULT_FONT    "/usr/photon/font_repository/tt0003m_.ttf"
#else
    #define DEFAULT_FONT    "/System/Library/Fonts/华文细黑.ttf"
#endif
#define MAX_TEXT_LENGTH 256

SDL_Surface *screen;

#ifdef HAVE_SDL_TTF
TTF_Font *font;
#endif
SDL_Rect textRect, markedRect;
Uint32 lineColor, backColor;
SDL_Color textColor = { 0, 0, 0 };
char text[MAX_TEXT_LENGTH], *markedText;

void usage()
{
    printf("usage: testime [--font fontfile] [--fullscreen]\n");
    exit(0);
}

void InitVideo(int argc, char *argv[])
{
    int width = 640, height = 480;
    int flags = SDL_HWSURFACE;
    const char *fontname = DEFAULT_FONT;
    int fullscreen = 0;

    for (argc--, argv++; argc > 0; argc--, argv++)
    {
        if (strcmp(argv[0], "--help") == 0)
            usage();

        else if (strcmp(argv[0], "--fullscreen") == 0)
            fullscreen = 1;

        else if (strcmp(argv[0], "--font") == 0)
        {
            argc--;
            argv++;

            if (argc > 0)
                fontname = argv[0];
            else
                usage();
        }
    }

    SDL_putenv("SDL_VIDEO_WINDOW_POS=center");
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        fprintf(stderr, "Unable to init SDL: %s\n", SDL_GetError());
        exit(-1);
    }

#ifdef HAVE_SDL_TTF
    /* Initialize fonts */
    TTF_Init();

    font = TTF_OpenFont(fontname, DEFAULT_PTSIZE);
    if (! font)
    {
        fprintf(stderr, "Failed to find font: %s\n", TTF_GetError());
        exit(-1);
    }
#endif

    printf("Using font: %s\n", fontname);
    atexit(SDL_Quit);

    if (fullscreen)
    {
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(&mode);

        width = mode.w;
        height = mode.h;
        fprintf(stderr, "%dx%d\n", width, height);
        flags |= SDL_FULLSCREEN;
    }

    /* Create window */
    screen = SDL_SetVideoMode(width, height, 32, flags);
    if (screen == NULL)
    {
        fprintf(stderr, "Unable to set %dx%d video: %s\n",
                width, height, SDL_GetError());
        exit(-1);
    }
}

void CleanupVideo()
{
    SDL_StopTextInput();
#ifdef HAVE_SDL_TTF
    TTF_CloseFont(font);
    TTF_Quit();
#endif
}

void InitInput()
{
    backColor = SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF);
    lineColor = SDL_MapRGB(screen->format, 0x0, 0x0, 0x0);

    /* Prepare a rect for text input */
    textRect.x = textRect.y = 100;
    textRect.w = screen->w - 2 * textRect.x;
    textRect.h = 50;

    text[0] = 0;
    markedRect = textRect;
    markedText = NULL;

    SDL_StartTextInput();
}

#ifdef HAVE_SDL_TTF
static void RenderText(SDL_Surface *sur,
                        TTF_Font *font,
                        const char *text,
                        int x, int y,
                        SDL_Color color)
{
    SDL_Surface *textSur = TTF_RenderUTF8_Blended(font, text, color);
    SDL_Rect dest = { x, y, textSur->w, textSur->h };

    SDL_BlitSurface(textSur, NULL, sur, &dest);
    SDL_FreeSurface(textSur);
}
#endif

void Redraw()
{
    int w = 0, h = textRect.h;
    SDL_Rect cursorRect, underlineRect;

    SDL_FillRect(screen, &textRect, backColor);

#ifdef HAVE_SDL_TTF
    if (strlen(text))
    {
        RenderText(screen, font, text, textRect.x, textRect.y, textColor);
        TTF_SizeUTF8(font, text, &w, &h);
    }
#endif

    markedRect.x = textRect.x + w;
    markedRect.w = textRect.w - w;
    if (markedRect.w < 0)
    {
        SDL_Flip(screen);
        // Stop text input because we cannot hold any more characters
        SDL_StopTextInput();
        return;
    }
    else
    {
        SDL_StartTextInput();
    }

    cursorRect = markedRect;
    cursorRect.w = 2;
    cursorRect.h = h;

    SDL_FillRect(screen, &markedRect, backColor);
    if (markedText)
    {
#ifdef HAVE_SDL_TTF
        RenderText(screen, font, markedText, markedRect.x, markedRect.y, textColor);
        TTF_SizeUTF8(font, markedText, &w, &h);
#endif

        underlineRect = markedRect;
        underlineRect.y += (h - 2);
        underlineRect.h = 2;
        underlineRect.w = w;

        cursorRect.x += w + 1;

        SDL_FillRect(screen, &underlineRect, lineColor);
    }

    SDL_FillRect(screen, &cursorRect, lineColor);

    SDL_Flip(screen);

    SDL_SetTextInputRect(&markedRect);
}

void
HotKey_ToggleFullScreen(void)
{
    SDL_Surface *screen;

    screen = SDL_GetVideoSurface();
    if (SDL_WM_ToggleFullScreen(screen)) {
        printf("Toggled fullscreen mode - now %s\n",
               (screen->flags & SDL_FULLSCREEN) ? "fullscreen" : "windowed");
    } else {
        printf("Unable to toggle fullscreen mode\n");
    }
}

int main(int argc, char *argv[])
{
    SDL_Event event;
    int done = 0;

    InitVideo(argc, argv);
    InitInput();
    Redraw();

    while (! done && SDL_WaitEvent(&event))
    {
        switch (event.type)
        {
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym)
            {
                case SDLK_ESCAPE:
                     done = 1;
                     break;
                case SDLK_RETURN:
                     text[0]=0x00;
                     Redraw();
                     break;
                case SDLK_BACKSPACE:
                     {
                         int textlen=SDL_strlen(text);

                         do {
                             if (textlen==0)
                             {
                                 break;
                             }
                             if ((text[textlen-1] & 0x80) == 0x00)
                             {
                                 /* One byte */
                                 text[textlen-1]=0x00;
                                 break;
                             }
                             if ((text[textlen-1] & 0xC0) == 0x80)
                             {
                                 /* Byte from the multibyte sequence */
                                 text[textlen-1]=0x00;
                                 textlen--;
                             }
                             if ((text[textlen-1] & 0xC0) == 0xC0)
                             {
                                 /* First byte of multibyte sequence */
                                 text[textlen-1]=0x00;
                                 break;
                             }
                         } while(1);

                         Redraw();
                     }
                     break;
            }

            if (done)
            {
                break;
            }

            fprintf(stderr,
                    "Keyboard %d: scancode 0x%08X = %s, keycode 0x%08X = %s\n",
                    event.key.which, event.key.keysym.scancode,
                    SDL_GetScancodeName(event.key.keysym.scancode),
                    event.key.keysym.sym, SDL_GetKeyName(event.key.keysym.sym));
            break;

        case SDL_TEXTINPUT:
            if (SDL_strlen(event.text.text) == 0 || event.text.text[0] == '\n' ||
                markedRect.w < 0)
                break;

            fprintf(stderr, "Keyboard %d: text input \"%s\"\n",
                    event.text.which, event.text.text);

            if (SDL_strlen(text) + SDL_strlen(event.text.text) < sizeof(text))
                strcpy(text + SDL_strlen(text), event.text.text);

            fprintf(stderr, "text inputed: %s\n", text);

            // After text inputed, we can clear up markedText because it
            // is committed
            markedText = NULL;
            Redraw();
            break;

        case SDL_TEXTEDITING:
            fprintf(stderr, "text editing \"%s\", selected range (%d, %d)\n",
                    event.edit.text, event.edit.start, event.edit.length);

            markedText = event.edit.text;
            Redraw();
            break;

        case SDL_QUIT:
            done = 1;
            break;

        default:
            break;
        }
    }

    CleanupVideo();
    return 0;
}
