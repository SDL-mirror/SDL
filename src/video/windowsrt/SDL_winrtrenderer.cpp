
#include <fstream>
#include <string>
#include <vector>
#include "SDLmain_WinRT_common.h"
#include "SDL_winrtrenderer.h"

extern "C" {
#include "SDL_syswm.h"
#include "../../core/windows/SDL_windows.h"
}

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

// Constructor.
SDL_winrtrenderer::SDL_winrtrenderer() :
    m_sdlRenderer(NULL),
    m_sdlRendererData(NULL),
    m_mainTexture(NULL)
{
}

SDL_winrtrenderer::~SDL_winrtrenderer()
{
    if (m_mainTexture) {
        SDL_DestroyTexture(m_mainTexture);
        m_mainTexture = NULL;
    }
}

void SDL_winrtrenderer::ResizeMainTexture(int w, int h)
{
    if (m_mainTexture) {
        SDL_DestroyTexture(m_mainTexture);
        m_mainTexture = NULL;
    }

    m_mainTexture = SDL_CreateTexture(m_sdlRenderer, SDL_PIXELFORMAT_RGB888, SDL_TEXTUREACCESS_TARGET, w, h);
}

static inline Platform::Exception ^
WINRT_CreateExceptionWithSDLError()
{
    wchar_t * sdlErrorMessage = WIN_UTF8ToString(SDL_GetError());
    Platform::String ^ errorMessage = ref new Platform::String(sdlErrorMessage);
    SDL_free(sdlErrorMessage);
    throw ref new Platform::FailureException(errorMessage);
}

void SDL_winrtrenderer::Render(SDL_Surface * surface, SDL_Rect * rects, int numrects)
{
    D3D11_TextureData * textureData = (D3D11_TextureData *)m_mainTexture->driverdata;

    SDL_SetRenderDrawColor(m_sdlRenderer, 0, 0, 0, 0);
    if (SDL_RenderClear(m_sdlRenderer) != 0) {
        throw WINRT_CreateExceptionWithSDLError();
    }

    // Only draw the screen once it is loaded (some loading is asynchronous).
    if (!m_sdlRendererData->loadingComplete) {
        return;
    }
    if (!textureData->mainTextureResourceView) {
        return;
    }

    // Update the main texture (for SDL usage).
    // TODO, WinRT: only update the requested rects (passed to SDL_UpdateWindowSurface), rather than everything
    if (SDL_UpdateTexture(m_mainTexture, NULL, surface->pixels, surface->pitch) != 0) {
        throw WINRT_CreateExceptionWithSDLError();
    }

    if (SDL_RenderCopy(m_sdlRenderer, m_mainTexture, NULL, NULL) != 0) {
        throw WINRT_CreateExceptionWithSDLError();
    }
}

// Method to deliver the final image to the display.
void SDL_winrtrenderer::Present()
{
    SDL_RenderPresent(m_sdlRenderer);
}
