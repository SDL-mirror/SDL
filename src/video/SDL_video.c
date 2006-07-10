/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"

/* The high-level video driver subsystem */

#include "SDL.h"
#include "SDL_sysvideo.h"
#include "SDL_blit.h"
#include "SDL_pixels_c.h"
#include "SDL_renderer_sw.h"
#include "../events/SDL_sysevents.h"
#include "../events/SDL_events_c.h"

/* Available video drivers */
static VideoBootStrap *bootstrap[] = {
#if SDL_VIDEO_DRIVER_QUARTZ
    &QZ_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_X11
    &X11_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_DGA
    &DGA_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_NANOX
    &NX_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_IPOD
    &iPod_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_QTOPIA
    &Qtopia_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_WSCONS
    &WSCONS_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_FBCON
    &FBCON_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_DIRECTFB
    &DirectFB_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_PS2GS
    &PS2GS_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_GGI
    &GGI_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_VGL
    &VGL_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_SVGALIB
    &SVGALIB_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_GAPI
    &GAPI_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_WIN32
    &WIN32_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_BWINDOW
    &BWINDOW_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_TOOLBOX
    &TOOLBOX_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_DRAWSPROCKET
    &DSp_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_CYBERGRAPHICS
    &CGX_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_PHOTON
    &ph_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_EPOC
    &EPOC_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_XBIOS
    &XBIOS_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_GEM
    &GEM_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_PICOGUI
    &PG_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_DC
    &DC_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_RISCOS
    &RISCOS_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_OS2FS
    &OS2FSLib_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_AALIB
    &AALIB_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_DUMMY
    &DUMMY_bootstrap,
#endif
#if SDL_VIDEO_DRIVER_GLSDL
    &glSDL_bootstrap,
#endif
    NULL
};

static SDL_VideoDevice *_this = NULL;

/* Various local functions */
int SDL_VideoInit(const char *driver_name, Uint32 flags);
void SDL_VideoQuit(void);

static int
cmpmodes(const void *A, const void *B)
{
    SDL_DisplayMode a = *(const SDL_DisplayMode *) A;
    SDL_DisplayMode b = *(const SDL_DisplayMode *) B;

    if (a.w != b.w) {
        return b.w - a.w;
    }
    if (a.h != b.h) {
        return b.h - a.h;
    }
    if (SDL_BITSPERPIXEL(a.format) != SDL_BITSPERPIXEL(b.format)) {
        return SDL_BITSPERPIXEL(b.format) - SDL_BITSPERPIXEL(a.format);
    }
    if (a.refresh_rate != b.refresh_rate) {
        return b.refresh_rate - a.refresh_rate;
    }
    return 0;
}

int
SDL_GetNumVideoDrivers(void)
{
    return SDL_arraysize(bootstrap) - 1;
}

const char *
SDL_GetVideoDriver(int index)
{
    if (index >= 0 && index < SDL_GetNumVideoDrivers()) {
        return bootstrap[index]->name;
    }
    return NULL;
}

/*
 * Initialize the video and event subsystems -- determine native pixel format
 */
int
SDL_VideoInit(const char *driver_name, Uint32 flags)
{
    SDL_VideoDevice *video;
    int index;
    int i;

    /* Toggle the event thread flags, based on OS requirements */
#if defined(MUST_THREAD_EVENTS)
    flags |= SDL_INIT_EVENTTHREAD;
#elif defined(CANT_THREAD_EVENTS)
    if ((flags & SDL_INIT_EVENTTHREAD) == SDL_INIT_EVENTTHREAD) {
        SDL_SetError("OS doesn't support threaded events");
        return -1;
    }
#endif

    /* Start the event loop */
    if (SDL_StartEventLoop(flags) < 0) {
        return -1;
    }

    /* Check to make sure we don't overwrite '_this' */
    if (_this != NULL) {
        SDL_VideoQuit();
    }

    /* Select the proper video driver */
    index = 0;
    video = NULL;
    if (driver_name != NULL) {
        for (i = 0; bootstrap[i]; ++i) {
            if (SDL_strncmp(bootstrap[i]->name, driver_name,
                            SDL_strlen(bootstrap[i]->name)) == 0) {
                if (bootstrap[i]->available()) {
                    video = bootstrap[i]->create(index);
                }
                break;
            }
        }
    } else {
        for (i = 0; bootstrap[i]; ++i) {
            if (bootstrap[i]->available()) {
                video = bootstrap[i]->create(index);
                if (video != NULL) {
                    break;
                }
            }
        }
    }
    if (video == NULL) {
        if (driver_name) {
            SDL_SetError("%s not available", driver_name);
        } else {
            SDL_SetError("No available video device");
        }
        return -1;
    }
    _this = video;
    _this->name = bootstrap[i]->name;
    _this->next_object_id = 1;


    /* Set some very sane GL defaults */
    _this->gl_config.driver_loaded = 0;
    _this->gl_config.dll_handle = NULL;
    _this->gl_config.red_size = 3;
    _this->gl_config.green_size = 3;
    _this->gl_config.blue_size = 2;
    _this->gl_config.alpha_size = 0;
    _this->gl_config.buffer_size = 0;
    _this->gl_config.depth_size = 16;
    _this->gl_config.stencil_size = 0;
    _this->gl_config.double_buffer = 1;
    _this->gl_config.accum_red_size = 0;
    _this->gl_config.accum_green_size = 0;
    _this->gl_config.accum_blue_size = 0;
    _this->gl_config.accum_alpha_size = 0;
    _this->gl_config.stereo = 0;
    _this->gl_config.multisamplebuffers = 0;
    _this->gl_config.multisamplesamples = 0;
    _this->gl_config.accelerated = -1;  /* not known, don't set */

    /* Initialize the video subsystem */
    if (_this->VideoInit(_this) < 0) {
        SDL_VideoQuit();
        return -1;
    }

    /* Make sure some displays were added */
    if (_this->num_displays == 0) {
        SDL_SetError("The video driver did not add any displays");
        SDL_VideoQuit();
        return (-1);
    }

    /* The software renderer is always available */
    for (i = 0; i < _this->num_displays; ++i) {
        if (_this->displays[i].num_render_drivers > 0) {
            SDL_AddRenderDriver(i, &SDL_SW_RenderDriver);
        }
    }

    /* We're ready to go! */
    return 0;
}

const char *
SDL_GetCurrentVideoDriver()
{
    if (!_this) {
        return NULL;
    }
    return _this->name;
}

SDL_VideoDevice *
SDL_GetVideoDevice()
{
    return _this;
}

int
SDL_AddBasicVideoDisplay(const SDL_DisplayMode * desktop_mode)
{
    SDL_VideoDisplay display;

    SDL_zero(display);
    if (desktop_mode) {
        display.desktop_mode = *desktop_mode;
    }
    display.current_mode = display.desktop_mode;

    return SDL_AddVideoDisplay(&display);
}

