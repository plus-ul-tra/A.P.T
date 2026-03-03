#pragma once

#include <wrl/client.h>
#include "DX11.h"

using Microsoft::WRL::ComPtr;

class RenderTargetContext
{
public:
	RenderTargetContext() = default;
	~RenderTargetContext() = default;

	bool Create(UINT width, UINT height, DXGI_FORMAT colorFormat = DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT depthFormat = DXGI_FORMAT_D32_FLOAT);
	bool Resize(UINT width, UINT height);
	void Reset();

	void Bind() const;
	void Clear(const COLOR& color, float depth = 1.0f, UINT stencil = 0) const;

	bool IsValid() const { return m_RenderTargetView != nullptr; }
	UINT GetWidth() const { return m_Width; }
	UINT GetHeight() const { return m_Height; }

	ID3D11ShaderResourceView* GetShaderResourceView() const { return m_ShaderResourceView.Get(); }
	void SetShaderResourceView(ID3D11ShaderResourceView* view) { m_ShaderResourceView = view; }

	void SetDevice(ID3D11Device* device, ID3D11DeviceContext* dxdc) { m_pDevice = device; m_pDXDC = dxdc; }
private:
	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_ColorFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT m_DepthFormat = DXGI_FORMAT_D32_FLOAT;

	ComPtr<ID3D11Texture2D> m_ColorTexture;
	ComPtr<ID3D11RenderTargetView> m_RenderTargetView;
	ComPtr<ID3D11ShaderResourceView> m_ShaderResourceView;
	ComPtr<ID3D11Texture2D> m_DepthTexture;
	ComPtr<ID3D11DepthStencilView> m_DepthStencilView;

	ComPtr<ID3D11Device>		m_pDevice;
	ComPtr<ID3D11DeviceContext> m_pDXDC;
};
