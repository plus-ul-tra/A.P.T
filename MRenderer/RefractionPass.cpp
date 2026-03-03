#include "RefractionPass.h"

void RefractionPass::Execute(const RenderData::FrameData& frame)
{
    if (m_RenderContext.isEditCam)
        return;

    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
    FLOAT backcolor[4] = { 0.21f, 0.21f, 0.21f, 1.0f };

    SetRenderTarget(m_RenderContext.pRTView_Refraction.Get(), nullptr, backcolor);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());        
    SetBlendState(BS::ALPHABLEND_WALL);
    SetRasterizerState(RS::CULLBACK);
    SetDepthStencilState(DS::DEPTH_OFF);
    SetSamplerState();

#pragma endregion

    if (m_RenderContext.pRTScene_ImguiMSAA && m_RenderContext.pRTScene_Imgui)
    {
        dxdc->ResolveSubresource(
            m_RenderContext.pRTScene_Imgui.Get(),
            0,
            m_RenderContext.pRTScene_ImguiMSAA.Get(),
            0,
            DXGI_FORMAT_R16G16B16A16_FLOAT);
    }


    //현재는 depthpass에서 먼저 그려주기 때문에 여기서 지워버리면 안된다. 지울 위치를 잘 찾아보자
    //ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);

    //렌더타겟 리소스 연결
    dxdc->PSSetShaderResources(0, 1, m_RenderContext.pTexRvScene_Imgui.GetAddressOf());

    //물 노이즈 연결
    dxdc->PSSetShaderResources(6, 1, m_RenderContext.WaterNoise.GetAddressOf());

    //전체 화면 그리기
    XMMATRIX tm = XMMatrixIdentity();
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, tm);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, tm);

    XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 1.0f, 100.0f);

    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, mProj);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, mProj);
    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));


    dxdc->VSSetShader(m_RenderContext.VS_FSTriangle.Get(), nullptr, 0);
    dxdc->PSSetShader(m_RenderContext.PS_Quad.Get(), nullptr, 0);

    m_RenderContext.DrawFSTriangle();




    SetCameraCB(frame);

    SetDirLight(frame);

    ID3D11DepthStencilView* depthView = m_RenderContext.pDSViewScene_DepthMSAA
        ? m_RenderContext.pDSViewScene_DepthMSAA.Get()
        : m_RenderContext.pDSViewScene_Depth.Get();


    dxdc->OMSetRenderTargets(1, m_RenderContext.pRTView_Refraction.GetAddressOf(), depthView);
    SetDepthStencilState(DS::DEPTH_ON_WRITE_OFF);

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

        ID3D11VertexShader* vertexShader = m_RenderContext.VS.Get();
        ID3D11PixelShader* pixelShader = m_RenderContext.PS.Get();

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