int
SDL_AddVideoDisplay(const SDL_VideoDisplay * display)
{
    SDL_VideoDisplay *displays;
    int index = -1;

    displays =
        SDL_realloc(_this->displays,
                    (_this->num_displays + 1) * sizeof(*displays));
    if (displays) {
        index = _this->num_displays++;
        displays[index] = *display;
        displays[index].device = _this;
        _this->displays = displays;
    } else {
        SDL_OutOfMemory();
    }
    return index;
}

int
SDL_GetNumVideoDisplays(void)
{
    if (!_this) {
        return 0;
    }
    return _this->num_displays;
}

int
SDL_SelectVideoDisplay(int index)
{
    if (!_this) {
        SDL_SetError("Video subsystem has not been initialized");
        return (-1);
    }
    if (index >= 0) {
        if (index >= _this->num_displays) {
            SDL_SetError("index must be in the range 0 - %d",
                         _this->num_displays - 1);
            return -1;
        }
        _this->current_display = index;
    }
    return _this->current_display;
}

SDL_bool
SDL_AddDisplayMode(int displayIndex, const SDL_DisplayMode * mode)
{
    SDL_VideoDisplay *display = &_this->displays[displayIndex];
    SDL_DisplayMode *modes;
    int i, nmodes;

    /* Make sure we don't already have the mode in the list */
    modes = display->display_modes;
    nmodes = display->num_display_modes;
    for (i = nmodes; i--;) {
        if (SDL_memcmp(mode, &modes[i], sizeof(*mode)) == 0) {
            return SDL_FALSE;
        }
    }

    /* Go ahead and add the new mode */
    if (nmodes == display->max_display_modes) {
        modes =
            SDL_realloc(modes,
                        (display->max_display_modes + 32) * sizeof(*modes));
        if (!modes) {
            return SDL_FALSE;
        }
        display->display_modes = modes;
        display->max_display_modes += 32;
    }
    modes[nmodes] = *mode;
    display->num_display_modes++;

    return SDL_TRUE;
}

int
SDL_GetNumDisplayModes()
{
    if (_this) {
        SDL_VideoDisplay *display = &SDL_CurrentDisplay;
        if (!display->num_display_modes && _this->GetDisplayModes) {
            _this->GetDisplayModes(_this);
            SDL_qsort(display->display_modes, display->num_display_modes,
                      sizeof(SDL_DisplayMode), cmpmodes);
        }
        return display->num_display_modes;
    }
    return 0;
}

const SDL_DisplayMode *
SDL_GetDisplayMode(int index)
{
    if (index < 0 || index >= SDL_GetNumDisplayModes()) {
        SDL_SetError("index must be in the range of 0 - %d",
                     SDL_GetNumDisplayModes() - 1);
        return NULL;
    }
    return &SDL_CurrentDisplay.display_modes[index];
}

const SDL_DisplayMode *
SDL_GetDesktopDisplayMode(void)
{
    if (_this) {
        return &SDL_CurrentDisplay.desktop_mode;
    }
    return NULL;
}

const SDL_DisplayMode *
SDL_GetCurrentDisplayMode(void)
{
    if (_this) {
        return &SDL_CurrentDisplay.current_mode;
    }
    return NULL;
}

SDL_DisplayMode *
SDL_GetClosestDisplayMode(const SDL_DisplayMode * mode,
                          SDL_DisplayMode * closest)
{
    Uint32 target_format;
    int target_refresh_rate;
    int i;
    SDL_DisplayMode *current, *match;

    if (!_this || !mode || !closest) {
        return NULL;
    }

    /* Default to the desktop format */
    if (mode->format) {
        target_format = mode->format;
    } else {
        target_format = SDL_CurrentDisplay.desktop_mode.format;
    }

    /* Default to the desktop refresh rate */
    if (mode->refresh_rate) {
        target_refresh_rate = mode->refresh_rate;
    } else {
        target_refresh_rate = SDL_CurrentDisplay.desktop_mode.refresh_rate;
    }

    match = NULL;
    for (i = 0; i < SDL_GetNumDisplayModes(); ++i) {
        current = &SDL_CurrentDisplay.display_modes[i];

        if ((current->w && current->h) &&
            (current->w < mode->w || current->h < mode->h)) {
            /* Out of sorted modes large enough here */
            break;
        }
        if (!match || current->w < match->w || current->h < match->h) {
            match = current;
            continue;
        }
        if (current->format != match->format) {
            /* Sorted highest depth to lowest */
            if (current->format == target_format ||
                (SDL_BITSPERPIXEL(current->format) >=
                 SDL_BITSPERPIXEL(target_format)
                 && SDL_PIXELTYPE(current->format) ==
                 SDL_PIXELTYPE(target_format))) {
                match = current;
            }
            continue;
        }
        if (current->refresh_rate != match->refresh_rate) {
            /* Sorted highest refresh to lowest */
            if (current->refresh_rate >= target_refresh_rate) {
                match = current;
            }
        }
    }
    if (match) {
        if (match->format) {
            closest->format = match->format;
        } else {
            closest->format = mode->format;
        }
        if (match->w && match->h) {
            closest->w = match->w;
            closest->h = match->h;
        } else {
            closest->w = mode->w;
            closest->h = mode->h;
        }
        if (match->refresh_rate) {
            closest->refresh_rate = match->refresh_rate;
        } else {
            closest->refresh_rate = mode->refresh_rate;
        }
        closest->driverdata = match->driverdata;

        /* Pick some reasonable defaults if the app and driver don't care */
        if (!closest->format) {
            closest->format = SDL_PixelFormat_RGB888;
        }
        if (!closest->w) {
            closest->w = 640;
        }
        if (!closest->h) {
            closest->h = 480;
        }
        return closest;
    }
    return NULL;
}

int
SDL_SetDisplayMode(const SDL_DisplayMode * mode)
{
    SDL_VideoDisplay *display;
    SDL_DisplayMode display_mode;
    int i, ncolors;

    if (!_this) {
        SDL_SetError("Video subsystem has not been initialized");
        return -1;
    }

    if (!mode) {
        mode = SDL_GetDesktopDisplayMode();
    }
    display = &SDL_CurrentDisplay;
    display_mode = *mode;

    /* Default to the current mode */
    if (!display_mode.format) {
        display_mode.format = display->current_mode.format;
    }
    if (!display_mode.w) {
        display_mode.w = display->current_mode.w;
    }
    if (!display_mode.h) {
        display_mode.h = display->current_mode.h;
    }
    if (!display_mode.refresh_rate) {
        display_mode.refresh_rate = display->current_mode.refresh_rate;
    }

    /* Get a good video mode, the closest one possible */
    if (!SDL_GetClosestDisplayMode(&display_mode, &display_mode)) {
        SDL_SetError("No video mode large enough for %dx%d",
                     display_mode.w, display_mode.h);
        return -1;
    }

    /* See if there's anything left to do */
    if (SDL_memcmp
        (&display_mode, SDL_GetCurrentDisplayMode(),
         sizeof(display_mode)) == 0) {
        return 0;
    }

    /* Actually change the display mode */
    if (_this->SetDisplayMode(_this, &display_mode) < 0) {
        return -1;
    }
    display->current_mode = display_mode;

    /* Set up a palette, if necessary */
    if (SDL_ISPIXELFORMAT_INDEXED(display_mode.format)) {
        ncolors = (1 << SDL_BITSPERPIXEL(display_mode.format));
    } else {
        ncolors = 0;
    }
    if ((!ncolors && display->palette) || (ncolors && !display->palette)
        || (ncolors && ncolors != display->palette->ncolors)) {
        if (display->palette) {
            SDL_FreePalette(display->palette);
            display->palette = NULL;
        }
        if (ncolors) {
            display->palette = SDL_AllocPalette(ncolors);
            if (!display->palette) {
                return -1;
            }
            SDL_DitherColors(display->palette->colors,
                             SDL_BITSPERPIXEL(display_mode.format));
        }
    }

    /* Move any fullscreen windows into position */
    for (i = 0; i < display->num_windows; ++i) {
        SDL_Window *window = &display->windows[i];
        if (FULLSCREEN_VISIBLE(window)) {
            SDL_SetWindowPosition(window->id,
                                  ((display_mode.w - window->w) / 2),
                                  ((display_mode.h - window->h) / 2));
        }
    }

    return 0;
}

