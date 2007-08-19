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

#ifndef _SDL_sysvideo_h
#define _SDL_sysvideo_h

#include "SDL_mouse.h"
#include "SDL_keysym.h"

/* The SDL video driver */

typedef struct SDL_Window SDL_Window;
typedef struct SDL_Texture SDL_Texture;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_RenderDriver SDL_RenderDriver;
typedef struct SDL_VideoDisplay SDL_VideoDisplay;
typedef struct SDL_VideoDevice SDL_VideoDevice;

/* Define the SDL texture structure */
struct SDL_Texture
{
    Uint32 id;

    Uint32 format;      /**< The pixel format of the texture */
    int access;         /**< SDL_TextureAccess */
    int w;              /**< The width of the texture */
    int h;              /**< The height of the texture */
    int modMode;        /**< The texture modulation mode */
    int blendMode;      /**< The texture blend mode */
    int scaleMode;      /**< The texture scale mode */
    Uint8 r, g, b, a;   /**< Texture modulation values */

    SDL_Renderer *renderer;

    void *driverdata;   /**< Driver specific texture representation */

    SDL_Texture *next;
};

/* Define the SDL renderer structure */
struct SDL_Renderer
{
    int (*ActivateRenderer) (SDL_Renderer * renderer);
    int (*DisplayModeChanged) (SDL_Renderer * renderer);
    int (*CreateTexture) (SDL_Renderer * renderer, SDL_Texture * texture);
    int (*QueryTexturePixels) (SDL_Renderer * renderer, SDL_Texture * texture,
                               void **pixels, int *pitch);
    int (*SetTexturePalette) (SDL_Renderer * renderer, SDL_Texture * texture,
                              const SDL_Color * colors, int firstcolor,
                              int ncolors);
    int (*GetTexturePalette) (SDL_Renderer * renderer, SDL_Texture * texture,
                              SDL_Color * colors, int firstcolor,
                              int ncolors);
    int (*SetTextureColorMod) (SDL_Renderer * renderer,
                               SDL_Texture * texture);
    int (*SetTextureAlphaMod) (SDL_Renderer * renderer,
                               SDL_Texture * texture);
    int (*SetTextureBlendMode) (SDL_Renderer * renderer,
                                SDL_Texture * texture);
    int (*SetTextureScaleMode) (SDL_Renderer * renderer,
                                SDL_Texture * texture);
    int (*UpdateTexture) (SDL_Renderer * renderer, SDL_Texture * texture,
                          const SDL_Rect * rect, const void *pixels,
                          int pitch);
    int (*LockTexture) (SDL_Renderer * renderer, SDL_Texture * texture,
                        const SDL_Rect * rect, int markDirty, void **pixels,
                        int *pitch);
    void (*UnlockTexture) (SDL_Renderer * renderer, SDL_Texture * texture);
    void (*DirtyTexture) (SDL_Renderer * renderer, SDL_Texture * texture,
                          int numrects, const SDL_Rect * rects);
    int (*RenderFill) (SDL_Renderer * renderer, Uint8 r, Uint8 g, Uint8 b,
                       Uint8 a, const SDL_Rect * rect);
    int (*RenderCopy) (SDL_Renderer * renderer, SDL_Texture * texture,
                       const SDL_Rect * srcrect, const SDL_Rect * dstrect);
    void (*RenderPresent) (SDL_Renderer * renderer);
    void (*DestroyTexture) (SDL_Renderer * renderer, SDL_Texture * texture);

    void (*DestroyRenderer) (SDL_Renderer * renderer);

    /* The current renderer info */
    SDL_RendererInfo info;

    /* The window associated with the renderer */
    SDL_WindowID window;

    void *driverdata;
};

/* Define the SDL render driver structure */
struct SDL_RenderDriver
{
    SDL_Renderer *(*CreateRenderer) (SDL_Window * window, Uint32 flags);

    /* Info about the renderer capabilities */
    SDL_RendererInfo info;
};

/* Define the SDL window structure, corresponding to toplevel windows */
struct SDL_Window
{
    Uint32 id;

    char *title;
    int x, y;
    int w, h;
    Uint32 flags;

    int display;
    SDL_Renderer *renderer;
    SDL_GLContext context;

    void *userdata;
    void *driverdata;
};
#define FULLSCREEN_VISIBLE(W) \
    (((W)->flags & SDL_WINDOW_FULLSCREEN) && \
     ((W)->flags & SDL_WINDOW_SHOWN) && \
     !((W)->flags & SDL_WINDOW_MINIMIZED))

/* Define the SDL display structure
   This corresponds to physical monitors attached to the system.
 */
struct SDL_VideoDisplay
{
    int max_display_modes;
    int num_display_modes;
    SDL_DisplayMode *display_modes;
    SDL_DisplayMode desktop_mode;
    SDL_DisplayMode current_mode;
    SDL_DisplayMode fullscreen_mode;
    SDL_Palette *palette;

