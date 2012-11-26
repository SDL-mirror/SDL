#pragma once

#include "DirectXHelper.h"
#include "SDL.h"

struct VertexPositionColor
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 tex;
};

// Helper class that initializes DirectX APIs for 3D rendering.
ref class SDL_winrtrenderer
{
internal:
	SDL_winrtrenderer();

public:
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

protected private:
	// Direct3D Objects.
	Microsoft::WRL::ComPtr<ID3D11Device1> m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext1> m_d3dContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_mainTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_mainTextureResourceView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_mainSampler;

	// Cached renderer properties.
	D3D_FEATURE_LEVEL m_featureLevel;
	Windows::Foundation::Size m_renderTargetSize;
	Windows::Foundation::Rect m_windowBounds;
	Platform::Agile<Windows::UI::Core::CoreWindow> m_window;
	Windows::Graphics::Display::DisplayOrientations m_orientation;
	uint32 m_vertexCount;

	// Transform used for display orientation.
	DirectX::XMFLOAT4X4 m_orientationTransform3D;

    // Has the renderer finished loading?
    bool m_loadingComplete;
};
