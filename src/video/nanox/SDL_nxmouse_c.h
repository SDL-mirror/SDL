#include "SDL_nxvideo.h"

extern WMcursor * NX_CreateWMCursor (_THIS, Uint8 * data, Uint8 * mask, int w, int h, int hot_x, int hot_y) ;
void NX_FreeWMCursor (_THIS, WMcursor * cursor) ;
extern void NX_WarpWMCursor (_THIS, Uint16 x, Uint16 y) ;
extern int NX_ShowWMCursor (_THIS, WMcursor * cursor) ;