    Uint16 *gamma;
    Uint16 *saved_gamma;        /* (just offset into gamma) */

    int num_render_drivers;
    SDL_RenderDriver *render_drivers;

    int num_windows;
    SDL_Window *windows;

    SDL_Renderer *current_renderer;

    /* The hash list of textures */
    SDL_Texture *textures[64];

    SDL_VideoDevice *device;

    void *driverdata;
};

/* Define the SDL video driver structure */
#define _THIS	SDL_VideoDevice *_this

struct SDL_VideoDevice
{
    /* * * */
    /* The name of this video driver */
    const char *name;

    /* * * */
    /* Initialization/Query functions */

    /* Initialize the native video subsystem, filling in the list
       of displays for this driver, returning 0 or -1 if there's an error.
     */
    int (*VideoInit) (_THIS);

    /* Reverse the effects VideoInit() -- called if VideoInit() fails
       or if the application is shutting down the video subsystem.
     */
    void (*VideoQuit) (_THIS);

    /* * * */
    /* Display functions
     */

    /* Get a list of the available display modes.
     * e.g.  SDL_AddDisplayMode(_this->current_display, mode)
     */
    void (*GetDisplayModes) (_THIS);

    /* Setting the display mode is independent of creating windows,
     * so when the display mode is changed, all existing windows
     * should have their data updated accordingly, including the
     * display surfaces associated with them.
     */
    int (*SetDisplayMode) (_THIS, SDL_DisplayMode * mode);

    /* Set the color entries of the display palette */
    int (*SetDisplayPalette) (_THIS, SDL_Palette * palette);

    /* Get the color entries of the display palette */
    int (*GetDisplayPalette) (_THIS, SDL_Palette * palette);

    /* Set the gamma ramp */
    int (*SetDisplayGammaRamp) (_THIS, Uint16 * ramp);

    /* Get the gamma ramp */
    int (*GetDisplayGammaRamp) (_THIS, Uint16 * ramp);

    /* * * */
    /* Window functions
     */
    int (*CreateWindow) (_THIS, SDL_Window * window);
    int (*CreateWindowFrom) (_THIS, SDL_Window * window, const void *data);
    void (*SetWindowTitle) (_THIS, SDL_Window * window);
    void (*SetWindowPosition) (_THIS, SDL_Window * window);
    void (*SetWindowSize) (_THIS, SDL_Window * window);
    void (*ShowWindow) (_THIS, SDL_Window * window);
    void (*HideWindow) (_THIS, SDL_Window * window);
    void (*RaiseWindow) (_THIS, SDL_Window * window);
    void (*MaximizeWindow) (_THIS, SDL_Window * window);
    void (*MinimizeWindow) (_THIS, SDL_Window * window);
    void (*RestoreWindow) (_THIS, SDL_Window * window);
    void (*SetWindowGrab) (_THIS, SDL_Window * window);
    void (*DestroyWindow) (_THIS, SDL_Window * window);

    /* Get some platform dependent window information */
      SDL_bool(*GetWindowWMInfo) (_THIS, SDL_Window * window,
                                  struct SDL_SysWMinfo * info);

    /* * * */
    /* OpenGL support
     */
    int (*GL_LoadLibrary) (_THIS, const char *path);
    void *(*GL_GetProcAddress) (_THIS, const char *proc);
      SDL_GLContext(*GL_CreateContext) (_THIS, SDL_Window * window);
    int (*GL_MakeCurrent) (_THIS, SDL_Window * window, SDL_GLContext context);
    int (*GL_SetSwapInterval) (_THIS, int interval);
    int (*GL_GetSwapInterval) (_THIS);
    void (*GL_SwapWindow) (_THIS, SDL_Window * window);
    void (*GL_DeleteContext) (_THIS, SDL_GLContext context);

    /* * * */
    /* Event manager functions
     */
    void (*PumpEvents) (_THIS);

    /* Get the layout key code corresponding to the given physical key code
     * according to the OS' current keyboard layout.
     *
     * - For character keys, this should return the Unicode code point of the
     * character that is generated when the key is pressed without shift or
     * any other modifiers.
     * - For non-character keys, this should return one of the SDLK_* constants
     * (usually the argument itself since these keys are typically layout-
     * independent). Make sure that all of these values returned by this
     * function have a suitable name defined, either the default from
     * SDL_keynames.h, or one set using SDL_SetKeyName() in VideoInit(). In
     * particular, if this function can return any of the codes whose default
     * names start with "SDLK_" (usually, it shouldn't, since these are layout-
     * dependent character keys), these names should be replaced by proper
     * user-readable names.
     * - If there are keys that cannot be adequately described by either a
     * single Unicode character or an SDLK_* constant, this function may return
     * a code with SDL_KEY_LAYOUT_SPECIAL_BIT set in the most significant byte
     * and an arbitrary value in the less significant 3 bytes. The
     * GetSpecialKeyName() function must then be implemented to take this code
     * and return a human-readable key name for it.
     * If the argument is not a physical key code or if translation of the key
     * code by the OS' keyboard layout fails for any reason, the argument must
     * be returned unchanged.
     *
     * On platforms that don't have the notion of a user-configurable keyboard
     * layout, this may be left unimplemented. The default implementation of
     * SDL_GetLayoutKey() then acts as the identity. The note about defining
     * key names above particularly applies in this case.
     */
      SDLKey(*GetLayoutKey) (_THIS, SDLKey physicalKey);

