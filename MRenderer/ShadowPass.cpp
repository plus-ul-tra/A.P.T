#include "ShadowPass.h"

#include <algorithm>
#include <cctype>
#include <string_view>

namespace
{
	bool ContainsCaseInsensitive(std::string_view text, std::string_view pattern)
	{
		if (pattern.empty() || text.size() < pattern.size())
		{
			return false;
		}

		return std::search(text.begin(), text.end(), pattern.begin(), pattern.end(),
			[](char a, char b)
			{
				return std::tolower(static_cast<unsigned char>(a)) == std::tolower(static_cast<unsigned char>(b));
			}) != text.end();
	}
}

bool ShadowPass::ShouldIncludeRenderItem(RenderData::RenderLayer layer, const RenderData::RenderItem& item) const
{
	if (layer != RenderData::OpaqueItems && layer != RenderData::WallItems)
	{
		return false;
	}

	if (!item.mesh.IsValid())
	{
		return true;
	}

	const auto& meshes = m_AssetLoader.GetMeshes();
	const std::string* meshName = meshes.GetDisplayName(item.mesh);
	const std::string* meshKey = meshes.GetKey(item.mesh);
	const bool isTile = (meshName && ContainsCaseInsensitive(*meshName, "Tile")) ||
		(meshKey && ContainsCaseInsensitive(*meshKey, "Tile"));

	return !isTile;
}


