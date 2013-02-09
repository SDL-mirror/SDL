
#include <fstream>
#include <string>
#include <vector>
#include "SDLmain_WinRT_common.h"
#include "SDL_winrtrenderer.h"

extern "C" {
#include "SDL_syswm.h"
}

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;
using namespace Windows::UI::Core;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;

extern HRESULT D3D11_CreateDeviceResources(SDL_Renderer * renderer);
extern CoreWindow ^ D3D11_GetCoreWindowFromSDLRenderer(SDL_Renderer * renderer);
extern HRESULT D3D11_CreateWindowSizeDependentResources(SDL_Renderer * renderer);

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

// Recreate all device resources and set them back to the current state.
void SDL_winrtrenderer::HandleDeviceLost()
{
    // Reset these member variables to ensure that UpdateForWindowSizeChange recreates all resources.
    m_sdlRendererData->windowSizeInDIPs.x = 0;
    m_sdlRendererData->windowSizeInDIPs.y = 0;
    m_sdlRendererData->swapChain = nullptr;

    // TODO, WinRT: reconnect HandleDeviceLost to SDL_Renderer
    CreateDeviceResources();
    UpdateForWindowSizeChange();
}

// These are the resources that depend on the device.
void SDL_winrtrenderer::CreateDeviceResources()
{
    DX::ThrowIfFailed(D3D11_CreateDeviceResources(m_sdlRenderer));
}

// Allocate all memory resources that change on a window SizeChanged event.
void SDL_winrtrenderer::CreateWindowSizeDependentResources()
{
    DX::ThrowIfFailed(D3D11_CreateWindowSizeDependentResources(m_sdlRenderer));
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
    CoreWindow ^ coreWindow = D3D11_GetCoreWindowFromSDLRenderer(m_sdlRenderer);
    if (coreWindow->Bounds.Width  != m_sdlRendererData->windowSizeInDIPs.x ||
        coreWindow->Bounds.Height != m_sdlRendererData->windowSizeInDIPs.y ||
        m_sdlRendererData->orientation != DisplayProperties::CurrentOrientation)
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
