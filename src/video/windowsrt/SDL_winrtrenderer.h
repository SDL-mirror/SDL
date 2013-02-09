#pragma once

#include "DirectXHelper.h"
#include "SDL.h"
#include "../../render/direct3d11/SDL_render_d3d11_cpp.h"

// Helper class that initializes DirectX APIs for 3D rendering.
ref class SDL_winrtrenderer
{
internal:
    SDL_winrtrenderer();

public:
    virtual ~SDL_winrtrenderer();
    virtual void Initialize(Windows::UI::Core::CoreWindow^ window);
    virtual void HandleDeviceLost();
    virtual void CreateDeviceResources();
    virtual void CreateWindowSizeDependentResources();
    virtual void UpdateForWindowSizeChange();
    virtual void Present();
    virtual float ConvertDipsToPixels(float dips);

internal:
    virtual void Render(SDL_Surface * surface, SDL_Rect * rects, int numrects);
    void ResizeMainTexture(int w, int h);

internal:
    // Internal SDL renderer (likely a temporary addition, for refactoring purposes):
    SDL_Renderer * m_sdlRenderer;
    D3D11_RenderData * m_sdlRendererData;

protected private:
    // UpdateWindowSurface helper objects
    SDL_Surface * m_mainTextureHelperSurface;

    // Cached renderer properties.
    Windows::Foundation::Size m_renderTargetSize;
    Windows::Foundation::Rect m_windowBounds;
    Platform::Agile<Windows::UI::Core::CoreWindow> m_window;
    Windows::Graphics::Display::DisplayOrientations m_orientation;

    // Transform used for display orientation.
    DirectX::XMFLOAT4X4 m_orientationTransform3D;

    // Has the renderer finished loading?
    bool m_loadingComplete;
};