int
SDL_SetFullscreenDisplayMode(const SDL_DisplayMode * mode)
{
    SDL_VideoDisplay *display;
    int i;

    if (!_this) {
        SDL_SetError("Video subsystem has not been initialized");
        return -1;
    }

    display = &SDL_CurrentDisplay;
    if (mode) {
        SDL_GetClosestDisplayMode(mode, &display->desired_mode);
        display->fullscreen_mode = &display->desired_mode;
    } else {
        display->fullscreen_mode = NULL;
    }

    /* Actually set the mode if we have a fullscreen window visible */
    for (i = 0; i < display->num_windows; ++i) {
        SDL_Window *window = &display->windows[i];
        if (FULLSCREEN_VISIBLE(window)) {
            return SDL_SetDisplayMode(display->fullscreen_mode);
        }
    }
    return 0;
}

const SDL_DisplayMode *
SDL_GetFullscreenDisplayMode(void)
{
    if (_this) {
        return SDL_CurrentDisplay.fullscreen_mode;
    }
    return NULL;
}

int
SDL_SetDisplayPalette(const SDL_Color * colors, int firstcolor, int ncolors)
{
    SDL_Palette *palette;
    int status = 0;

    if (!_this) {
        SDL_SetError("Video subsystem has not been initialized");
        return -1;
    }
    palette = SDL_CurrentDisplay.palette;
    if (!palette) {
        SDL_SetError("Display mode does not have a palette");
        return -1;
    }

    status = SDL_SetPaletteColors(palette, colors, firstcolor, ncolors);

    if (_this->SetDisplayPalette) {
        if (_this->SetDisplayPalette(_this, palette) < 0) {
            status = -1;
        }
    }
    return status;
}

int
SDL_GetDisplayPalette(SDL_Color * colors, int firstcolor, int ncolors)
{
    SDL_Palette *palette;

    if (!_this) {
        SDL_SetError("Video subsystem has not been initialized");
        return -1;
    }

    palette = SDL_CurrentDisplay.palette;
    if (!palette->ncolors) {
        SDL_SetError("Display mode does not have a palette");
        return -1;
    }

    if (firstcolor < 0 || (firstcolor + ncolors) > palette->ncolors) {
        SDL_SetError("Palette indices are out of range");
        return -1;
    }

    SDL_memcpy(colors, &palette->colors[firstcolor],
               ncolors * sizeof(*colors));
    return 0;
}

SDL_WindowID
SDL_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags)
{
    const Uint32 allowed_flags = (SDL_WINDOW_FULLSCREEN |
                                  SDL_WINDOW_OPENGL |
                                  SDL_WINDOW_SHOWN |
                                  SDL_WINDOW_BORDERLESS |
                                  SDL_WINDOW_RESIZABLE |
                                  SDL_WINDOW_MAXIMIZED |
                                  SDL_WINDOW_MINIMIZED |
                                  SDL_WINDOW_INPUT_GRABBED);
    SDL_VideoDisplay *display;
    SDL_Window window;
    int num_windows;
    SDL_Window *windows;

    if (!_this) {
        SDL_SetError("Video subsystem has not been initialized");
        return 0;
    }

    SDL_zero(window);
    window.id = _this->next_object_id++;
    window.title = title ? SDL_strdup(title) : NULL;
    window.x = x;
    window.y = y;
    window.w = w;
    window.h = h;
    window.flags = (flags & allowed_flags);
    window.display = _this->current_display;

    if (_this->CreateWindow && _this->CreateWindow(_this, &window) < 0) {
        if (window.title) {
            SDL_free(window.title);
        }
        return 0;
    }

    display = &SDL_CurrentDisplay;
    num_windows = display->num_windows;
    windows =
        SDL_realloc(display->windows, (num_windows + 1) * sizeof(*windows));
    if (!windows) {
        if (_this->DestroyWindow) {
            _this->DestroyWindow(_this, &window);
        }
        if (window.title) {
            SDL_free(window.title);
        }
        return 0;
    }
    windows[num_windows] = window;
    display->windows = windows;
    display->num_windows++;

    if (FULLSCREEN_VISIBLE(&window)) {
        /* Hide any other fullscreen windows */
        int i;
        for (i = 0; i < display->num_windows; ++i) {
            SDL_Window *other = &display->windows[i];
            if (other->id != window.id && FULLSCREEN_VISIBLE(other)) {
                SDL_MinimizeWindow(other->id);
            }
        }
        SDL_SetDisplayMode(display->fullscreen_mode);
    }

    return window.id;
}

SDL_WindowID
SDL_CreateWindowFrom(const void *data)
{
    SDL_VideoDisplay *display;
    SDL_Window window;
    int num_windows;
    SDL_Window *windows;

    if (!_this) {
        SDL_SetError("Video subsystem has not been initialized");
        return (0);
    }

    SDL_zero(window);
    window.id = _this->next_object_id++;
    window.display = _this->current_display;

    if (!_this->CreateWindowFrom ||
        _this->CreateWindowFrom(_this, &window, data) < 0) {
        return 0;
    }

    display = &SDL_CurrentDisplay;
    num_windows = display->num_windows;
    windows =
        SDL_realloc(display->windows, (num_windows + 1) * sizeof(*windows));
    if (!windows) {
        if (_this->DestroyWindow) {
            _this->DestroyWindow(_this, &window);
        }
        if (window.title) {
            SDL_free(window.title);
        }
        return 0;
    }
    windows[num_windows] = window;
    display->windows = windows;
    display->num_windows++;

    return window.id;
}

SDL_Window *
SDL_GetWindowFromID(SDL_WindowID windowID)
{
    int i, j;

    if (!_this) {
        return NULL;
    }

    for (i = 0; i < _this->num_displays; ++i) {
        SDL_VideoDisplay *display = &_this->displays[i];
        for (j = 0; j < display->num_windows; ++j) {
            SDL_Window *window = &display->windows[j];
            if (window->id == windowID) {
                return window;
            }
        }
    }
    return NULL;
}

