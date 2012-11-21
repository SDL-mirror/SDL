#include "SDLmain_WinRT_common.h"
#include "SDL_winrtrenderer.h"

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;

SDL_winrtrenderer::SDL_winrtrenderer() :
	m_loadingComplete(false),
	m_vertexCount(0)
{
}

void SDL_winrtrenderer::CreateDeviceResources()
{
	Direct3DBase::CreateDeviceResources();

	auto loadVSTask = DX::ReadDataAsync("SDL_VS2012_WinRT\\SimpleVertexShader.cso");
	auto loadPSTask = DX::ReadDataAsync("SDL_VS2012_WinRT\\SimplePixelShader.cso");

	auto createVSTask = loadVSTask.then([this](Platform::Array<byte>^ fileData) {
		DX::ThrowIfFailed(
			m_d3dDevice->CreateVertexShader(
 				fileData->Data,
				fileData->Length,
				nullptr,
				&m_vertexShader
				)
			);

		const D3D11_INPUT_ELEMENT_DESC vertexDesc[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		};

		DX::ThrowIfFailed(
			m_d3dDevice->CreateInputLayout(
				vertexDesc,
				ARRAYSIZE(vertexDesc),
				fileData->Data,
				fileData->Length,
				&m_inputLayout
				)
			);
	});

	auto createPSTask = loadPSTask.then([this](Platform::Array<byte>^ fileData) {
		DX::ThrowIfFailed(
			m_d3dDevice->CreatePixelShader(
				fileData->Data,
				fileData->Length,
				nullptr,
				&m_pixelShader
				)
			);
	});

	auto createCubeTask = (createPSTask && createVSTask).then([this] () {
		VertexPositionColor cubeVertices[] = 
		{
			{XMFLOAT3(-1.0f, -1.0f, 0.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.0f, 0.0f)},
			{XMFLOAT3(-1.0f, 1.0f, 0.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f)},
			{XMFLOAT3(1.0f, -1.0f, 0.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.0f, 0.0f)},
			{XMFLOAT3(1.0f, 1.0f, 0.0f), XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT2(1.0f, 1.0f)},
		};

		m_vertexCount = ARRAYSIZE(cubeVertices);

		D3D11_SUBRESOURCE_DATA vertexBufferData = {0};
		vertexBufferData.pSysMem = cubeVertices;
		vertexBufferData.SysMemPitch = 0;
		vertexBufferData.SysMemSlicePitch = 0;
		CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(cubeVertices), D3D11_BIND_VERTEX_BUFFER);
		DX::ThrowIfFailed(
			m_d3dDevice->CreateBuffer(
				&vertexBufferDesc,
				&vertexBufferData,
				&m_vertexBuffer
				)
			);
	});

	auto createMainTextureTask = createCubeTask.then([this] () {
		D3D11_TEXTURE2D_DESC textureDesc = {0};
		textureDesc.Width = (int)m_windowBounds.Width;
		textureDesc.Height = (int)m_windowBounds.Height;
		textureDesc.MipLevels = 1;
		textureDesc.ArraySize = 1;
		textureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		textureDesc.SampleDesc.Count = 1;
		textureDesc.SampleDesc.Quality = 0;
		textureDesc.Usage = D3D11_USAGE_DYNAMIC;
		textureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		textureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		textureDesc.MiscFlags = 0;

		int numPixels = (int)m_windowBounds.Width * (int)m_windowBounds.Height;
		std::vector<uint8> initialTexturePixels(numPixels * 4);
		for (int i = 0; i < (numPixels * 4); i += 4) {
			initialTexturePixels[i+0] = 0xFF;
			initialTexturePixels[i+1] = 0x00;
			initialTexturePixels[i+2] = 0x00;
			initialTexturePixels[i+3] = 0xFF;
		}
		D3D11_SUBRESOURCE_DATA initialTextureData = {0};
		initialTextureData.pSysMem = (void *)&(initialTexturePixels[0]);
		initialTextureData.SysMemPitch = (int)m_windowBounds.Width * 4;
		initialTextureData.SysMemSlicePitch = numPixels * 4;
		DX::ThrowIfFailed(
			m_d3dDevice->CreateTexture2D(
				&textureDesc,
				&initialTextureData,
				&m_mainTexture
				)
			);

		D3D11_SHADER_RESOURCE_VIEW_DESC resourceViewDesc;
		resourceViewDesc.Format = textureDesc.Format;
		resourceViewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceViewDesc.Texture2D.MostDetailedMip = 0;
		resourceViewDesc.Texture2D.MipLevels = textureDesc.MipLevels;
		DX::ThrowIfFailed(
			m_d3dDevice->CreateShaderResourceView(
				m_mainTexture.Get(),
				&resourceViewDesc,
				&m_mainTextureResourceView)
			);
	});

	auto createMainSamplerTask = createMainTextureTask.then([this] () {
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
		DX::ThrowIfFailed(
			m_d3dDevice->CreateSamplerState(
				&samplerDesc,
				&m_mainSampler
				)
			);
	});

	createMainSamplerTask.then([this] () {
		m_loadingComplete = true;
	});
}

void SDL_winrtrenderer::Render()
{
	const float midnightBlue[] = { 0.098f, 0.098f, 0.439f, 1.000f };
	m_d3dContext->ClearRenderTargetView(
		m_renderTargetView.Get(),
		midnightBlue
		);

	m_d3dContext->ClearDepthStencilView(
		m_depthStencilView.Get(),
		D3D11_CLEAR_DEPTH,
		1.0f,
		0
		);

	// Only draw the cube once it is loaded (loading is asynchronous).
	if (!m_loadingComplete)
	{
		return;
	}

	m_d3dContext->OMSetRenderTargets(
		1,
		m_renderTargetView.GetAddressOf(),
		m_depthStencilView.Get()
		);

	UINT stride = sizeof(VertexPositionColor);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(
		0,
		1,
		m_vertexBuffer.GetAddressOf(),
		&stride,
		&offset
		);

	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	m_d3dContext->IASetInputLayout(m_inputLayout.Get());

	m_d3dContext->VSSetShader(
		m_vertexShader.Get(),
		nullptr,
		0
		);

	m_d3dContext->PSSetShader(
		m_pixelShader.Get(),
		nullptr,
		0
		);

	m_d3dContext->PSSetShaderResources(0, 1, m_mainTextureResourceView.GetAddressOf());

	m_d3dContext->PSSetSamplers(0, 1, m_mainSampler.GetAddressOf());

	m_d3dContext->Draw(4, 0);
}
