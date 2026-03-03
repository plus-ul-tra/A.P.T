#include "RenderTargetContext.h"

bool RenderTargetContext::Create(UINT width, UINT height, DXGI_FORMAT colorFormat, DXGI_FORMAT depthFormat)
{
	if (width == 0 || height == 0)
	{
		return false;
	}

	Reset();

	m_Width = width;
	m_Height = height;
	m_ColorFormat = colorFormat;
	m_DepthFormat = depthFormat;

	ID3D11Texture2D* colorTex = nullptr;
	if (FAILED( RTTexCreate(m_pDevice.Get(), width, height, colorFormat, &colorTex)))
	{
		return false;
	}
	m_ColorTexture.Attach(colorTex);

	if (FAILED(RTViewCreate(m_pDevice.Get(), colorFormat, m_ColorTexture.Get(), m_RenderTargetView.GetAddressOf())))
	{
		Reset();
		return false;
	}

	if (FAILED(RTSRViewCreate(m_pDevice.Get(), colorFormat, m_ColorTexture.Get(), m_ShaderResourceView.GetAddressOf())))
	{
		Reset();
		return false;
	}

	ID3D11Texture2D* depthTex = nullptr;
	ID3D11DepthStencilView* dsv = nullptr;

	if (FAILED(DSCreate(m_pDevice.Get(), width, height, depthFormat, &depthTex, &dsv)))
	{
		Reset();
		return false;
	}
	m_DepthTexture.Attach(depthTex);
	m_DepthStencilView.Attach(dsv);

	return true;
}

bool RenderTargetContext::Resize(UINT width, UINT height)
{
	if (width == 0 || height == 0)
	{
		return false;
	}

	if (width == m_Width && height == m_Height && IsValid())
	{
		return true;
	}

	return Create(width, height, m_ColorFormat, m_DepthFormat);
}

void RenderTargetContext::Reset()
{
	m_ColorTexture.Reset();
	m_RenderTargetView.Reset();
	m_ShaderResourceView.Reset();
	m_DepthTexture.Reset();
	m_DepthStencilView.Reset();
	m_Width = 0;
	m_Height = 0;
}

void RenderTargetContext::Bind() const
{
	if (!IsValid())
	{
		return;
	}

	ID3D11RenderTargetView* rtvs[] = { m_RenderTargetView.Get() };
	m_pDXDC->OMSetRenderTargets(1, rtvs, m_DepthStencilView.Get());
	SetViewPort(static_cast<int>(m_Width), static_cast<int>(m_Height), m_pDXDC.Get());
}

void RenderTargetContext::Clear(const COLOR& color, float depth, UINT stencil) const
{
	if (!IsValid())
	{
		return;
	}

	m_pDXDC->ClearRenderTargetView(m_RenderTargetView.Get(), reinterpret_cast<const float*>(&color));
	m_pDXDC->ClearDepthStencilView(m_DepthStencilView.Get(), D3D11_CLEAR_DEPTH, depth, stencil);
}
