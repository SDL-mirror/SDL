/*
  Simple DirectMedia Layer
  Copyright (C) 1997-2012 Sam Lantinga <slouken@libsdl.org>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#include "SDL_config.h"

#if SDL_VIDEO_RENDER_D3D11 && !SDL_RENDER_DISABLED

extern "C" {
#include "../../core/windows/SDL_windows.h"
//#include "SDL_hints.h"
//#include "SDL_loadso.h"
#include "SDL_syswm.h"
#include "../SDL_sysrender.h"
//#include "stdio.h"
}

#include "SDL_render_d3d11_cpp.h"

/* Direct3D renderer implementation */

static SDL_Renderer *D3D11_CreateRenderer(SDL_Window * window, Uint32 flags);
//static void D3D11_WindowEvent(SDL_Renderer * renderer,
//                            const SDL_WindowEvent *event);
//static int D3D11_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
//static int D3D11_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
//                             const SDL_Rect * rect, const void *pixels,
//                             int pitch);
//static int D3D11_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
//                           const SDL_Rect * rect, void **pixels, int *pitch);
//static void D3D11_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
//static int D3D11_SetRenderTarget(SDL_Renderer * renderer, SDL_Texture * texture);
static int D3D11_UpdateViewport(SDL_Renderer * renderer);
//static int D3D11_RenderClear(SDL_Renderer * renderer);
//static int D3D11_RenderDrawPoints(SDL_Renderer * renderer,
//                                const SDL_FPoint * points, int count);
//static int D3D11_RenderDrawLines(SDL_Renderer * renderer,
//                               const SDL_FPoint * points, int count);
//static int D3D11_RenderFillRects(SDL_Renderer * renderer,
//                               const SDL_FRect * rects, int count);
//static int D3D11_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
//                          const SDL_Rect * srcrect, const SDL_FRect * dstrect);
//static int D3D11_RenderCopyEx(SDL_Renderer * renderer, SDL_Texture * texture,
//                          const SDL_Rect * srcrect, const SDL_FRect * dstrect,
//                          const double angle, const SDL_FPoint * center, const SDL_RendererFlip flip);
//static int D3D11_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
//                                Uint32 format, void * pixels, int pitch);
static void D3D11_RenderPresent(SDL_Renderer * renderer);
//static void D3D11_DestroyTexture(SDL_Renderer * renderer,
//                               SDL_Texture * texture);
//static void D3D11_DestroyRenderer(SDL_Renderer * renderer);


extern "C" {
    SDL_RenderDriver D3D11_RenderDriver = {
    D3D11_CreateRenderer,
    {
     "direct3d",
     (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE),
     1,
     {SDL_PIXELFORMAT_ARGB8888},
     0,
     0}
    };
}

//typedef struct
//{
//    float x, y, z;
//    DWORD color;
//    float u, v;
//} Vertex;

