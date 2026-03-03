#include "OpaquePass.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

void OpaquePass::Execute(const RenderData::FrameData& frame)
{
	ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
	//SetRenderTarget()		아래에서 카메라에 따라 처리
	SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());
	SetBlendState(BS::DEFAULT);
	SetRasterizerState(RS::CULLBACK);
	SetDepthStencilState(DS::DEPTH_ON);
	SetSamplerState();

#pragma endregion
	FLOAT backcolor[4] = { 0.21f, 0.21f, 0.21f, 1.0f };
	ID3D11DepthStencilView* depthView = m_RenderContext.pDSViewScene_DepthMSAA
		? m_RenderContext.pDSViewScene_DepthMSAA.Get()
		: m_RenderContext.pDSViewScene_Depth.Get();
	SetCameraCB(frame);
	if (m_RenderContext.isEditCam)
	{
		SetRenderTarget(m_RenderContext.pRTView_Imgui_edit.Get(), depthView, backcolor);
	}
	else if (!m_RenderContext.isEditCam)
	{
		SetRenderTarget(m_RenderContext.pRTView_Imgui.Get(), depthView, backcolor);
	}

	//임시 스카이박스 테스트
	if (frame.currScene == 1 || frame.currScene == 2)
	{
		m_RenderContext.pDXDC->PSSetShaderResources(3, 1, m_RenderContext.pHDRI_1.GetAddressOf());
	}
	else
	{
		m_RenderContext.pDXDC->PSSetShaderResources(3, 1, m_RenderContext.SkyBox.GetAddressOf());

	}

	m_RenderContext.pDXDC->VSSetShader(m_RenderContext.VS_SkyBox.Get(), nullptr, 0);
	m_RenderContext.pDXDC->PSSetShader(m_RenderContext.PS_SkyBox.Get(), nullptr, 0);
	SetDepthStencilState(DS::DEPTH_OFF);
	m_RenderContext.DrawFullscreenQuad();
	SetDepthStencilState(DS::DEPTH_ON);


	//빛 상수 버퍼 set
	SetDirLight(frame);
	SetOtherLights(frame);

	//★이부분 에디터랑 게임 씬 크기가 다르면 이것도 if문안에 넣어야할듯


	//터레인 그리기
	if (frame.currScene != 3)
	{
		XMMATRIX mTM, mScale, mRotate, mTrans;
		mScale = XMMatrixScaling(50, 50, 1);
		mRotate = XMMatrixRotationX(XM_PI / 2);
		mTrans = XMMatrixTranslation(0.0f, -10.0f, 0.0f);
		mTM = mScale * mRotate * mTrans;
		XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, mTM);
		UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));
		dxdc->PSSetShaderResources(18, 1, m_RenderContext.WaterNoise.GetAddressOf());
		dxdc->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
		dxdc->PSSetShaderResources(2, 1, m_RenderContext.pShadowRV.GetAddressOf());
		dxdc->VSSetShader(m_RenderContext.VS_Shadow.Get(), nullptr, 0);
		dxdc->PSSetShader(m_RenderContext.PS_Shadow.Get(), nullptr, 0);
		m_RenderContext.DrawFullscreenQuad();

	}
	//터레인 끝

	//m_RenderContext.UpdateGrid(frame);
	//m_RenderContext.DrawGrid();


	//현재는 depthpass에서 먼저 그려주기 때문에 여기서 지워버리면 안된다. 지울 위치를 잘 찾아보자
	//ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);

	for (const auto& queueItem : GetQueue())
	{
		const auto& item = *queueItem.item;
		SetBaseCB(item);
#ifdef _DEBUG
// 		if (const auto* mesh = m_AssetLoader.GetMeshes().Get(item.mesh))
// 		{
// 			if (mesh->hasSkinning)
// 			{
// 				const UINT32 paletteCount = item.skinningPaletteCount;
// 				if (paletteCount == 0 || mesh->maxBoneIndex >= paletteCount)
// 				{
// 					static std::unordered_set<UINT32> warnedMeshes;
// 					if (warnedMeshes.insert(item.mesh.id).second)
// 					{
// 						std::cout << "[Skinning] meshId=" << item.mesh.id
// 							<< " maxBoneIndex=" << mesh->maxBoneIndex
// 							<< " paletteCount=" << paletteCount
// 							<< " skeletonId=" << item.skeleton.id
// 							<< "\n";
// 					}
// 				}
// 			}
// 		}

		
#endif
		if (m_RenderContext.pSkinCB && item.skinningPaletteCount > 0)
		{
			const size_t paletteStart = item.skinningPaletteOffset;
			const size_t paletteCount = item.skinningPaletteCount;
			const size_t paletteSize = frame.skinningPalettes.size();
			const size_t maxCount = min(static_cast<size_t>(kMaxSkinningBones), paletteCount);
			// Clamp
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

		const auto* vertexBuffers = m_RenderContext.vertexBuffers;
		const auto* indexBuffers = m_RenderContext.indexBuffers;
		const auto* indexCounts = m_RenderContext.indexCounts;
		const auto* textures = m_RenderContext.textures;
		const auto* vertexShaders = m_RenderContext.vertexShaders;
		const auto* pixelShaders = m_RenderContext.pixelShaders;

		ID3D11VertexShader* vertexShader = m_RenderContext.VS_PBR.Get();
		ID3D11PixelShader* pixelShader = m_RenderContext.PS_PBR.Get();

		const RenderData::MaterialData* mat = nullptr;
		if (item.useMaterialOverrides)
		{
			mat = &item.materialOverrides;
		}
		else if (item.material.IsValid())
		{
			mat = m_AssetLoader.GetMaterials().Get(item.material);
		}
		if (mat)
		{
			SetMaterialCB(*mat);
		}

		if (textures && mat)
		{

			if (mat->shaderAsset.IsValid())
			{
				const auto* shaderAsset = m_AssetLoader.GetShaderAssets().Get(mat->shaderAsset);
				if (shaderAsset)
				{
					if (vertexShaders && shaderAsset->vertexShader.IsValid())
					{
						const auto shaderIt = vertexShaders->find(shaderAsset->vertexShader);
						if (shaderIt != vertexShaders->end() && shaderIt->second.vertexShader)
						{
							vertexShader = shaderIt->second.vertexShader.Get();
						}
					}

					if (pixelShaders && shaderAsset->pixelShader.IsValid())
					{
						const auto shaderIt = pixelShaders->find(shaderAsset->pixelShader);
						if (shaderIt != pixelShaders->end() && shaderIt->second.pixelShader)
						{
							pixelShader = shaderIt->second.pixelShader.Get();
						}
					}
				}
			}

			if (vertexShaders && mat->vertexShader.IsValid())
			{
				const auto shaderIt = vertexShaders->find(mat->vertexShader);
				if (shaderIt != vertexShaders->end() && shaderIt->second.vertexShader)
				{
					vertexShader = shaderIt->second.vertexShader.Get();
				}
			}

			if (pixelShaders && mat->pixelShader.IsValid())
			{
				const auto shaderIt = pixelShaders->find(mat->pixelShader);
				if (shaderIt != pixelShaders->end() && shaderIt->second.pixelShader)
				{
					pixelShader = shaderIt->second.pixelShader.Get();
				}
			}



#pragma region TextureBinding
			for (UINT slot = 0; slot < static_cast<UINT>(RenderData::MaterialTextureSlot::TEX_MAX); ++slot)
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
			if (frame.currScene == 1)
			{
				m_RenderContext.pDXDC->PSSetShaderResources(17, 1, m_RenderContext.pHDRI_1.GetAddressOf());
			}
			else if (frame.currScene == 2)
			{
				m_RenderContext.pDXDC->PSSetShaderResources(17, 1, m_RenderContext.pHDRI_2.GetAddressOf());
			}
		}

#pragma endregion


		if (vertexBuffers && indexBuffers && indexCounts && item.mesh.IsValid())
		{
			const MeshHandle bufferHandle = item.mesh;
			const auto vbIt = vertexBuffers->find(bufferHandle);
			const auto ibIt = indexBuffers->find(bufferHandle);
			const auto countIt = indexCounts->find(bufferHandle);

			if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexCounts->end())
			{
				ID3D11Buffer* vb = vbIt->second.Get();
				ID3D11Buffer* ib = ibIt->second.Get();
				const UINT32 fullCount = countIt->second;
				const bool useSubMesh = item.useSubMesh;
				const UINT32 indexCount = useSubMesh ? item.indexCount : fullCount;
				const UINT32 indexStart = useSubMesh ? item.indexStart : 0;

				//DrawMesh(vb, ib, m_RenderContext.inputLayout.Get(), m_RenderContext.VS.Get(), m_RenderContext.PS.Get(), useSubMesh, indexCount, indexStart);
				DrawMesh(vb, ib, vertexShader, pixelShader, useSubMesh, indexCount, indexStart);

				//DrawBones(vertexShader, pixelShader, m_RenderContext.SkinCBuffer.boneCount);
			}
		}
	}
}