void ShadowPass::Execute(const RenderData::FrameData& frame)
{
	ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
	ID3D11ShaderResourceView* nullSRVs[128] = { nullptr };
	dxdc->PSSetShaderResources(0, 128, nullSRVs);

	FLOAT backcolor[4] = { 0.f, 0.f, 0.f, 1.0f };
	SetRenderTarget(nullptr, m_RenderContext.pDSViewScene_Shadow.Get(), backcolor);
	SetViewPort(m_RenderContext.ShadowTextureSize.width, m_RenderContext.ShadowTextureSize.height, m_RenderContext.pDXDC.Get());
	SetBlendState(BS::DEFAULT);
	SetRasterizerState(RS::CULLBACK);
	SetDepthStencilState(DS::DEPTH_ON);

#pragma endregion
	const auto& context = frame.context;
	if (frame.lights.empty())
		return;

	for (const auto& light : frame.lights)
	{
		if (light.type != RenderData::LightType::Directional)
			continue;
		const auto& mainlight = light;

		//0번이 전역광이라고 가정...
		XMMATRIX lightview, lightproj;
		XMVECTOR playerpos = XMLoadFloat3(&frame.playerPosition);
		XMVECTOR dir = XMVector3Normalize(XMLoadFloat3(&mainlight.direction));
		constexpr float kShadowDistance = 80.0f;
		constexpr float kShadowWidth = 128.0f;
		constexpr float kShadowHeight = 128.0f;
		constexpr float kShadowNear = 0.1f;
		constexpr float kShadowFar = 200.0f;

		XMVECTOR pos = playerpos - (dir * kShadowDistance);
		XMVECTOR look = playerpos;

		XMVECTOR up = XMVectorSet(0, 1, 0, 0);
		if (fabsf(XMVectorGetX(XMVector3Dot(dir, up))) > 0.99f)
		{
			up = XMVectorSet(0, 0, 1, 0);
		}

		if (XMVector4Equal(pos, look)) return;

		lightview = XMMatrixLookAtLH(pos, look, up);

		const XMVECTOR playerInLightSpace = XMVector3TransformCoord(playerpos, lightview);
		const float centerX = XMVectorGetX(playerInLightSpace);
		const float centerY = XMVectorGetY(playerInLightSpace);
		const float halfW = kShadowWidth * 0.5f;
		const float halfH = kShadowHeight * 0.5f;

		// 플레이어를 그림자 버퍼 중심에 고정
		lightproj = XMMatrixOrthographicOffCenterLH(
			centerX - halfW,
			centerX + halfW,
			centerY - halfH,
			centerY + halfH,
			kShadowNear,
			kShadowFar);


		//텍스처 좌표 변환
		XMFLOAT4X4 m = {
			0.5f,  0.0f, 0.0f, 0.0f,
			 0.0f, -0.5f, 0.0f, 0.0f,
			 0.0f,  0.0f, 1.0f, 0.0f,
			 0.5f,  0.5f, 0.0f, 1.0f
		};

		XMMATRIX mscale = XMLoadFloat4x4(&m);
		XMMATRIX mLightTM;
		mLightTM = lightview * lightproj * mscale;

		XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, lightview);
		XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, lightproj);
		XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, lightview * lightproj);

		//★ 나중에 그림자 매핑용 행렬 위치 정해지면 상수 버퍼 set
		XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mShadow, mLightTM);

		UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));

		break;
	}



	for (const auto& queueItem : GetQueue())
	{
		const auto& item = *queueItem.item;
		SetBaseCB(item);
		if (m_RenderContext.pSkinCB && item.skinningPaletteCount > 0)
		{
			const size_t paletteStart = item.skinningPaletteOffset;
			const size_t paletteCount = item.skinningPaletteCount;
			const size_t paletteSize = frame.skinningPalettes.size();
			const size_t maxCount = min(static_cast<size_t>(kMaxSkinningBones), paletteCount);
			const size_t safeCount = (paletteStart + maxCount <= paletteSize) ? maxCount : (paletteSize > paletteStart ? paletteSize - paletteStart : 0);

			m_RenderContext.SkinCBuffer.boneCount = static_cast<UINT>(safeCount);
			for (size_t i = 0; i < safeCount; ++i)
			{
				m_RenderContext.SkinCBuffer.bones[i] = frame.skinningPalettes[paletteStart + i];
			}
			UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pSkinCB.Get(), &m_RenderContext.SkinCBuffer, sizeof(SkinningConstBuffer));

			m_RenderContext.pDXDC->VSSetConstantBuffers(3, 1, m_RenderContext.pSkinCB.GetAddressOf());
		}
		else if (m_RenderContext.pSkinCB)
		{
			m_RenderContext.SkinCBuffer.boneCount = 0;
			UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pSkinCB.Get(), &m_RenderContext.SkinCBuffer, sizeof(SkinningConstBuffer));
			m_RenderContext.pDXDC->VSSetConstantBuffers(3, 1, m_RenderContext.pSkinCB.GetAddressOf());
		}


		ID3D11VertexShader* vertexShader = m_RenderContext.VS_MakeShadow.Get();
		ID3D11PixelShader* pixelShader = m_RenderContext.PS_MakeShadow_Transparent.Get();



		const RenderData::MaterialData* mat = nullptr;
		if (item.useMaterialOverrides)
		{
			mat = &item.materialOverrides;
		}
		else if (item.material.IsValid())
		{
			mat = m_AssetLoader.GetMaterials().Get(item.material);
		}

		const auto* textures = m_RenderContext.textures;
		if (textures && mat)
		{
			for (UINT slot = 0; slot < static_cast<UINT>(RenderData::MaterialTextureSlot::Albedo) + 1; ++slot)
			{
				const TextureHandle h = mat->textures[slot];
				if (!h.IsValid())
					continue;

				const auto tIt = textures->find(h);
				if (tIt == textures->end())
					continue;

				ID3D11ShaderResourceView* srv = tIt->second.Get();


				m_RenderContext.pDXDC->PSSetShaderResources(11 + slot, 1, &srv);
			}


		}


		const auto* vertexBuffers = m_RenderContext.vertexBuffers;
		const auto* indexBuffers = m_RenderContext.indexBuffers;
		const auto* indexcounts = m_RenderContext.indexCounts;

		if (vertexBuffers && indexBuffers && indexcounts && item.mesh.IsValid())
		{
			const MeshHandle bufferHandle = item.mesh;
			const auto vbIt = vertexBuffers->find(bufferHandle);
			const auto ibIt = indexBuffers->find(bufferHandle);
			const auto countIt = indexcounts->find(bufferHandle);
			if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexcounts->end())
			{
				ID3D11Buffer* vb = vbIt->second.Get();
				ID3D11Buffer* ib = ibIt->second.Get();
				const bool useSubMesh = item.useSubMesh;
				const UINT32 indexCount = useSubMesh ? item.indexCount : countIt->second;
				const UINT32 indexStart = useSubMesh ? item.indexStart : 0;
				if (vb && ib)
				{
					DrawMesh(vb, ib, vertexShader, pixelShader, useSubMesh, indexCount, indexStart);
				}
			}
		}
	}
}
