
#include <fstream>
#include <string>
#include <vector>
#include "SDLmain_WinRT_common.h"
#include "SDL_winrtrenderer.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

// Constructor.
SDL_winrtrenderer::SDL_winrtrenderer() :
    m_mainTextureHelperSurface(NULL),
    m_sdlRenderer(NULL),
    m_sdlRendererData(NULL)
{
}

SDL_winrtrenderer::~SDL_winrtrenderer()
{
    if (m_mainTextureHelperSurface) {
        SDL_FreeSurface(m_mainTextureHelperSurface);
        m_mainTextureHelperSurface = NULL;
    }
}

// Initialize the Direct3D resources required to run.
void SDL_winrtrenderer::Initialize(CoreWindow^ window)
{
    m_window = window;
    
    CreateDeviceResources();
    CreateWindowSizeDependentResources();
}

// Recreate all device resources and set them back to the current state.
void SDL_winrtrenderer::HandleDeviceLost()
{
    // Reset these member variables to ensure that UpdateForWindowSizeChange recreates all resources.
    m_windowBounds.Width = 0;
    m_windowBounds.Height = 0;
    m_sdlRendererData->swapChain = nullptr;

    // TODO, WinRT: reconnect HandleDeviceLost to SDL_Renderer
    CreateDeviceResources();
    UpdateForWindowSizeChange();
}

extern HRESULT WINRT_CreateDeviceResources(SDL_Renderer * renderer);