SDL_VideoDisplay *
SDL_GetDisplayFromWindow(SDL_Window * window)
{
    if (!_this) {
        return NULL;
    }
    return &_this->displays[window->display];
}

Uint32
SDL_GetWindowFlags(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return 0;
    }
    return window->flags;
}

void
SDL_SetWindowTitle(SDL_WindowID windowID, const char *title)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return;
    }
    if (window->title) {
        SDL_free(window->title);
    }
    window->title = SDL_strdup(title);

    if (_this->SetWindowTitle) {
        _this->SetWindowTitle(_this, window);
    }
}

const char *
SDL_GetWindowTitle(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return NULL;
    }
    return window->title;
}

void
SDL_SetWindowData(SDL_WindowID windowID, void *userdata)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return;
    }
    window->userdata = userdata;
}

void *
SDL_GetWindowData(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return NULL;
    }
    return window->userdata;
}

void
SDL_SetWindowPosition(SDL_WindowID windowID, int x, int y)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return;
    }

    if (x != SDL_WINDOWPOS_UNDEFINED) {
        window->x = x;
    }
    if (y != SDL_WINDOWPOS_UNDEFINED) {
        window->y = y;
    }

    if (_this->SetWindowPosition) {
        _this->SetWindowPosition(_this, window);
    }
}

void
SDL_GetWindowPosition(SDL_WindowID windowID, int *x, int *y)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return;
    }
    if (x) {
        *x = window->x;
    }
    if (y) {
        *y = window->y;
    }
}

void
SDL_SetWindowSize(SDL_WindowID windowID, int w, int h)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return;
    }

    window->w = w;
    window->h = h;

    if (_this->SetWindowSize) {
        _this->SetWindowSize(_this, window);
    }
}

void
SDL_GetWindowSize(SDL_WindowID windowID, int *w, int *h)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return;
    }
    if (w) {
        *w = window->w;
    }
    if (h) {
        *h = window->h;
    }
}

void
SDL_ShowWindow(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window || (window->flags & SDL_WINDOW_SHOWN)) {
        return;
    }

    SDL_SendWindowEvent(window->id, SDL_WINDOWEVENT_SHOWN, 0, 0);

    if (_this->ShowWindow) {
        _this->ShowWindow(_this, window);
    }
}

void
SDL_HideWindow(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window || !(window->flags & SDL_WINDOW_SHOWN)) {
        return;
    }

    SDL_SendWindowEvent(window->id, SDL_WINDOWEVENT_HIDDEN, 0, 0);

    if (_this->HideWindow) {
        _this->HideWindow(_this, window);
    }
}

void
SDL_RaiseWindow(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return;
    }

    if (_this->RaiseWindow) {
        _this->RaiseWindow(_this, window);
    }
}

void
SDL_MaximizeWindow(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window || (window->flags & SDL_WINDOW_MAXIMIZED)) {
        return;
    }

    SDL_SendWindowEvent(window->id, SDL_WINDOWEVENT_MAXIMIZED, 0, 0);

    if (_this->MaximizeWindow) {
        _this->MaximizeWindow(_this, window);
    }
}

void
SDL_MinimizeWindow(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window || (window->flags & SDL_WINDOW_MINIMIZED)) {
        return;
    }

    SDL_SendWindowEvent(window->id, SDL_WINDOWEVENT_MINIMIZED, 0, 0);

    if (_this->MinimizeWindow) {
        _this->MinimizeWindow(_this, window);
    }
}

void
SDL_RestoreWindow(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window
        || (window->flags & (SDL_WINDOW_MAXIMIZED | SDL_WINDOW_MINIMIZED))) {
        return;
    }

    SDL_SendWindowEvent(window->id, SDL_WINDOWEVENT_RESTORED, 0, 0);

    if (_this->RestoreWindow) {
        _this->RestoreWindow(_this, window);
    }
}

int
SDL_SetWindowFullscreen(SDL_WindowID windowID, int fullscreen)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return -1;
    }

    if (fullscreen) {
        fullscreen = SDL_WINDOW_FULLSCREEN;
    }
    if ((window->flags & SDL_WINDOW_FULLSCREEN) == fullscreen) {
        return 0;
    }

    if (fullscreen) {
        window->flags |= SDL_WINDOW_FULLSCREEN;

        if (FULLSCREEN_VISIBLE(window)) {
            SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);

            /* Hide any other fullscreen windows */
            int i;
            for (i = 0; i < display->num_windows; ++i) {
                SDL_Window *other = &display->windows[i];
                if (other->id != windowID && FULLSCREEN_VISIBLE(other)) {
                    SDL_MinimizeWindow(other->id);
                }
            }

            SDL_SetDisplayMode(display->fullscreen_mode);
        }
    } else {
        window->flags &= ~SDL_WINDOW_FULLSCREEN;

        if (FULLSCREEN_VISIBLE(window)) {
            SDL_SetDisplayMode(NULL);
        }
    }
    return 0;
}

void
SDL_SetWindowGrab(SDL_WindowID windowID, int mode)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window || (!!mode == !!(window->flags & SDL_WINDOW_INPUT_GRABBED))) {
        return;
    }

    if (mode) {
        window->flags |= SDL_WINDOW_INPUT_GRABBED;
    } else {
        window->flags &= ~SDL_WINDOW_INPUT_GRABBED;
    }

    if ((window->flags & SDL_WINDOW_INPUT_FOCUS) && _this->SetWindowGrab) {
        _this->SetWindowGrab(_this, window);
    }
}

int
SDL_GetWindowGrab(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return 0;
    }

    return ((window->flags & SDL_WINDOW_INPUT_GRABBED) != 0);
}

void
SDL_OnWindowShown(SDL_Window * window)
{
}

void
SDL_OnWindowHidden(SDL_Window * window)
{
}

void
SDL_OnWindowFocusGained(SDL_Window * window)
{
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        SDL_SetDisplayMode(display->fullscreen_mode);
    }
    if (display->gamma && _this->SetDisplayGammaRamp) {
        _this->SetDisplayGammaRamp(_this, display->gamma);
    }
    if ((window->flags & SDL_WINDOW_INPUT_GRABBED) && _this->SetWindowGrab) {
        _this->SetWindowGrab(_this, window);
    }
}

void
SDL_OnWindowFocusLost(SDL_Window * window)
{
    SDL_VideoDisplay *display = SDL_GetDisplayFromWindow(window);

    if (window->flags & SDL_WINDOW_FULLSCREEN) {
        SDL_MinimizeWindow(window->id);
        SDL_SetDisplayMode(NULL);
    }
    if (display->gamma && _this->SetDisplayGammaRamp) {
        _this->SetDisplayGammaRamp(_this, display->saved_gamma);
    }
    if ((window->flags & SDL_WINDOW_INPUT_GRABBED) && _this->SetWindowGrab) {
        _this->SetWindowGrab(_this, window);
    }
}

SDL_WindowID
SDL_GetFocusWindow(void)
{
    SDL_VideoDisplay *display;
    int i;

    if (!_this) {
        return 0;
    }

    display = &SDL_CurrentDisplay;
    for (i = 0; i < display->num_windows; ++i) {
        SDL_Window *window = &display->windows[i];

        if (window->flags & SDL_WINDOW_INPUT_FOCUS) {
            return window->id;
        }
    }
    return 0;
}

