#include "WallPass.h"

void WallPass::Execute(const RenderData::FrameData & frame)
{
    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
    //SetRenderTarget()		Opaque단계에서 처리
    //SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());        Opaque단계에서 처리
    SetBlendState(BS::ALPHABLEND_WALL);
    SetRasterizerState(RS::CULLBACK);
    SetDepthStencilState(DS::DEPTH_ON);
    SetSamplerState();

#pragma endregion

    SetCameraCB(frame);
    SetDirLight(frame);
    SetOtherLights(frame);


    //현재는 depthpass에서 먼저 그려주기 때문에 여기서 지워버리면 안된다. 지울 위치를 잘 찾아보자
    //ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);

    //임시 벽뚫 이미지 바인딩
    m_RenderContext.pDXDC->PSSetShaderResources(5, 1, m_RenderContext.Vignetting.GetAddressOf());

    SetMaskingTM(frame, frame.context.gameCamera.cameraPos);             //현재는 각 오브젝트 전부 바라보고 있음, 후에 플레이어 정보를 받아서 플레이어쪽을 바라보도록 해야함, for문 위에서 미리 한번만 세팅해두는 방식이 좋을 듯


    for (const auto& queueItem : GetQueue())
    {
        const auto& item = *queueItem.item;
        SetBaseCB(item);

        const auto* vertexBuffers = m_RenderContext.vertexBuffers;
        const auto* indexBuffers = m_RenderContext.indexBuffers;
        const auto* indexcounts = m_RenderContext.indexCounts;
        const auto* textures = m_RenderContext.textures;
        const auto* vertexShaders = m_RenderContext.vertexShaders;
        const auto* pixelShaders = m_RenderContext.pixelShaders;

        ID3D11VertexShader* vertexShader = m_RenderContext.VS_Wall.Get();
        ID3D11PixelShader* pixelShader = m_RenderContext.PS_Wall.Get();

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
                const UINT32 fullCount = countIt->second;
                const bool useSubMesh = item.useSubMesh;
                const UINT32 indexCount = useSubMesh ? item.indexCount : fullCount;
                const UINT32 indexStart = useSubMesh ? item.indexStart : 0;

                DrawMesh(vb, ib, vertexShader, pixelShader, useSubMesh, indexCount, indexStart);
            }
        }
    }
}