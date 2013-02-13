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
#include "SDL_system.h"
#include "SDL_syswm.h"
#include "../SDL_sysrender.h"
//#include "stdio.h"
}

#include <fstream>
#include <string>
#include <vector>

#include "SDL_render_d3d11_cpp.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace std;

#ifdef __WINRT__
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;
#endif

/* Direct3D 11.1 renderer implementation */

static SDL_Renderer *D3D11_CreateRenderer(SDL_Window * window, Uint32 flags);
static void D3D11_WindowEvent(SDL_Renderer * renderer,
                            const SDL_WindowEvent *event);
static int D3D11_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture);
static int D3D11_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                             const SDL_Rect * rect, const void *pixels,
                             int pitch);
//static int D3D11_LockTexture(SDL_Renderer * renderer, SDL_Texture * texture,
//                           const SDL_Rect * rect, void **pixels, int *pitch);
//static void D3D11_UnlockTexture(SDL_Renderer * renderer, SDL_Texture * texture);
//static int D3D11_SetRenderTarget(SDL_Renderer * renderer, SDL_Texture * texture);
static int D3D11_UpdateViewport(SDL_Renderer * renderer);
static int D3D11_RenderClear(SDL_Renderer * renderer);
//static int D3D11_RenderDrawPoints(SDL_Renderer * renderer,
//                                const SDL_FPoint * points, int count);
//static int D3D11_RenderDrawLines(SDL_Renderer * renderer,
//                               const SDL_FPoint * points, int count);
//static int D3D11_RenderFillRects(SDL_Renderer * renderer,
//                               const SDL_FRect * rects, int count);
static int D3D11_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * srcrect, const SDL_FRect * dstrect);
//static int D3D11_RenderCopyEx(SDL_Renderer * renderer, SDL_Texture * texture,
//                          const SDL_Rect * srcrect, const SDL_FRect * dstrect,
//                          const double angle, const SDL_FPoint * center, const SDL_RendererFlip flip);
//static int D3D11_RenderReadPixels(SDL_Renderer * renderer, const SDL_Rect * rect,
//                                Uint32 format, void * pixels, int pitch);
static void D3D11_RenderPresent(SDL_Renderer * renderer);
static void D3D11_DestroyTexture(SDL_Renderer * renderer,
                                 SDL_Texture * texture);
static void D3D11_DestroyRenderer(SDL_Renderer * renderer);

/* Direct3D 11.1 Internal Functions */
HRESULT D3D11_CreateDeviceResources(SDL_Renderer * renderer);
HRESULT D3D11_CreateWindowSizeDependentResources(SDL_Renderer * renderer);
HRESULT D3D11_UpdateForWindowSizeChange(SDL_Renderer * renderer);
HRESULT D3D11_HandleDeviceLost(SDL_Renderer * renderer);