void
SDL_DestroyWindow(SDL_WindowID windowID)
{
    int i, j;

    if (!_this) {
        return;
    }

    /* Restore video mode, etc. */
    SDL_SendWindowEvent(windowID, SDL_WINDOWEVENT_FOCUS_LOST, 0, 0);

    for (i = 0; i < _this->num_displays; ++i) {
        SDL_VideoDisplay *display = &_this->displays[i];
        for (j = 0; j < display->num_windows; ++j) {
            SDL_Window *window = &display->windows[j];
            if (window->id != windowID) {
                continue;
            }
            if (window->renderer) {
                SDL_DestroyRenderer(window->id);
            }
            if (_this->DestroyWindow) {
                _this->DestroyWindow(_this, window);
            }
            if (window->title) {
                SDL_free(window->title);
            }
            if (j != display->num_windows - 1) {
                SDL_memcpy(&display->windows[i],
                           &display->windows[i + 1],
                           (display->num_windows - i - 1) * sizeof(*window));
            }
            --display->num_windows;
            return;
        }
    }
}

void
SDL_AddRenderDriver(int displayIndex, const SDL_RenderDriver * driver)
{
    SDL_VideoDisplay *display = &_this->displays[displayIndex];
    SDL_RenderDriver *render_drivers;

    render_drivers =
        SDL_realloc(display->render_drivers,
                    (display->num_render_drivers +
                     1) * sizeof(*render_drivers));
    if (render_drivers) {
        render_drivers[display->num_render_drivers] = *driver;
        display->render_drivers = render_drivers;
        display->num_render_drivers++;
    }
}

int
SDL_GetNumRenderers(void)
{
    if (_this) {
        return SDL_CurrentDisplay.num_render_drivers;
    }
    return 0;
}

int
SDL_GetRendererInfo(int index, SDL_RendererInfo * info)
{
    if (index < 0 || index >= SDL_GetNumRenderers()) {
        SDL_SetError("index must be in the range of 0 - %d",
                     SDL_GetNumRenderers() - 1);
        return -1;
    }
    *info = SDL_CurrentDisplay.render_drivers[index].info;
    return 0;
}

int
SDL_CreateRenderer(SDL_WindowID windowID, int index, Uint32 flags)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window) {
        return 0;
    }

    if (index < 0) {
        int n = SDL_GetNumRenderers();
        for (index = 0; index < n; ++index) {
            SDL_RenderDriver *driver =
                &SDL_CurrentDisplay.render_drivers[index];

            /* Skip minimal drivers in automatic scans */
            if (!(flags & SDL_Renderer_Minimal)
                && (driver->info.flags & SDL_Renderer_Minimal)) {
                continue;
            }
            if ((driver->info.flags & flags) == flags) {
                break;
            }
        }
        if (index == n) {
            SDL_SetError("Couldn't find matching render driver");
            return -1;
        }
    }

    if (index >= SDL_GetNumRenderers()) {
        SDL_SetError("index must be -1 or in the range of 0 - %d",
                     SDL_GetNumRenderers() - 1);
        return -1;
    }

    /* Free any existing renderer */
    SDL_DestroyRenderer(windowID);

    /* Create a new renderer instance */
    window->renderer =
        SDL_CurrentDisplay.render_drivers[index].CreateRenderer(window,
                                                                flags);
    if (!window->renderer) {
        return -1;
    }
    SDL_CurrentDisplay.current_renderer = window->renderer;

    return 0;
}

int
SDL_SelectRenderer(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window || !window->renderer) {
        return -1;
    }
    SDL_CurrentDisplay.current_renderer = window->renderer;
    return 0;
}

SDL_TextureID
SDL_CreateTexture(Uint32 format, int access, int w, int h)
{
    int hash;
    SDL_Renderer *renderer;
    SDL_Texture *texture;

    if (!_this) {
        return 0;
    }

    renderer = SDL_CurrentDisplay.current_renderer;
    if (!renderer || !renderer->CreateTexture) {
        return 0;
    }

    texture = (SDL_Texture *) SDL_malloc(sizeof(*texture));
    if (!texture) {
        SDL_OutOfMemory();
        return 0;
    }

    SDL_zerop(texture);
    texture->id = _this->next_object_id++;
    texture->format = format;
    texture->access = access;
    texture->w = w;
    texture->h = h;
    texture->renderer = renderer;

    if (renderer->CreateTexture(renderer, texture) < 0) {
        SDL_free(texture);
        return 0;
    }

    hash = (texture->id % SDL_arraysize(SDL_CurrentDisplay.textures));
    texture->next = SDL_CurrentDisplay.textures[hash];
    SDL_CurrentDisplay.textures[hash] = texture;

    return texture->id;
}