SDL_Renderer *
D3D11_CreateRenderer(SDL_Window * window, Uint32 flags)
{
    SDL_Renderer *renderer;
    D3D11_RenderData *data;
//    SDL_SysWMinfo windowinfo;
 //   HRESULT result;
 //   D3DPRESENT_PARAMETERS pparams;
 //   IDirect3DSwapChain9 *chain;
 //   D3DCAPS9 caps;
 //   Uint32 window_flags;
 //   int w, h;
 //   SDL_DisplayMode fullscreen_mode;
 //   D3DMATRIX matrix;
 //   int d3dxVersion;
	//char d3dxDLLFile[50];

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(renderer);

    data = new D3D11_RenderData;    // Use the C++ 'new' operator to make sure the struct's members initialize using C++ rules
    if (!data) {
        delete data;
        SDL_OutOfMemory();
        return NULL;
    }

    // TODO: Create Direct3D Object(s)

    //renderer->WindowEvent = D3D11_WindowEvent;
    //renderer->CreateTexture = D3D11_CreateTexture;
    //renderer->UpdateTexture = D3D11_UpdateTexture;
    //renderer->LockTexture = D3D11_LockTexture;
    //renderer->UnlockTexture = D3D11_UnlockTexture;
    //renderer->SetRenderTarget = D3D11_SetRenderTarget;
    renderer->UpdateViewport = D3D11_UpdateViewport;
    //renderer->RenderClear = D3D11_RenderClear;
    //renderer->RenderDrawPoints = D3D11_RenderDrawPoints;
    //renderer->RenderDrawLines = D3D11_RenderDrawLines;
    //renderer->RenderFillRects = D3D11_RenderFillRects;
    //renderer->RenderCopy = D3D11_RenderCopy;
    //renderer->RenderCopyEx = D3D11_RenderCopyEx;
    //renderer->RenderReadPixels = D3D11_RenderReadPixels;
    renderer->RenderPresent = D3D11_RenderPresent;
    //renderer->DestroyTexture = D3D11_DestroyTexture;
    //renderer->DestroyRenderer = D3D11_DestroyRenderer;
    renderer->info = D3D11_RenderDriver.info;
    renderer->driverdata = data;

    renderer->info.flags = SDL_RENDERER_ACCELERATED;

    //SDL_VERSION(&windowinfo.version);
    //SDL_GetWindowWMInfo(window, &windowinfo);

    //window_flags = SDL_GetWindowFlags(window);
    //SDL_GetWindowSize(window, &w, &h);
    //SDL_GetWindowDisplayMode(window, &fullscreen_mode);

    //SDL_zero(pparams);
    //pparams.hDeviceWindow = windowinfo.info.win.window;
    //pparams.BackBufferWidth = w;
    //pparams.BackBufferHeight = h;
    //if (window_flags & SDL_WINDOW_FULLSCREEN) {
    //    pparams.BackBufferFormat =
    //        PixelFormatToD3DFMT(fullscreen_mode.format);
    //} else {
    //    pparams.BackBufferFormat = D3DFMT_UNKNOWN;
    //}
    //pparams.BackBufferCount = 1;
    //pparams.SwapEffect = D3DSWAPEFFECT_DISCARD;

    //if (window_flags & SDL_WINDOW_FULLSCREEN) {
    //    pparams.Windowed = FALSE;
    //    pparams.FullScreen_RefreshRateInHz =
    //        fullscreen_mode.refresh_rate;
    //} else {
    //    pparams.Windowed = TRUE;
    //    pparams.FullScreen_RefreshRateInHz = 0;
    //}
    //if (flags & SDL_RENDERER_PRESENTVSYNC) {
    //    pparams.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
    //} else {
    //    pparams.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    //}

    ///* FIXME: Which adapter? */
    //data->adapter = D3DADAPTER_DEFAULT;
    //IDirect3D9_GetDeviceCaps(data->d3d, data->adapter, D3DDEVTYPE_HAL, &caps);

    //result = IDirect3D9_CreateDevice(data->d3d, data->adapter,
    //                                 D3DDEVTYPE_HAL,
    //                                 pparams.hDeviceWindow,
    //                                 D3DCREATE_FPU_PRESERVE | ((caps.
    //                                  DevCaps &
    //                                  D3DDEVCAPS_HWTRANSFORMANDLIGHT) ?
    //                                 D3DCREATE_HARDWARE_VERTEXPROCESSING :
    //                                 D3DCREATE_SOFTWARE_VERTEXPROCESSING),
    //                                 &pparams, &data->device);
    //if (FAILED(result)) {
    //    D3D11_DestroyRenderer(renderer);
    //    D3D11_SetError("CreateDevice()", result);
    //    return NULL;
    //}
    //data->beginScene = SDL_TRUE;
    //data->scaleMode = D3DTEXF_FORCE_DWORD;

    ///* Get presentation parameters to fill info */
    //result = IDirect3DDevice9_GetSwapChain(data->device, 0, &chain);
    //if (FAILED(result)) {
    //    D3D11_DestroyRenderer(renderer);
    //    D3D11_SetError("GetSwapChain()", result);
    //    return NULL;
    //}
    //result = IDirect3DSwapChain9_GetPresentParameters(chain, &pparams);
    //if (FAILED(result)) {
    //    IDirect3DSwapChain9_Release(chain);
    //    D3D11_DestroyRenderer(renderer);
    //    D3D11_SetError("GetPresentParameters()", result);
    //    return NULL;
    //}
    //IDirect3DSwapChain9_Release(chain);
    //if (pparams.PresentationInterval == D3DPRESENT_INTERVAL_ONE) {
    //    renderer->info.flags |= SDL_RENDERER_PRESENTVSYNC;
    //}
    //data->pparams = pparams;

    //IDirect3DDevice9_GetDeviceCaps(data->device, &caps);
    //renderer->info.max_texture_width = caps.MaxTextureWidth;
    //renderer->info.max_texture_height = caps.MaxTextureHeight;
    //if (caps.NumSimultaneousRTs >= 2) {
    //    renderer->info.flags |= SDL_RENDERER_TARGETTEXTURE;
    //}

    ///* Set up parameters for rendering */
    //IDirect3DDevice9_SetVertexShader(data->device, NULL);
    //IDirect3DDevice9_SetFVF(data->device,
    //                        D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    //IDirect3DDevice9_SetRenderState(data->device, D3DRS_ZENABLE, D3DZB_FALSE);
    //IDirect3DDevice9_SetRenderState(data->device, D3DRS_CULLMODE,
    //                                D3DCULL_NONE);
    //IDirect3DDevice9_SetRenderState(data->device, D3DRS_LIGHTING, FALSE);
    ///* Enable color modulation by diffuse color */
    //IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLOROP,
    //                                      D3DTOP_MODULATE);
    //IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLORARG1,
    //                                      D3DTA_TEXTURE);
    //IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_COLORARG2,
    //                                      D3DTA_DIFFUSE);
    ///* Enable alpha modulation by diffuse alpha */
    //IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAOP,
    //                                      D3DTOP_MODULATE);
    //IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAARG1,
    //                                      D3DTA_TEXTURE);
    //IDirect3DDevice9_SetTextureStageState(data->device, 0, D3DTSS_ALPHAARG2,
    //                                      D3DTA_DIFFUSE);
    ///* Disable second texture stage, since we're done */
    //IDirect3DDevice9_SetTextureStageState(data->device, 1, D3DTSS_COLOROP,
    //                                      D3DTOP_DISABLE);
    //IDirect3DDevice9_SetTextureStageState(data->device, 1, D3DTSS_ALPHAOP,
    //                                      D3DTOP_DISABLE);

    ///* Store the default render target */
    //IDirect3DDevice9_GetRenderTarget(data->device, 0, &data->defaultRenderTarget );
    //data->currentRenderTarget = NULL;

    ///* Set an identity world and view matrix */
    //matrix.m[0][0] = 1.0f;
    //matrix.m[0][1] = 0.0f;
    //matrix.m[0][2] = 0.0f;
    //matrix.m[0][3] = 0.0f;
    //matrix.m[1][0] = 0.0f;
    //matrix.m[1][1] = 1.0f;
    //matrix.m[1][2] = 0.0f;
    //matrix.m[1][3] = 0.0f;
    //matrix.m[2][0] = 0.0f;
    //matrix.m[2][1] = 0.0f;
    //matrix.m[2][2] = 1.0f;
    //matrix.m[2][3] = 0.0f;
    //matrix.m[3][0] = 0.0f;
    //matrix.m[3][1] = 0.0f;
    //matrix.m[3][2] = 0.0f;
    //matrix.m[3][3] = 1.0f;
    //IDirect3DDevice9_SetTransform(data->device, D3DTS_WORLD, &matrix);
    //IDirect3DDevice9_SetTransform(data->device, D3DTS_VIEW, &matrix);

    return renderer;
}