extern "C" {
    SDL_RenderDriver D3D11_RenderDriver = {
    D3D11_CreateRenderer,
    {
     "direct3d 11.1",
     (SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_TARGETTEXTURE),
     1,
     {SDL_PIXELFORMAT_RGB888},
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

    renderer = (SDL_Renderer *) SDL_calloc(1, sizeof(*renderer));
    if (!renderer) {
        SDL_OutOfMemory();
        return NULL;
    }
    SDL_zerop(renderer);

    data = new D3D11_RenderData;    // Use the C++ 'new' operator to make sure the struct's members initialize using C++ rules
    if (!data) {
        SDL_OutOfMemory();
        return NULL;
    }
    data->featureLevel = (D3D_FEATURE_LEVEL) 0;
    data->loadingComplete = false;
    data->windowSizeInDIPs = XMFLOAT2(0, 0);
    data->renderTargetSize = XMFLOAT2(0, 0);

    renderer->WindowEvent = D3D11_WindowEvent;
    renderer->CreateTexture = D3D11_CreateTexture;
    renderer->UpdateTexture = D3D11_UpdateTexture;
    //renderer->LockTexture = D3D11_LockTexture;
    //renderer->UnlockTexture = D3D11_UnlockTexture;
    //renderer->SetRenderTarget = D3D11_SetRenderTarget;
    renderer->UpdateViewport = D3D11_UpdateViewport;
    renderer->RenderClear = D3D11_RenderClear;
    //renderer->RenderDrawPoints = D3D11_RenderDrawPoints;
    //renderer->RenderDrawLines = D3D11_RenderDrawLines;
    //renderer->RenderFillRects = D3D11_RenderFillRects;
    renderer->RenderCopy = D3D11_RenderCopy;
    //renderer->RenderCopyEx = D3D11_RenderCopyEx;
    //renderer->RenderReadPixels = D3D11_RenderReadPixels;
    renderer->RenderPresent = D3D11_RenderPresent;
    renderer->DestroyTexture = D3D11_DestroyTexture;
    renderer->DestroyRenderer = D3D11_DestroyRenderer;
    renderer->info = D3D11_RenderDriver.info;
    renderer->info.flags = SDL_RENDERER_ACCELERATED;
    renderer->driverdata = data;

    // HACK: make sure the SDL_Renderer references the SDL_Window data now, in
    // order to give init functions access to the underlying window handle:
    renderer->window = window;

    /* Initialize Direct3D resources */
    if (FAILED(D3D11_CreateDeviceResources(renderer))) {
        D3D11_DestroyRenderer(renderer);
        return NULL;
    }
    if (FAILED(D3D11_CreateWindowSizeDependentResources(renderer))) {
        D3D11_DestroyRenderer(renderer);
        return NULL;
    }

    // TODO, WinRT: fill in renderer->info.texture_formats where appropriate

    return renderer;
}

static void
D3D11_DestroyRenderer(SDL_Renderer * renderer)
{
    D3D11_RenderData *data = (D3D11_RenderData *) renderer->driverdata;
    if (data) {
        delete data;
        data = NULL;
    }
}

static bool
D3D11_ReadFileContents(const wstring & fileName, vector<char> & out)
{
    ifstream in(fileName, ios::in | ios::binary);
    if (!in) {
        return false;
    }

    in.seekg(0, ios::end);
    out.resize((size_t) in.tellg());
    in.seekg(0, ios::beg);
    in.read(&out[0], out.size());
    return in.good();
}

static bool
D3D11_ReadShaderContents(const wstring & shaderName, vector<char> & out)
{
    wstring fileName;

#if WINAPI_FAMILY == WINAPI_FAMILY_APP
    fileName = SDL_WinRTGetFileSystemPath(SDL_WINRT_PATH_INSTALLED_LOCATION);
    fileName += L"\\SDL_VS2012_WinRT\\";
#elif WINAPI_FAMILY == WINAPI_PHONE_APP
    fileName = SDL_WinRTGetFileSystemPath(SDL_WINRT_PATH_INSTALLED_LOCATION);
    fileName += L"\\";
#endif
    // WinRT, TODO: test Direct3D 11.1 shader loading on Win32
    fileName += shaderName;
    return D3D11_ReadFileContents(fileName, out);
}

// Create resources that depend on the device.
HRESULT
D3D11_CreateDeviceResources(SDL_Renderer * renderer)
{
    D3D11_RenderData *data = (D3D11_RenderData *) renderer->driverdata;

    // This flag adds support for surfaces with a different color channel ordering
    // than the API default. It is required for compatibility with Direct2D.
    UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
    // If the project is in a debug build, enable debugging via SDK Layers with this flag.
    creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    // This array defines the set of DirectX hardware feature levels this app will support.
    // Note the ordering should be preserved.
    // Don't forget to declare your application's minimum required feature level in its
    // description.  All applications are assumed to support 9.1 unless otherwise stated.
    D3D_FEATURE_LEVEL featureLevels[] = 
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
        D3D_FEATURE_LEVEL_9_3,
        D3D_FEATURE_LEVEL_9_2,
        D3D_FEATURE_LEVEL_9_1
    };

    // Create the Direct3D 11 API device object and a corresponding context.
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    HRESULT result = S_OK;
    result = D3D11CreateDevice(
        nullptr, // Specify nullptr to use the default adapter.
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        creationFlags, // Set set debug and Direct2D compatibility flags.
        featureLevels, // List of feature levels this app can support.
        ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION, // Always set this to D3D11_SDK_VERSION for Windows Store apps.
        &device, // Returns the Direct3D device created.
        &data->featureLevel, // Returns feature level of device created.
        &context // Returns the device immediate context.
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    // Get the Direct3D 11.1 API device and context interfaces.
    Microsoft::WRL::ComPtr<ID3D11Device1> d3dDevice1;
    result = device.As(&(data->d3dDevice));
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    result = context.As(&data->d3dContext);
    if (FAILED(result)) {
        return result;
    }

    // Start loading GPU shaders:
    vector<char> fileData;

    //
    // Load in SDL's one and only vertex shader:
    //
    if (!D3D11_ReadShaderContents(L"SimpleVertexShader.cso", fileData)) {
        SDL_SetError("Unable to open SDL's vertex shader file.");
        return E_FAIL;
    }

    result = data->d3dDevice->CreateVertexShader(
        &fileData[0],
        fileData.size(),
        nullptr,
        &data->vertexShader
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    //
    // Create an input layout for SDL's vertex shader:
    //
    const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    result = data->d3dDevice->CreateInputLayout(
        vertexDesc,
        ARRAYSIZE(vertexDesc),
        &fileData[0],
        fileData.size(),
        &data->inputLayout
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    //
    // Load in SDL's one and only pixel shader (for now, more are likely to follow):
    //
    if (!D3D11_ReadShaderContents(L"SimplePixelShader.cso", fileData)) {
        SDL_SetError("Unable to open SDL's pixel shader file.");
        return E_FAIL;
    }

    result = data->d3dDevice->CreatePixelShader(
        &fileData[0],
        fileData.size(),
        nullptr,
        &data->pixelShader
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    //
    // Setup space to hold vertex shader constants:
    //
    CD3D11_BUFFER_DESC constantBufferDesc(sizeof(SDL_VertexShaderConstants), D3D11_BIND_CONSTANT_BUFFER);
    result = data->d3dDevice->CreateBuffer(
		&constantBufferDesc,
		nullptr,
        &data->vertexShaderConstants
		);
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    //
    // Make sure that the vertex buffer, if already created, gets freed.
    // It will be recreated later.
    //
    data->vertexBuffer = nullptr;

    //
    // Create a sampler to use when drawing textures:
    //
    D3D11_SAMPLER_DESC samplerDesc;
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.MipLODBias = 0.0f;
    samplerDesc.MaxAnisotropy = 1;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
    samplerDesc.BorderColor[0] = 0.0f;
    samplerDesc.BorderColor[1] = 0.0f;
    samplerDesc.BorderColor[2] = 0.0f;
    samplerDesc.BorderColor[3] = 0.0f;
    samplerDesc.MinLOD = 0.0f;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    result = data->d3dDevice->CreateSamplerState(
        &samplerDesc,
        &data->mainSampler
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    //
    // Setup the Direct3D rasterizer
    //
    D3D11_RASTERIZER_DESC rasterDesc;
    memset(&rasterDesc, 0, sizeof(rasterDesc));
	rasterDesc.AntialiasedLineEnable = false;
	rasterDesc.CullMode = D3D11_CULL_NONE;
	rasterDesc.DepthBias = 0;
	rasterDesc.DepthBiasClamp = 0.0f;
	rasterDesc.DepthClipEnable = true;
	rasterDesc.FillMode = D3D11_FILL_SOLID;
	rasterDesc.FrontCounterClockwise = false;
	rasterDesc.MultisampleEnable = false;
	rasterDesc.ScissorEnable = false;
	rasterDesc.SlopeScaledDepthBias = 0.0f;
	result = data->d3dDevice->CreateRasterizerState(&rasterDesc, &data->mainRasterizer);
	if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    //
    // All done!
    //
    data->loadingComplete = true;       // This variable can probably be factored-out
    return S_OK;
}

#ifdef __WINRT__

static CoreWindow ^
D3D11_GetCoreWindowFromSDLRenderer(SDL_Renderer * renderer)
{
    SDL_Window * sdlWindow = renderer->window;
    if ( ! renderer->window ) {
        return nullptr;
    }

    SDL_SysWMinfo sdlWindowInfo;
    SDL_VERSION(&sdlWindowInfo.version);
    if ( ! SDL_GetWindowWMInfo(sdlWindow, &sdlWindowInfo) ) {
        return nullptr;
    }

    if (sdlWindowInfo.subsystem != SDL_SYSWM_WINDOWSRT) {
        return nullptr;
    }

    CoreWindow ^* coreWindowPointer = (CoreWindow ^*) sdlWindowInfo.info.winrt.window;
    if ( ! coreWindowPointer ) {
        return nullptr;
    }

    return *coreWindowPointer;
}

// Method to convert a length in device-independent pixels (DIPs) to a length in physical pixels.
static float
D3D11_ConvertDipsToPixels(float dips)
{
    static const float dipsPerInch = 96.0f;
    return floor(dips * DisplayProperties::LogicalDpi / dipsPerInch + 0.5f); // Round to nearest integer.
}
#endif

// Initialize all resources that change when the window's size changes.
// WinRT, TODO: get D3D11_CreateWindowSizeDependentResources working on Win32
HRESULT
D3D11_CreateWindowSizeDependentResources(SDL_Renderer * renderer)
{
    D3D11_RenderData *data = (D3D11_RenderData *) renderer->driverdata;
    HRESULT result = S_OK;
    Windows::UI::Core::CoreWindow ^ coreWindow = D3D11_GetCoreWindowFromSDLRenderer(renderer);

    // Store the window bounds so the next time we get a SizeChanged event we can
    // avoid rebuilding everything if the size is identical.
    data->windowSizeInDIPs.x = coreWindow->Bounds.Width;
    data->windowSizeInDIPs.y = coreWindow->Bounds.Height;

    // Calculate the necessary swap chain and render target size in pixels.
    float windowWidth = D3D11_ConvertDipsToPixels(data->windowSizeInDIPs.x);
    float windowHeight = D3D11_ConvertDipsToPixels(data->windowSizeInDIPs.y);

    // The width and height of the swap chain must be based on the window's
    // landscape-oriented width and height. If the window is in a portrait
    // orientation, the dimensions must be reversed.
    data->orientation = DisplayProperties::CurrentOrientation;
    bool swapDimensions =
        data->orientation == DisplayOrientations::Portrait ||
        data->orientation == DisplayOrientations::PortraitFlipped;
    data->renderTargetSize.x = swapDimensions ? windowHeight : windowWidth;
    data->renderTargetSize.y = swapDimensions ? windowWidth : windowHeight;

    if(data->swapChain != nullptr)
    {
        // If the swap chain already exists, resize it.
        result = data->swapChain->ResizeBuffers(
            2, // Double-buffered swap chain.
            static_cast<UINT>(data->renderTargetSize.x),
            static_cast<UINT>(data->renderTargetSize.y),
            DXGI_FORMAT_B8G8R8A8_UNORM,
            0
            );
        if (FAILED(result)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, result);
            return result;
        }
    }
    else
    {
        // Otherwise, create a new one using the same adapter as the existing Direct3D device.
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
        swapChainDesc.Width = static_cast<UINT>(data->renderTargetSize.x); // Match the size of the window.
        swapChainDesc.Height = static_cast<UINT>(data->renderTargetSize.y);
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
        result = data->d3dDevice.As(&dxgiDevice);
        if (FAILED(result)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, result);
            return result;
        }

        ComPtr<IDXGIAdapter> dxgiAdapter;
        result = dxgiDevice->GetAdapter(&dxgiAdapter);
        if (FAILED(result)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, result);
            return result;
        }

        ComPtr<IDXGIFactory2> dxgiFactory;
        result = dxgiAdapter->GetParent(
            __uuidof(IDXGIFactory2), 
            &dxgiFactory
            );
        if (FAILED(result)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, result);
            return result;
        }

        result = dxgiFactory->CreateSwapChainForCoreWindow(
            data->d3dDevice.Get(),
            reinterpret_cast<IUnknown*>(coreWindow),
            &swapChainDesc,
            nullptr, // Allow on all displays.
            &data->swapChain
            );
        if (FAILED(result)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, result);
            return result;
        }
            
        // Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
        // ensures that the application will only render after each VSync, minimizing power consumption.
        result = dxgiDevice->SetMaximumFrameLatency(1);
        if (FAILED(result)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, result);
            return result;
        }
    }
    
    // Set the proper orientation for the swap chain, and generate the
    // 3D matrix transformation for rendering to the rotated swap chain.
    DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;
    switch (data->orientation)
    {
        case DisplayOrientations::Landscape:
            rotation = DXGI_MODE_ROTATION_IDENTITY;
            data->vertexShaderConstantsData.projection = XMFLOAT4X4( // 0-degree Z-rotation
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
                );
            break;

        case DisplayOrientations::Portrait:
            rotation = DXGI_MODE_ROTATION_ROTATE270;
            data->vertexShaderConstantsData.projection = XMFLOAT4X4( // 270-degree Z-rotation
                0.0f, -1.0f, 0.0f, 0.0f,
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
                );
            break;

        case DisplayOrientations::LandscapeFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE180;
            data->vertexShaderConstantsData.projection = XMFLOAT4X4( // 180-degree Z-rotation
                -1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, -1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
                );
            break;

        case DisplayOrientations::PortraitFlipped:
            rotation = DXGI_MODE_ROTATION_ROTATE90;
            data->vertexShaderConstantsData.projection = XMFLOAT4X4( // 90-degree Z-rotation
                0.0f, 1.0f, 0.0f, 0.0f,
                -1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 1.0f
                );
            break;

        default:
            throw ref new Platform::FailureException();
    }

#if WINAPI_FAMILY != WINAPI_FAMILY_PHONE_APP
    // TODO, WinRT: Windows Phone does not have the IDXGISwapChain1::SetRotation method.  Check if an alternative is available, or needed.
    result = data->swapChain->SetRotation(rotation);
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }
#endif

    //
    // Update the view matrix
    //
    XMStoreFloat4x4(&data->vertexShaderConstantsData.view,  // (4)
        XMMatrixMultiply(
            XMMatrixScaling(2.0f / windowWidth, 2.0f / windowHeight, 1.0f),
            XMMatrixMultiply(
                XMMatrixTranslation(-1, -1, 0),
                XMMatrixRotationX(XM_PI)
                )));

    // Create a render target view of the swap chain back buffer.
    ComPtr<ID3D11Texture2D> backBuffer;
    result = data->swapChain->GetBuffer(
        0,
        __uuidof(ID3D11Texture2D),
        &backBuffer
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    result = data->d3dDevice->CreateRenderTargetView(
        backBuffer.Get(),
        nullptr,
        &data->renderTargetView
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    // Create a depth stencil view.
    CD3D11_TEXTURE2D_DESC depthStencilDesc(
        DXGI_FORMAT_D24_UNORM_S8_UINT, 
        static_cast<UINT>(data->renderTargetSize.x),
        static_cast<UINT>(data->renderTargetSize.y),
        1,
        1,
        D3D11_BIND_DEPTH_STENCIL
        );

    ComPtr<ID3D11Texture2D> depthStencil;
    result = data->d3dDevice->CreateTexture2D(
        &depthStencilDesc,
        nullptr,
        &depthStencil
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    // Set the rendering viewport to target the entire window.
    CD3D11_VIEWPORT viewport(
        0.0f,
        0.0f,
        data->renderTargetSize.x,
        data->renderTargetSize.y
        );

    data->d3dContext->RSSetViewports(1, &viewport);

    return S_OK;
}

// This method is called when the window's size changes.
HRESULT
D3D11_UpdateForWindowSizeChange(SDL_Renderer * renderer)
{
    D3D11_RenderData *data = (D3D11_RenderData *) renderer->driverdata;
    HRESULT result = S_OK;
    Windows::UI::Core::CoreWindow ^ coreWindow = D3D11_GetCoreWindowFromSDLRenderer(renderer);

    if (coreWindow->Bounds.Width  != data->windowSizeInDIPs.x ||
        coreWindow->Bounds.Height != data->windowSizeInDIPs.y ||
        data->orientation != DisplayProperties::CurrentOrientation)
    {
        ID3D11RenderTargetView* nullViews[] = {nullptr};
        data->d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
        data->renderTargetView = nullptr;
        data->d3dContext->Flush();
        result = D3D11_CreateWindowSizeDependentResources(renderer);
        if (FAILED(result)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, result);
            return result;
        }
    }

    return S_OK;
}

HRESULT
D3D11_HandleDeviceLost(SDL_Renderer * renderer)
{
    D3D11_RenderData *data = (D3D11_RenderData *) renderer->driverdata;
    HRESULT result = S_OK;

    // Reset these member variables to ensure that D3D11_UpdateForWindowSizeChange recreates all resources.
    data->windowSizeInDIPs.x = 0;
    data->windowSizeInDIPs.y = 0;
    data->swapChain = nullptr;

    result = D3D11_CreateDeviceResources(renderer);
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    result = D3D11_UpdateForWindowSizeChange(renderer);
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return result;
    }

    return S_OK;
}