    /* Get a human-readable name for a special layout key code.
     * This only needs to be implemented if this driver's implementation of
     * GetLayoutKey() generates such codes (with SDL_KEY_LAYOUT_SPECIAL_BIT
     * set) - see above.
     */
    const char *(*GetSpecialKeyName) (_THIS, SDLKey layoutKey);

    /* * * */
    /* Data common to all drivers */
    int num_displays;
    SDL_VideoDisplay *displays;
    int current_display;
    Uint32 next_object_id;

    /* * * */
    /* Data used by the GL drivers */
    struct
    {
        int red_size;
        int green_size;
        int blue_size;
        int alpha_size;
        int depth_size;
        int buffer_size;
        int stencil_size;
        int double_buffer;
        int accum_red_size;
        int accum_green_size;
        int accum_blue_size;
        int accum_alpha_size;
        int stereo;
        int multisamplebuffers;
        int multisamplesamples;
        int accelerated;
        int driver_loaded;
        char driver_path[256];
        void *dll_handle;
    } gl_config;

    /* * * */
    /* Data private to this driver */
    void *driverdata;
    struct SDL_GLDriverData *gl_data;

    /* * * */
    /* The function used to dispose of this structure */
    void (*free) (_THIS);
};

typedef struct VideoBootStrap
{
    const char *name;
    const char *desc;
    int (*available) (void);
    SDL_VideoDevice *(*create) (int devindex);
} VideoBootStrap;

#if SDL_VIDEO_DRIVER_COCOA
extern VideoBootStrap COCOA_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_X11
extern VideoBootStrap X11_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_NANOX
extern VideoBootStrap NX_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_IPOD
extern VideoBootStrap iPod_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_WSCONS
extern VideoBootStrap WSCONS_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_FBCON
extern VideoBootStrap FBCON_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_DIRECTFB
extern VideoBootStrap DirectFB_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_PS2GS
extern VideoBootStrap PS2GS_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_VGL
extern VideoBootStrap VGL_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_SVGALIB
extern VideoBootStrap SVGALIB_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_GAPI
extern VideoBootStrap GAPI_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_WIN32
extern VideoBootStrap WIN32_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_BWINDOW
extern VideoBootStrap BWINDOW_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_PHOTON
extern VideoBootStrap ph_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_EPOC
extern VideoBootStrap EPOC_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_XBIOS
extern VideoBootStrap XBIOS_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_GEM
extern VideoBootStrap GEM_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_DC
extern VideoBootStrap DC_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_RISCOS
extern VideoBootStrap RISCOS_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_OS2FS
extern VideoBootStrap OS2FSLib_bootstrap;
#endif
#if SDL_VIDEO_DRIVER_DUMMY
extern VideoBootStrap DUMMY_bootstrap;
#endif

#define SDL_CurrentDisplay	(_this->displays[_this->current_display])

extern SDL_VideoDevice *SDL_GetVideoDevice();
extern int SDL_AddBasicVideoDisplay(const SDL_DisplayMode * desktop_mode);
extern int SDL_AddVideoDisplay(const SDL_VideoDisplay * display);
extern SDL_bool SDL_AddDisplayMode(int displayIndex,
                                   const SDL_DisplayMode * mode);
extern void SDL_AddRenderDriver(int displayIndex,
                                const SDL_RenderDriver * driver);

extern int SDL_RecreateWindow(SDL_Window * window, Uint32 flags);
extern SDL_Window *SDL_GetWindowFromID(SDL_WindowID windowID);
extern SDL_VideoDisplay *SDL_GetDisplayFromWindow(SDL_Window * window);

extern void SDL_OnWindowShown(SDL_Window * window);
extern void SDL_OnWindowHidden(SDL_Window * window);
extern void SDL_OnWindowResized(SDL_Window * window);
extern void SDL_OnWindowFocusGained(SDL_Window * window);
extern void SDL_OnWindowFocusLost(SDL_Window * window);
extern SDL_WindowID SDL_GetFocusWindow(void);

#endif /* _SDL_sysvideo_h */

/* vi: set ts=4 sw=4 expandtab: */