SDL_TextureID
SDL_CreateTextureFromSurface(Uint32 format, int access, SDL_Surface * surface)
{
    SDL_TextureID textureID;
    Uint32 surface_flags = surface->flags;
    SDL_PixelFormat *fmt = surface->format;
    Uint8 alpha;
    SDL_Rect bounds;
    SDL_Surface dst;
    int bpp;
    Uint32 Rmask, Gmask, Bmask, Amask;

    if (!surface) {
        SDL_SetError("SDL_CreateTextureFromSurface() passed NULL surface");
        return 0;
    }

    if (format) {
        if (!SDL_PixelFormatEnumToMasks
            (format, &bpp, &Rmask, &Gmask, &Bmask, &Amask)) {
            SDL_SetError("Unknown pixel format");
            return 0;
        }
    } else {
        if (fmt->Amask || !(surface_flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA))) {
            bpp = fmt->BitsPerPixel;
            Rmask = fmt->Rmask;
            Gmask = fmt->Gmask;
            Bmask = fmt->Bmask;
            Amask = fmt->Amask;
        } else {
            /* Need a format with alpha */
            bpp = 32;
            Rmask = 0x00FF0000;
            Gmask = 0x0000FF00;
            Bmask = 0x000000FF;
            Amask = 0xFF000000;
        }
        format = SDL_MasksToPixelFormatEnum(bpp, Rmask, Gmask, Bmask, Amask);
        if (!format) {
            SDL_SetError("Unknown pixel format");
            return 0;
        }
    }

    textureID = SDL_CreateTexture(format, access, surface->w, surface->h);
    if (!textureID) {
        return 0;
    }

    /* Set up a destination surface for the texture update */
    SDL_zero(dst);
    dst.format = SDL_AllocFormat(bpp, Rmask, Gmask, Bmask, Amask);
    if (!dst.format) {
        SDL_DestroyTexture(textureID);
        return 0;
    }
    dst.w = surface->w;
    dst.h = surface->h;
    if (SDL_LockTexture(textureID, NULL, 1, &dst.pixels, &dst.pitch) == 0) {
        dst.flags |= SDL_PREALLOC;
    } else {
        dst.pitch = SDL_CalculatePitch(&dst);
        dst.pixels = SDL_malloc(dst.h * dst.pitch);
        if (!dst.pixels) {
            SDL_DestroyTexture(textureID);
            SDL_FreeFormat(dst.format);
            SDL_OutOfMemory();
            return 0;
        }
    }

    /* Copy the palette if any */
    if (SDL_ISPIXELFORMAT_INDEXED(format)) {
        if (fmt->palette) {
            SDL_SetTexturePalette(textureID, fmt->palette->colors, 0,
                                  fmt->palette->ncolors);
            SDL_SetSurfacePalette(&dst, fmt->palette);
        } else {
            dst.format->palette =
                SDL_AllocPalette((1 << SDL_BITSPERPIXEL(format)));
            if (!dst.format->palette) {
                SDL_DestroyTexture(textureID);
                SDL_FreeFormat(dst.format);
                return 0;
            }
            SDL_DitherColors(dst.format->palette->colors,
                             SDL_BITSPERPIXEL(format));
        }
    }

    /* Make the texture transparent if the surface has colorkey */
    if (surface_flags & SDL_SRCCOLORKEY) {
        int row;
        int length = dst.w * dst.format->BytesPerPixel;
        Uint8 *p = (Uint8 *) dst.pixels;
        for (row = 0; row < dst.h; ++row) {
            SDL_memset(p, 0, length);
            p += dst.pitch;
        }
    }

    /* Copy over the alpha channel */
    if (surface_flags & SDL_SRCALPHA) {
        if (fmt->Amask) {
            surface->flags &= ~SDL_SRCALPHA;
        } else {
            /* FIXME: Need to make sure the texture has an alpha channel
             *        and copy 'alpha' into the texture alpha channel.
             */
            alpha = surface->format->alpha;
            SDL_SetAlpha(surface, 0, 0);
        }
    }

    /* Copy over the image data */
    bounds.x = 0;
    bounds.y = 0;
    bounds.w = surface->w;
    bounds.h = surface->h;
    SDL_LowerBlit(surface, &bounds, &dst, &bounds);

    /* Clean up the original surface */
    if ((surface_flags & SDL_SRCALPHA) == SDL_SRCALPHA) {
        Uint32 aflags = surface_flags & (SDL_SRCALPHA | SDL_RLEACCELOK);
        if (fmt->Amask) {
            surface->flags |= SDL_SRCALPHA;
        } else {
            SDL_SetAlpha(surface, aflags, alpha);
        }
    }

    /* Update the texture */
    if (dst.flags & SDL_PREALLOC) {
        SDL_UnlockTexture(textureID);
    } else {
        SDL_UpdateTexture(textureID, NULL, dst.pixels, dst.pitch);
        SDL_free(dst.pixels);
    }
    SDL_FreeFormat(dst.format);

    return textureID;
}

static __inline__ SDL_Texture *
SDL_GetTextureFromID(SDL_TextureID textureID)
{
    int hash;
    SDL_Texture *texture;

    if (!_this) {
        return NULL;
    }

    hash = (textureID % SDL_arraysize(SDL_CurrentDisplay.textures));
    for (texture = SDL_CurrentDisplay.textures[hash]; texture;
         texture = texture->next) {
        if (texture->id == textureID) {
            return texture;
        }
    }
    return NULL;
}

int
SDL_QueryTexture(SDL_TextureID textureID, Uint32 * format, int *access,
                 int *w, int *h)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);

    if (!texture) {
        return -1;
    }

    if (format) {
        *format = texture->format;
    }
    if (access) {
        *access = texture->access;
    }
    if (w) {
        *w = texture->w;
    }
    if (h) {
        *h = texture->h;
    }
    return 0;
}

int
SDL_QueryTexturePixels(SDL_TextureID textureID, void **pixels, int *pitch)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;

    if (!texture) {
        return -1;
    }

    renderer = texture->renderer;
    if (!renderer->QueryTexturePixels) {
        return -1;
    }
    return renderer->QueryTexturePixels(renderer, texture, pixels, pitch);
}

int
SDL_SetTexturePalette(SDL_TextureID textureID, const SDL_Color * colors,
                      int firstcolor, int ncolors)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;

    if (!texture) {
        return -1;
    }

    renderer = texture->renderer;
    if (!renderer->SetTexturePalette) {
        return -1;
    }
    return renderer->SetTexturePalette(renderer, texture, colors, firstcolor,
                                       ncolors);
}

int
SDL_GetTexturePalette(SDL_TextureID textureID, SDL_Color * colors,
                      int firstcolor, int ncolors)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;

    if (!texture) {
        return -1;
    }

    renderer = texture->renderer;
    if (!renderer->GetTexturePalette) {
        return -1;
    }
    return renderer->GetTexturePalette(renderer, texture, colors, firstcolor,
                                       ncolors);
}

int
SDL_UpdateTexture(SDL_TextureID textureID, const SDL_Rect * rect,
                  const void *pixels, int pitch)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;
    SDL_Rect full_rect;

    if (!texture) {
        return -1;
    }

    renderer = texture->renderer;
    if (!renderer->UpdateTexture) {
        return -1;
    }

    if (!rect) {
        full_rect.x = 0;
        full_rect.y = 0;
        full_rect.w = texture->w;
        full_rect.h = texture->h;
        rect = &full_rect;
    }

    return renderer->UpdateTexture(renderer, texture, rect, pixels, pitch);
}

int
SDL_LockTexture(SDL_TextureID textureID, const SDL_Rect * rect, int markDirty,
                void **pixels, int *pitch)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;
    SDL_Rect full_rect;

    if (!texture) {
        return -1;
    }

    renderer = texture->renderer;
    if (!renderer->LockTexture) {
        return -1;
    }

    if (!rect) {
        full_rect.x = 0;
        full_rect.y = 0;
        full_rect.w = texture->w;
        full_rect.h = texture->h;
        rect = &full_rect;
    }

    return renderer->LockTexture(renderer, texture, rect, markDirty, pixels,
                                 pitch);
}

void
SDL_UnlockTexture(SDL_TextureID textureID)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;

    if (!texture) {
        return;
    }

    renderer = texture->renderer;
    if (!renderer->UnlockTexture) {
        return;
    }
    renderer->UnlockTexture(renderer, texture);
}

void
SDL_DirtyTexture(SDL_TextureID textureID, int numrects,
                 const SDL_Rect * rects)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;

    if (!texture) {
        return;
    }

    renderer = texture->renderer;
    if (!renderer->DirtyTexture) {
        return;
    }
    renderer->DirtyTexture(renderer, texture, numrects, rects);
}

void
SDL_SelectRenderTexture(SDL_TextureID textureID)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;

    if (!texture || texture->access != SDL_TextureAccess_Render) {
        return;
    }
    renderer = texture->renderer;
    if (!renderer->SelectRenderTexture) {
        return;
    }
    renderer->SelectRenderTexture(renderer, texture);
}

