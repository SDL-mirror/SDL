#pragma once

#include "DirectXHelper.h"
#include "../../render/direct3d11/SDL_render_d3d11_cpp.h"

extern "C" {
#include "SDL.h"
#include "../../render/SDL_sysrender.h"
}

// Helper class that initializes DirectX APIs for 3D rendering.
ref class SDL_winrtrenderer
{
internal:
    SDL_winrtrenderer();

public:
    virtual ~SDL_winrtrenderer();
    virtual void Present();

internal:
    virtual void Render(SDL_Surface * surface, SDL_Rect * rects, int numrects);
    void ResizeMainTexture(int w, int h);

internal:
    // Internal SDL renderer (likely a temporary addition, for refactoring purposes):
    SDL_Renderer * m_sdlRenderer;
    D3D11_RenderData * m_sdlRendererData;

protected private:
    // UpdateWindowSurface helper objects
    SDL_Texture * m_mainTexture;
};