static void
D3D11_WindowEvent(SDL_Renderer * renderer, const SDL_WindowEvent *event)
{
    //D3D11_RenderData *data = (D3D11_RenderData *) renderer->driverdata;

    if (event->event == SDL_WINDOWEVENT_RESIZED) {
        D3D11_UpdateForWindowSizeChange(renderer);
    }
}

static int
D3D11_CreateTexture(SDL_Renderer * renderer, SDL_Texture * texture)
{
    D3D11_RenderData *rendererData = (D3D11_RenderData *) renderer->driverdata;
    D3D11_TextureData *textureData;
    HRESULT result;

    textureData = new D3D11_TextureData;
    if (!textureData) {
        SDL_OutOfMemory();
        return -1;
    }
    textureData->pixelFormat = SDL_AllocFormat(texture->format);

    texture->driverdata = textureData;

    const int pixelSizeInBytes = textureData->pixelFormat->BytesPerPixel;

    D3D11_TEXTURE2D_DESC textureDesc = {0};
    textureDesc.Width = texture->w;
    textureDesc.Height = texture->h;
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
    result = rendererData->d3dDevice->CreateTexture2D(
        &textureDesc,
        &initialTextureData,
        &textureData->mainTexture
        );
    if (FAILED(result)) {
        D3D11_DestroyTexture(renderer, texture);
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return -1;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
    resourceViewDesc.Format = textureDesc.Format;
    resourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    resourceViewDesc.Texture2D.MostDetailedMip = 0;
    resourceViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
    result = rendererData->d3dDevice->CreateShaderResourceView(
        textureData->mainTexture.Get(),
        &resourceViewDesc,
        &textureData->mainTextureResourceView
        );
    if (FAILED(result)) {
        D3D11_DestroyTexture(renderer, texture);
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return -1;
    }

    return 0;
}

static void
D3D11_DestroyTexture(SDL_Renderer * renderer,
                     SDL_Texture * texture)
{
    D3D11_TextureData *textureData = (D3D11_TextureData *) texture->driverdata;

    if (textureData) {
        if (textureData->pixelFormat) {
            SDL_FreeFormat(textureData->pixelFormat);
            textureData->pixelFormat = NULL;
        }

        delete textureData;
        texture->driverdata = NULL;
    }
}

static int
D3D11_UpdateTexture(SDL_Renderer * renderer, SDL_Texture * texture,
                    const SDL_Rect * rect, const void *pixels,
                    int pitch)
{
    D3D11_RenderData *rendererData = (D3D11_RenderData *) renderer->driverdata;
    D3D11_TextureData *textureData = (D3D11_TextureData *) texture->driverdata;
    HRESULT result = S_OK;

    D3D11_MAPPED_SUBRESOURCE textureMemory = {0};
    result = rendererData->d3dContext->Map(
        textureData->mainTexture.Get(),
        0,
        D3D11_MAP_WRITE_DISCARD,
        0,
        &textureMemory
        );
    if (FAILED(result)) {
        WIN_SetErrorFromHRESULT(__FUNCTION__, result);
        return -1;
    }

    // Copy pixel data to the locked texture's memory:
    for (int y = 0; y < rect->h; ++y) {
        memcpy(
            ((Uint8 *)textureMemory.pData) + (textureMemory.RowPitch * y),
            ((Uint8 *)pixels) + (pitch * y),
            pitch
            );
    }

    // Clean up a bit, then commit the texture's memory back to Direct3D:
    rendererData->d3dContext->Unmap(
        textureData->mainTexture.Get(),
        0);

    return 0;
}

static int
D3D11_UpdateViewport(SDL_Renderer * renderer)
{
    return 0;
}

static int
D3D11_RenderClear(SDL_Renderer * renderer)
{
    D3D11_RenderData *data = (D3D11_RenderData *) renderer->driverdata;
    const float colorRGBA[] = {
        (renderer->r / 255.0f),
        (renderer->g / 255.0f),
        (renderer->b / 255.0f),
        (renderer->a / 255.0f)
    };
    data->d3dContext->ClearRenderTargetView(
        data->renderTargetView.Get(),
        colorRGBA
        );
    return 0;
}

static int D3D11_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                            const SDL_Rect * srcrect, const SDL_FRect * dstrect)
{
    D3D11_RenderData *rendererData = (D3D11_RenderData *) renderer->driverdata;
    D3D11_TextureData *textureData = (D3D11_TextureData *) texture->driverdata;
    HRESULT result = S_OK;

    rendererData->d3dContext->OMSetRenderTargets(
        1,
        rendererData->renderTargetView.GetAddressOf(),
        nullptr
        );

    rendererData->d3dContext->UpdateSubresource(
        rendererData->vertexShaderConstants.Get(),
		0,
		NULL,
		&rendererData->vertexShaderConstantsData,
		0,
		0
		);

    //
    // Create or update the vertex buffer:
    //

    // WinRT, TODO: get srcrect working in tandem with SDL_RenderCopy, etc.
    //SDL_FRect fSrcRect;
    //fSrcRect.x = (float)srcrect->x;
    //fSrcRect.y = (float)srcrect->y;
    //fSrcRect.w = (float)srcrect->w;
    //fSrcRect.h = (float)srcrect->h;

    VertexPositionColor vertices[] =
    {
        {XMFLOAT3(dstrect->x, dstrect->y, 0.0f),                           XMFLOAT2(0.0f, 0.0f)},
        {XMFLOAT3(dstrect->x, dstrect->y + dstrect->h, 0.0f),              XMFLOAT2(0.0f, 1.0f)},
        {XMFLOAT3(dstrect->x + dstrect->h, dstrect->y, 0.0f),              XMFLOAT2(1.0f, 0.0f)},
        {XMFLOAT3(dstrect->x + dstrect->h, dstrect->y + dstrect->h, 0.0f), XMFLOAT2(1.0f, 1.0f)},
    };

    if (rendererData->vertexBuffer) {
        rendererData->d3dContext->UpdateSubresource(rendererData->vertexBuffer.Get(), 0, NULL, vertices, sizeof(vertices), 0);
    } else {
        D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
        vertexBufferData.pSysMem = vertices;
        vertexBufferData.SysMemPitch = 0;
        vertexBufferData.SysMemSlicePitch = 0;
        CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(vertices), D3D11_BIND_VERTEX_BUFFER);
        result = rendererData->d3dDevice->CreateBuffer(
            &vertexBufferDesc,
            &vertexBufferData,
            &rendererData->vertexBuffer
            );
        if (FAILED(result)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, result);
            return -1;
        }
    }

    UINT stride = sizeof(VertexPositionColor);
    UINT offset = 0;
    rendererData->d3dContext->IASetVertexBuffers(
        0,
        1,
        rendererData->vertexBuffer.GetAddressOf(),
        &stride,
        &offset
        );

    rendererData->d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    rendererData->d3dContext->IASetInputLayout(rendererData->inputLayout.Get());

    rendererData->d3dContext->VSSetShader(
        rendererData->vertexShader.Get(),
        nullptr,
        0
        );

    rendererData->d3dContext->VSSetConstantBuffers(
		0,
		1,
        rendererData->vertexShaderConstants.GetAddressOf()
		);

    rendererData->d3dContext->PSSetShader(
        rendererData->pixelShader.Get(),
        nullptr,
        0
        );

    rendererData->d3dContext->PSSetShaderResources(0, 1, textureData->mainTextureResourceView.GetAddressOf());

    rendererData->d3dContext->PSSetSamplers(0, 1, rendererData->mainSampler.GetAddressOf());

    rendererData->d3dContext->RSSetState(rendererData->mainRasterizer.Get());

    rendererData->d3dContext->Draw(4, 0);

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
    //
    // TODO, WinRT: consider throwing an exception if D3D11_RenderPresent fails, especially if there is a way to salvedge debug info from users' machines
    if (hr == DXGI_ERROR_DEVICE_REMOVED)
    {
        hr = D3D11_HandleDeviceLost(renderer);
        if (FAILED(hr)) {
            WIN_SetErrorFromHRESULT(__FUNCTION__, hr);
        }
    }
    else
    {
        WIN_SetErrorFromHRESULT(__FUNCTION__, hr);
    }
}

#endif /* SDL_VIDEO_RENDER_D3D && !SDL_RENDER_DISABLED */

/* vi: set ts=4 sw=4 expandtab: */