int
SDL_RenderFill(const SDL_Rect * rect, Uint32 color)
{
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Rect real_rect;

    if (!_this) {
        return -1;
    }

    renderer = SDL_CurrentDisplay.current_renderer;
    if (!renderer || !renderer->RenderFill) {
        return -1;
    }

    window = SDL_GetWindowFromID(renderer->window);
    real_rect.x = 0;
    real_rect.y = 0;
    real_rect.w = window->w;
    real_rect.h = window->h;
    if (rect) {
        if (!SDL_IntersectRect(rect, &real_rect, &real_rect)) {
            return 0;
        }
    }
    rect = &real_rect;

    return renderer->RenderFill(renderer, rect, color);
}

int
SDL_RenderCopy(SDL_TextureID textureID, const SDL_Rect * srcrect,
               const SDL_Rect * dstrect, int blendMode, int scaleMode)
{
    SDL_Texture *texture = SDL_GetTextureFromID(textureID);
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Rect real_srcrect;
    SDL_Rect real_dstrect;

    if (!texture || texture->renderer != SDL_CurrentDisplay.current_renderer) {
        return -1;
    }

    renderer = SDL_CurrentDisplay.current_renderer;
    if (!renderer || !renderer->RenderCopy) {
        return -1;
    }

    /* FIXME: implement clipping */
    window = SDL_GetWindowFromID(renderer->window);
    real_srcrect.x = 0;
    real_srcrect.y = 0;
    real_srcrect.w = texture->w;
    real_srcrect.h = texture->h;
    real_dstrect.x = 0;
    real_dstrect.y = 0;
    real_dstrect.w = window->w;
    real_dstrect.h = window->h;
    if (!srcrect) {
        srcrect = &real_srcrect;
    }
    if (!dstrect) {
        dstrect = &real_dstrect;
    }

    return renderer->RenderCopy(renderer, texture, srcrect, dstrect,
                                blendMode, scaleMode);
}

int
SDL_RenderReadPixels(const SDL_Rect * rect, void *pixels, int pitch)
{
    SDL_Renderer *renderer;
    SDL_Rect full_rect;

    if (!_this) {
        return -1;
    }

    renderer = SDL_CurrentDisplay.current_renderer;
    if (!renderer || !renderer->RenderReadPixels) {
        return -1;
    }

    if (!rect) {
        SDL_Window *window = SDL_GetWindowFromID(renderer->window);
        full_rect.x = 0;
        full_rect.y = 0;
        full_rect.w = window->w;
        full_rect.h = window->h;
        rect = &full_rect;
    }

    return renderer->RenderReadPixels(renderer, rect, pixels, pitch);
}

int
SDL_RenderWritePixels(const SDL_Rect * rect, const void *pixels, int pitch)
{
    SDL_Renderer *renderer;
    SDL_Rect full_rect;

    if (!_this) {
        return -1;
    }

    renderer = SDL_CurrentDisplay.current_renderer;
    if (!renderer || !renderer->RenderWritePixels) {
        return -1;
    }

    if (!rect) {
        SDL_Window *window = SDL_GetWindowFromID(renderer->window);
        full_rect.x = 0;
        full_rect.y = 0;
        full_rect.w = window->w;
        full_rect.h = window->h;
        rect = &full_rect;
    }

    return renderer->RenderWritePixels(renderer, rect, pixels, pitch);
}

void
SDL_RenderPresent(void)
{
    SDL_Renderer *renderer;

    if (!_this) {
        return;
    }

    renderer = SDL_CurrentDisplay.current_renderer;
    if (!renderer || !renderer->RenderPresent) {
        return;
    }

    renderer->RenderPresent(renderer);
}

void
SDL_DestroyTexture(SDL_TextureID textureID)
{
    int hash;
    SDL_Texture *prev, *texture;
    SDL_Renderer *renderer;

    if (!_this) {
        return;
    }

    /* Look up the texture in the hash table */
    hash = (textureID % SDL_arraysize(SDL_CurrentDisplay.textures));
    prev = NULL;
    for (texture = SDL_CurrentDisplay.textures[hash]; texture;
         prev = texture, texture = texture->next) {
        if (texture->id == textureID) {
            break;
        }
    }
    if (!texture) {
        return;
    }

    /* Unlink the texture from the list */
    if (prev) {
        prev->next = texture->next;
    } else {
        SDL_CurrentDisplay.textures[hash] = texture->next;
    }

    /* Free the texture */
    renderer = texture->renderer;
    renderer->DestroyTexture(renderer, texture);
    SDL_free(texture);
}

void
SDL_DestroyRenderer(SDL_WindowID windowID)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);
    SDL_Renderer *renderer;
    int i;

    if (!window) {
        return;
    }

    renderer = window->renderer;
    if (!renderer) {
        return;
    }

    /* Free existing textures for this renderer */
    for (i = 0; i < SDL_arraysize(SDL_CurrentDisplay.textures); ++i) {
        SDL_Texture *texture;
        SDL_Texture *prev = NULL;
        SDL_Texture *next;
        for (texture = SDL_CurrentDisplay.textures[i]; texture;
             texture = next) {
            next = texture->next;
            if (texture->renderer == renderer) {
                if (prev) {
                    prev->next = next;
                } else {
                    SDL_CurrentDisplay.textures[i] = next;
                }
                renderer->DestroyTexture(renderer, texture);
                SDL_free(texture);
            } else {
                prev = texture;
            }
        }
    }

    /* Free the renderer instance */
    renderer->DestroyRenderer(renderer);

    /* Clear references */
    window->renderer = NULL;
    if (SDL_CurrentDisplay.current_renderer == renderer) {
        SDL_CurrentDisplay.current_renderer = NULL;
    }
}

void
SDL_VideoQuit(void)
{
    int i, j;

    if (!_this) {
        return;
    }

    /* Halt event processing before doing anything else */
    SDL_StopEventLoop();

    /* Clean up the system video */
    for (i = _this->num_displays; i--;) {
        SDL_VideoDisplay *display = &_this->displays[i];
        for (j = display->num_windows; j--;) {
            SDL_DestroyWindow(display->windows[i].id);
        }
        if (display->windows) {
            SDL_free(display->windows);
            display->windows = NULL;
        }
        display->num_windows = 0;
    }
    _this->VideoQuit(_this);

    for (i = _this->num_displays; i--;) {
        SDL_VideoDisplay *display = &_this->displays[i];
        for (j = display->num_display_modes; j--;) {
            if (display->display_modes[j].driverdata) {
                SDL_free(display->display_modes[j].driverdata);
                display->display_modes[j].driverdata = NULL;
            }
        }
        if (display->display_modes) {
            SDL_free(display->display_modes);
            display->display_modes = NULL;
        }
        if (display->desktop_mode.driverdata) {
            SDL_free(display->desktop_mode.driverdata);
            display->desktop_mode.driverdata = NULL;
        }
        if (display->palette) {
            SDL_FreePalette(display->palette);
            display->palette = NULL;
        }
        if (display->gamma) {
            SDL_free(display->gamma);
        }
        if (display->driverdata) {
            SDL_free(display->driverdata);
        }
    }
    if (_this->displays) {
        SDL_free(_this->displays);
        _this->displays = NULL;
    }
    _this->free(_this);
    _this = NULL;
}