// These are the resources that depend on the device.
void SDL_winrtrenderer::CreateDeviceResources()
{
    DX::ThrowIfFailed(WINRT_CreateDeviceResources(m_sdlRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void SDL_winrtrenderer::CreateWindowSizeDependentResources()
{
    // Store the window bounds so the next time we get a SizeChanged event we can
    // avoid rebuilding everything if the size is identical.
    m_windowBounds = m_window->Bounds;

    // Calculate the necessary swap chain and render target size in pixels.
    float windowWidth = ConvertDipsToPixels(m_windowBounds.Width);
    float windowHeight = ConvertDipsToPixels(m_windowBounds.Height);

    // The width and height of the swap chain must be based on the window's
    // landscape-oriented width and height. If the window is in a portrait
    // orientation, the dimensions must be reversed.
    m_orientation = DisplayProperties::CurrentOrientation;
    bool swapDimensions =
        m_orientation == DisplayOrientations::Portrait ||
        m_orientation == DisplayOrientations::PortraitFlipped;
    m_renderTargetSize.Width = swapDimensions ? windowHeight : windowWidth;
    m_renderTargetSize.Height = swapDimensions ? windowWidth : windowHeight;

    if(m_sdlRendererData->swapChain != nullptr)
    {
        // If the swap chain already exists, resize it.
        DX::ThrowIfFailed(
            m_sdlRendererData->swapChain->ResizeBuffers(
                2, // Double-buffered swap chain.
                static_cast<UINT>(m_renderTargetSize.Width),
                static_cast<UINT>(m_renderTargetSize.Height),
                DXGI_FORMAT_B8G8R8A8_UNORM,
                0
                )
            );
    }
    else
    {
        // Otherwise, create a new one using the same adapter as the existing Direct3D device.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
        swapChainDesc.Width = static_cast<UINT>(m_renderTargetSize.Width); // Match the size of the window.
        swapChainDesc.Height = static_cast<UINT>(m_renderTargetSize.Height);
        swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
#if WINAPI_FAMILY == WINAPI_FAMILY_PHONE_APP
        swapChainDesc.Scaling = DXGI_SCALING_STRETCH; // On phone, only stretch and aspect-ratio stretch scaling are allowed.
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD; // On phone, no swap effects are supported.
#else
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
#endif
        swapChainDesc.Flags = 0;

        ComPtr<IDXGIDevice1>  dxgiDevice;
        DX::ThrowIfFailed(
            m_sdlRendererData->d3dDevice.As(&dxgiDevice)
            );

        ComPtr<IDXGIAdapter> dxgiAdapter;
        DX::ThrowIfFailed(
            dxgiDevice->GetAdapter(&dxgiAdapter)
            );

        ComPtr<IDXGIFactory2> dxgiFactory;
        DX::ThrowIfFailed(
            dxgiAdapter->GetParent(
                __uuidof(IDXGIFactory2), 
                &dxgiFactory
                )
            );

        Windows::UI::Core::CoreWindow^ window = m_window.Get();
        DX::ThrowIfFailed(
            dxgiFactory->CreateSwapChainForCoreWindow(
                m_sdlRendererData->d3dDevice.Get(),
                reinterpret_cast<IUnknown*>(window),
                &swapChainDesc,
                nullptr, // Allow on all displays.
                &m_sdlRendererData->swapChain
                )
            );
            
        // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
        // ensures that the application will only render after each VSync, minimizing power consumption.
        DX::ThrowIfFailed(
            dxgiDevice->SetMaximumFrameLatency(1)
            );
    }
    
    // Set the proper orientation for the swap chain, and generate the
    // 3D matrix transformation for rendering to the rotated swap chain.
    DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
    switch (m_orientation)
    {
        case DisplayOrientations::Landscape:
            rotation = DXGI_MODE_ROTATION_IDENTITY;
            m_orientationTransform3D = XMFLOAT4X4( // 0-degree Z-rotation
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
                );
            break;

        case DisplayOrientations::Portrait:
            rotation = DXGI_MODE_ROTATION_ROTATE270;
            m_orientationTransform3D = XMFLOAT4X4( // 90-degree Z-rotation
                0.0f, 1.0f, 0.0f, 0.0f,
                -1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
                );
            break;

        case DisplayOrientations::LandscapeFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE180;
            m_orientationTransform3D = XMFLOAT4X4( // 180-degree Z-rotation
                -1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
                );
            break;

        case DisplayOrientations::PortraitFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE90;
            m_orientationTransform3D = XMFLOAT4X4( // 270-degree Z-rotation
                0.0f, -1.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
                );
            break;

        default:
            throw ref new Platform::FailureException();
    }

#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
    // TODO, WinRT: Windows Phone does not have the IDXGISwapChain1::SetRotation method.  Check if an alternative is available, or needed.
    DX::ThrowIfFailed(
        m_sdlRendererData->swapChain->SetRotation(rotation)
        );
#endif

    // Create a render target view of the swap chain back buffer.
    ComPtr<ID3D11Texture2D> backBuffer;
    DX::ThrowIfFailed(
        m_sdlRendererData->swapChain->GetBuffer(
            0,
            __uuidof(ID3D11Texture2D),
            &backBuffer
            )
        );

    DX::ThrowIfFailed(
        m_sdlRendererData->d3dDevice->CreateRenderTargetView(
            backBuffer.Get(),
            nullptr,
            &m_sdlRendererData->renderTargetView
            )
        );

    // Create a depth stencil view.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT, 
        static_cast<UINT>(m_renderTargetSize.Width),
        static_cast<UINT>(m_renderTargetSize.Height),
        1,
        1,
        D3D11_BIND_DEPTH_STENCIL
        );

    ComPtr<ID3D11Texture2D> depthStencil;
    DX::ThrowIfFailed(
        m_sdlRendererData->d3dDevice->CreateTexture2D(
            &depthStencilDesc,
            nullptr,
            &depthStencil
            )
        );

    // Set the rendering viewport to target the entire window.
    CD3D11_VIEWPORT viewport(
        0.0f,
        0.0f,
        m_renderTargetSize.Width,
        m_renderTargetSize.Height
        );

    m_sdlRendererData->d3dContext->RSSetViewports(1, &viewport);
}

void SDL_winrtrenderer::ResizeMainTexture(int w, int h)
{
    const int pixelSizeInBytes = 4;

    D3D11_TEXTURE2D_DESC textureDesc = {0};
    textureDesc.Width = w;
    textureDesc.Height = h;
    textureDesc.MipLevels = 1;
    textureDesc.ArraySize = 1;
    textureDesc.Format = DXGI_FORMAT_B8G8R8X8_UNORM;
    textureDesc.SampleDesc.Count = 1;
    textureDesc.SampleDesc.Quality = 0;
    textureDesc.Usage = D3D11_USAGE_DYNAMIC;
    textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    textureDesc.MiscFlags = 0;

    const int numPixels = textureDesc.Width * textureDesc.Height;
    std::vector<uint8> initialTexturePixels(numPixels * pixelSizeInBytes, 0x00);

    // Fill the texture with a non-black color, for debugging purposes:
    //for (int i = 0; i < (numPixels * pixelSizeInBytes); i += pixelSizeInBytes) {
    //    initialTexturePixels[i+0] = 0xff;
    //    initialTexturePixels[i+1] = 0xff;
    //    initialTexturePixels[i+2] = 0x00;
    //    initialTexturePixels[i+3] = 0xff;
    //}

    D3D11_SUBRESOURCE_DATA initialTextureData = {0};
    initialTextureData.pSysMem = (void *)&(initialTexturePixels[0]);
    initialTextureData.SysMemPitch = textureDesc.Width * pixelSizeInBytes;
    initialTextureData.SysMemSlicePitch = numPixels * pixelSizeInBytes;
    DX::ThrowIfFailed(
        m_sdlRendererData->d3dDevice->CreateTexture2D(
            &textureDesc,
            &initialTextureData,
            &m_sdlRendererData->mainTexture
            )
        );

    if (m_mainTextureHelperSurface) {
        SDL_FreeSurface(m_mainTextureHelperSurface);
        m_mainTextureHelperSurface = NULL;
    }
    m_mainTextureHelperSurface = SDL_CreateRGBSurfaceFrom(
        NULL,
        textureDesc.Width, textureDesc.Height,
        (pixelSizeInBytes * 8),
        0,      // Use an nil pitch for now.  This'll be filled in when updating the texture.
        0x00ff0000, 0x0000ff00, 0x000000ff, 0x00000000);    // TODO, WinRT: calculate masks given the Direct3D-defined pixel format of the texture
    if (m_mainTextureHelperSurface == NULL) {
        DX::ThrowIfFailed(E_FAIL);  // TODO, WinRT: generate a better error here, taking into account who's calling this function.
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
    resourceViewDesc.Format = textureDesc.Format;
    resourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    resourceViewDesc.Texture2D.MostDetailedMip = 0;
    resourceViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
    DX::ThrowIfFailed(
        m_sdlRendererData->d3dDevice->CreateShaderResourceView(
            m_sdlRendererData->mainTexture.Get(),
            &resourceViewDesc,
            &m_sdlRendererData->mainTextureResourceView)
        );
}

// This method is called in the event handler for the SizeChanged event.
void SDL_winrtrenderer::UpdateForWindowSizeChange()
{
    if (m_window->Bounds.Width  != m_windowBounds.Width ||
        m_window->Bounds.Height != m_windowBounds.Height ||
        m_orientation != DisplayProperties::CurrentOrientation)
    {
        ID3D11RenderTargetView* nullViews[] = {nullptr};
        m_sdlRendererData->d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
        m_sdlRendererData->renderTargetView = nullptr;
        m_sdlRendererData->d3dContext->Flush();
        CreateWindowSizeDependentResources();
    }
}

void SDL_winrtrenderer::Render(SDL_Surface * surface, SDL_Rect * rects, int numrects)
{
    const float blackColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };
    m_sdlRendererData->d3dContext->ClearRenderTargetView(
        m_sdlRendererData->renderTargetView.Get(),
        blackColor
        );

    // Only draw the screen once it is loaded (some loading is asynchronous).
    if (!m_sdlRendererData->loadingComplete)
    {
        return;
    }
    if (!m_sdlRendererData->mainTextureResourceView)
    {
        return;
    }

    // Update the main texture (for SDL usage).  Start by mapping the SDL
    // window's main texture to CPU-accessible memory:
    D3D11_MAPPED_SUBRESOURCE textureMemory = {0};
    DX::ThrowIfFailed(
        m_sdlRendererData->d3dContext->Map(
            m_sdlRendererData->mainTexture.Get(),
            0,
            D3D11_MAP_WRITE_DISCARD,
            0,
            &textureMemory)
        );

    // Copy pixel data to the locked texture's memory:
    m_mainTextureHelperSurface->pixels = textureMemory.pData;
    m_mainTextureHelperSurface->pitch = textureMemory.RowPitch;
    SDL_BlitSurface(surface, NULL, m_mainTextureHelperSurface, NULL);
    // TODO, WinRT: only update the requested rects (passed to SDL_UpdateWindowSurface), rather than everything

    // Clean up a bit, then commit the texture's memory back to Direct3D:
    m_mainTextureHelperSurface->pixels = NULL;
    m_mainTextureHelperSurface->pitch = 0;
    m_sdlRendererData->d3dContext->Unmap(
        m_sdlRendererData->mainTexture.Get(),
        0);

    m_sdlRendererData->d3dContext->OMSetRenderTargets(
        1,
        m_sdlRendererData->renderTargetView.GetAddressOf(),
        nullptr
        );

    UINT stride = sizeof(VertexPositionColor);
    UINT offset = 0;
    m_sdlRendererData->d3dContext->IASetVertexBuffers(
        0,
        1,
        m_sdlRendererData->vertexBuffer.GetAddressOf(),
        &stride,
        &offset
        );

    m_sdlRendererData->d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    m_sdlRendererData->d3dContext->IASetInputLayout(m_sdlRendererData->inputLayout.Get());

    m_sdlRendererData->d3dContext->VSSetShader(
        m_sdlRendererData->vertexShader.Get(),
        nullptr,
        0
        );

    m_sdlRendererData->d3dContext->PSSetShader(
        m_sdlRendererData->pixelShader.Get(),
        nullptr,
        0
        );

    m_sdlRendererData->d3dContext->PSSetShaderResources(0, 1, m_sdlRendererData->mainTextureResourceView.GetAddressOf());

    m_sdlRendererData->d3dContext->PSSetSamplers(0, 1, m_sdlRendererData->mainSampler.GetAddressOf());

    m_sdlRendererData->d3dContext->Draw(4, 0);
}

// Method to deliver the final image to the display.
void SDL_winrtrenderer::Present()
{
    SDL_RenderPresent(m_sdlRenderer);
}

// Method to convert a length in device-independent pixels (DIPs) to a length in physical pixels.
float SDL_winrtrenderer::ConvertDipsToPixels(float dips)
{
    static const float dipsPerInch = 96.0f;
    return floor(dips * DisplayProperties::LogicalDpi / dipsPerInch + 0.5f); // Round to nearest integer.
}