static int
D3D11_UpdateViewport(SDL_Renderer * renderer)
{
    return 0;
}

static void
D3D11_RenderPresent(SDL_Renderer * renderer)
{
    D3D11_RenderData *data = (D3D11_RenderData *) renderer->driverdata;

#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = data->swapChain->Present(1, 0);
#else
    // The application may optionally specify "dirty" or "scroll"
    // rects to improve efficiency in certain scenarios.
    // This option is not available on Windows Phone 8, to note.
    DXGI_PRESENT_PARAMETERS parameters = {0};
    parameters.DirtyRectsCount = 0;
    parameters.pDirtyRects = nullptr;
    parameters.pScrollRect = nullptr;
    parameters.pScrollOffset = nullptr;
    
    // The first argument instructs DXGI to block until VSync, putting the application
    // to sleep until the next VSync. This ensures we don't waste any cycles rendering
    // frames that will never be displayed to the screen.
    HRESULT hr = data->swapChain->Present1(1, 0, &parameters);
#endif

    // Discard the contents of the render target.
    // This is a valid operation only when the existing contents will be entirely
    // overwritten. If dirty or scroll rects are used, this call should be removed.
    data->d3dContext->DiscardView(data->renderTargetView.Get());

    // If the device was removed either by a disconnect or a driver upgrade, we 
    // must recreate all device resources.
    if (hr == DXGI_ERROR_DEVICE_REMOVED)
    {
        extern void WINRT_HandleDeviceLost();   // TODO, WinRT: move lost-device handling into the Direct3D 11.1 renderer, as appropriate
        WINRT_HandleDeviceLost();
    }
    else
    {
        WIN_SetErrorFromHRESULT(__FUNCTION__, hr);
        // TODO, WinRT: consider throwing an exception if D3D11_RenderPresent fails, especially if there is a way to salvedge debug info from users' machines
    }
}

#endif /* SDL_VIDEO_RENDER_D3D && !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
