#pragma once

#include "Direct3DBase.h"

struct VertexPositionColor
{
	DirectX::XMFLOAT3 pos;
	DirectX::XMFLOAT2 tex;
};

// This class renders a simple spinning cube.
ref class SDL_winrtrenderer sealed : public Direct3DBase
{
public:
	SDL_winrtrenderer();

	// Direct3DBase methods.
	virtual void CreateDeviceResources() override;

internal:
	virtual void Render(SDL_Surface * surface, SDL_Rect * rects, int numrects) override;
    void ResizeMainTexture(int w, int h);

private:
	bool m_loadingComplete;

	Microsoft::WRL::ComPtr<ID3D11InputLayout> m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader> m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader> m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_mainTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_mainTextureResourceView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_mainSampler;

	uint32 m_vertexCount;
};
