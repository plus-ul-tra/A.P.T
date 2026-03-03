#include "UIPass.h"

void UIPass::Execute(const RenderData::FrameData & frame)
{
    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
    ID3D11ShaderResourceView* nullSRVs[128] = { nullptr };
    dxdc->PSSetShaderResources(0, 128, nullSRVs);
	FLOAT backcolor[4] = { 0.21f, 0.21f, 0.21f, 1.0f };
    //SetRenderTarget(m_RenderContext.pRTView_Post.Get(), nullptr, backcolor);
    //SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());
    SetBlendState(BS::ALPHABLEND_WALL);
    SetRasterizerState(RS::SOLID);
    SetDepthStencilState(DS::DEPTH_OFF);
    SetSamplerState();
#pragma endregion

	const auto& context = frame.context;

	const auto& uiElements = frame.uiElements;
	if (uiElements.empty())
	{
		return;
	}

	XMMATRIX tm = XMMatrixIdentity();
	XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, tm);
	XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, tm);

	XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 0.0f, 1.0f);

	XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, mProj);
	XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, mProj);
	UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));


	//현재는 depthpass에서 먼저 그려주기 때문에 여기서 지워버리면 안된다. 지울 위치를 잘 찾아보자
	//ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);

	ID3D11Buffer* vb = m_RenderContext.UIQuadVertexBuffer.Get();
	ID3D11Buffer* ib = m_RenderContext.UIQuadIndexBuffer.Get();
	if (!vb || !ib)
	{
		return;
	}
	const UINT stride = sizeof(RenderData::Vertex);
	const UINT offset = 0;

	m_RenderContext.pDXDC->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	m_RenderContext.pDXDC->IASetInputLayout(m_RenderContext.InputLayout.Get());
	m_RenderContext.pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_RenderContext.pDXDC->VSSetShader(m_RenderContext.VS_UI.Get(), nullptr, 0);
	m_RenderContext.pDXDC->PSSetShader(m_RenderContext.PS_UI.Get(), nullptr, 0);
	m_RenderContext.pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	m_RenderContext.pDXDC->VSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	m_RenderContext.pDXDC->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);

	for (const auto& element : uiElements)
	{
		if (element.size.x <= 0.0f || element.size.y <= 0.0f)
		{
			continue;
		}

		const float halfW = element.size.x * 0.5f;
		const float halfH = element.size.y * 0.5f;
		XMMATRIX scale = XMMatrixScaling(halfW, halfH, 1.0f);
		XMMATRIX rotation = XMMatrixRotationZ(XMConvertToRadians(element.rotation));
		XMMATRIX translation = XMMatrixTranslation(element.position.x + halfW, element.position.y + halfH, 0.0f);
		XMMATRIX world = scale * rotation * translation;
		XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, world);

		XMFLOAT4X4 tint{};
		tint._11 = element.color.x;
		tint._12 = element.color.y;
		tint._13 = element.color.z;
		tint._14 = element.color.w * element.opacity;
		tint._21 = element.progress;
		tint._22 = element.progressDirection;
		m_RenderContext.BCBuffer.mTextureMask = tint;

		UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));

		ID3D11VertexShader* vertexShader = m_RenderContext.VS_UI.Get();
		ID3D11PixelShader* pixelShader = m_RenderContext.PS_UI.Get();
		if (element.useMaterialOverrides)
		{
			VertexShaderHandle vertexShaderHandle = VertexShaderHandle::Invalid();
			PixelShaderHandle pixelShaderHandle = PixelShaderHandle::Invalid();

			if (element.materialOverrides.shaderAsset.IsValid())
			{
				const auto& shaderAssets = m_AssetLoader.GetShaderAssets();
				if (const auto* asset = shaderAssets.Get(element.materialOverrides.shaderAsset))
				{
					vertexShaderHandle = asset->vertexShader;
					pixelShaderHandle = asset->pixelShader;
				}
			}

			if (element.materialOverrides.vertexShader.IsValid())
			{
				vertexShaderHandle = element.materialOverrides.vertexShader;
			}
			if (element.materialOverrides.pixelShader.IsValid())
			{
				pixelShaderHandle = element.materialOverrides.pixelShader;
			}

			if (vertexShaderHandle.IsValid() && m_RenderContext.vertexShaders)
			{
				auto it = m_RenderContext.vertexShaders->find(vertexShaderHandle);
				if (it != m_RenderContext.vertexShaders->end())
				{
					vertexShader = it->second.vertexShader.Get();
				}
			}

			if (pixelShaderHandle.IsValid() && m_RenderContext.pixelShaders)
			{
				auto it = m_RenderContext.pixelShaders->find(pixelShaderHandle);
				if (it != m_RenderContext.pixelShaders->end())
				{
					pixelShader = it->second.pixelShader.Get();
				}
			}
		}

		m_RenderContext.pDXDC->VSSetShader(vertexShader, nullptr, 0);
		m_RenderContext.pDXDC->PSSetShader(pixelShader, nullptr, 0);

		TextureHandle textureHandle = TextureHandle::Invalid();
		if (element.useMaterialOverrides && element.materialOverrides.textureHandle.IsValid())
		{
			textureHandle = element.materialOverrides.textureHandle;
		}

		ID3D11ShaderResourceView* srv = nullptr;
		if (textureHandle.IsValid() && m_RenderContext.textures)
		{
			auto it = m_RenderContext.textures->find(textureHandle);
			if (it != m_RenderContext.textures->end())
			{
				srv = it->second.Get();
			}
		}
		if (!srv)
		{
			srv = m_RenderContext.UIWhiteTexture.Get();
		}

		TextureHandle maskHandle = element.maskTextureHandle;
		ID3D11ShaderResourceView* maskSrv = nullptr;
		if (maskHandle.IsValid() && m_RenderContext.textures)
		{
			auto itMask = m_RenderContext.textures->find(maskHandle);
			if (itMask != m_RenderContext.textures->end())
			{
				maskSrv = itMask->second.Get();
			}
		}
		if (!maskSrv)
		{
			maskSrv = m_RenderContext.UIWhiteTexture.Get();
		}

		ID3D11ShaderResourceView* srvs[2] = { srv, maskSrv };

		m_RenderContext.pDXDC->PSSetShaderResources(21, 2, srvs);
		m_RenderContext.pDXDC->PSSetSamplers(0, 1, m_RenderContext.SState[SS::CLAMP].GetAddressOf());
		m_RenderContext.pDXDC->DrawIndexed(m_RenderContext.UIQuadIndexCount, 0, 0);
	}
}