/* Load the GL driver library */
int
SDL_GL_LoadLibrary(const char *path)
{
    int retval;

    retval = -1;
    if (_this == NULL) {
        SDL_SetError("Video subsystem has not been initialized");
    } else {
        if (_this->GL_LoadLibrary) {
            retval = _this->GL_LoadLibrary(_this, path);
        } else {
            SDL_SetError("No dynamic GL support in video driver");
        }
    }
    return (retval);
}

void *
SDL_GL_GetProcAddress(const char *proc)
{
    void *func;

    func = NULL;
    if (_this->GL_GetProcAddress) {
        if (_this->gl_config.driver_loaded) {
            func = _this->GL_GetProcAddress(_this, proc);
        } else {
            SDL_SetError("No GL driver has been loaded");
        }
    } else {
        SDL_SetError("No dynamic GL support in video driver");
    }
    return func;
}

/* Set the specified GL attribute for setting up a GL video mode */
int
SDL_GL_SetAttribute(SDL_GLattr attr, int value)
{
    int retval;

    retval = 0;
    switch (attr) {
    case SDL_GL_RED_SIZE:
        _this->gl_config.red_size = value;
        break;
    case SDL_GL_GREEN_SIZE:
        _this->gl_config.green_size = value;
        break;
    case SDL_GL_BLUE_SIZE:
        _this->gl_config.blue_size = value;
        break;
    case SDL_GL_ALPHA_SIZE:
        _this->gl_config.alpha_size = value;
        break;
    case SDL_GL_DOUBLEBUFFER:
        _this->gl_config.double_buffer = value;
        break;
    case SDL_GL_BUFFER_SIZE:
        _this->gl_config.buffer_size = value;
        break;
    case SDL_GL_DEPTH_SIZE:
        _this->gl_config.depth_size = value;
        break;
    case SDL_GL_STENCIL_SIZE:
        _this->gl_config.stencil_size = value;
        break;
    case SDL_GL_ACCUM_RED_SIZE:
        _this->gl_config.accum_red_size = value;
        break;
    case SDL_GL_ACCUM_GREEN_SIZE:
        _this->gl_config.accum_green_size = value;
        break;
    case SDL_GL_ACCUM_BLUE_SIZE:
        _this->gl_config.accum_blue_size = value;
        break;
    case SDL_GL_ACCUM_ALPHA_SIZE:
        _this->gl_config.accum_alpha_size = value;
        break;
    case SDL_GL_STEREO:
        _this->gl_config.stereo = value;
        break;
    case SDL_GL_MULTISAMPLEBUFFERS:
        _this->gl_config.multisamplebuffers = value;
        break;
    case SDL_GL_MULTISAMPLESAMPLES:
        _this->gl_config.multisamplesamples = value;
        break;
    case SDL_GL_ACCELERATED_VISUAL:
        _this->gl_config.accelerated = value;
        break;
    default:
        SDL_SetError("Unknown OpenGL attribute");
        retval = -1;
        break;
    }
    return (retval);
}

/* Retrieve an attribute value from the windowing system. */
int
SDL_GL_GetAttribute(SDL_GLattr attr, int *value)
{
    int retval = -1;

    if (_this->GL_GetAttribute) {
        retval = _this->GL_GetAttribute(_this, attr, value);
    } else {
        *value = 0;
        SDL_SetError("GL_GetAttribute not supported");
    }
    return retval;
}

/* Perform a GL buffer swap on the current GL context */
void
SDL_GL_SwapBuffers(void)
{
    // FIXME: Track the current window context - do we provide N contexts, and match them to M windows, or is there a one-to-one mapping?
    _this->GL_SwapBuffers(_this);
}

#if 0                           // FIXME
/* Utility function used by SDL_WM_SetIcon();
 * flags & 1 for color key, flags & 2 for alpha channel. */
static void
CreateMaskFromColorKeyOrAlpha(SDL_Surface * icon, Uint8 * mask, int flags)
{
    int x, y;
    Uint32 colorkey;
#define SET_MASKBIT(icon, x, y, mask) \
	mask[(y*((icon->w+7)/8))+(x/8)] &= ~(0x01<<(7-(x%8)))

    colorkey = icon->format->colorkey;
    switch (icon->format->BytesPerPixel) {
    case 1:
        {
            Uint8 *pixels;
            for (y = 0; y < icon->h; ++y) {
                pixels = (Uint8 *) icon->pixels + y * icon->pitch;
                for (x = 0; x < icon->w; ++x) {
                    if (*pixels++ == colorkey) {
                        SET_MASKBIT(icon, x, y, mask);
                    }
                }
            }
        }
        break;

    case 2:
        {
            Uint16 *pixels;
            for (y = 0; y < icon->h; ++y) {
                pixels = (Uint16 *) icon->pixels + y * icon->pitch / 2;
                for (x = 0; x < icon->w; ++x) {
                    if ((flags & 1) && *pixels == colorkey) {
                        SET_MASKBIT(icon, x, y, mask);
                    } else if ((flags & 2)
                               && (*pixels & icon->format->Amask) == 0) {
                        SET_MASKBIT(icon, x, y, mask);
                    }
                    pixels++;
                }
            }
        }
        break;

    case 4:
        {
            Uint32 *pixels;
            for (y = 0; y < icon->h; ++y) {
                pixels = (Uint32 *) icon->pixels + y * icon->pitch / 4;
                for (x = 0; x < icon->w; ++x) {
                    if ((flags & 1) && *pixels == colorkey) {
                        SET_MASKBIT(icon, x, y, mask);
                    } else if ((flags & 2)
                               && (*pixels & icon->format->Amask) == 0) {
                        SET_MASKBIT(icon, x, y, mask);
                    }
                    pixels++;
                }
            }
        }
        break;
    }
}

/*
 * Sets the window manager icon for the display window.
 */
void
SDL_WM_SetIcon(SDL_Surface * icon, Uint8 * mask)
{
    if (icon && _this->SetIcon) {
        /* Generate a mask if necessary, and create the icon! */
        if (mask == NULL) {
            int mask_len = icon->h * (icon->w + 7) / 8;
            int flags = 0;
            mask = (Uint8 *) SDL_malloc(mask_len);
            if (mask == NULL) {
                return;
            }
            SDL_memset(mask, ~0, mask_len);
            if (icon->flags & SDL_SRCCOLORKEY)
                flags |= 1;
            if (icon->flags & SDL_SRCALPHA)
                flags |= 2;
            if (flags) {
                CreateMaskFromColorKeyOrAlpha(icon, mask, flags);
            }
            _this->SetIcon(_this, icon, mask);
            SDL_free(mask);
        } else {
            _this->SetIcon(_this, icon, mask);
        }
    }
}
#endif

SDL_bool
SDL_GetWindowWMInfo(SDL_WindowID windowID, struct SDL_SysWMinfo *info)
{
    SDL_Window *window = SDL_GetWindowFromID(windowID);

    if (!window || !_this->GetWindowWMInfo) {
        return SDL_FALSE;
    }
    return (_this->GetWindowWMInfo(_this, window, info));
}

/* vi: set ts=4 sw=4 expandtab: */
