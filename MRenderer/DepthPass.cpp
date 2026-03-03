#include "DepthPass.h"

#include <algorithm>

void DepthPass::Execute(const RenderData::FrameData& frame)
{
    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
    FLOAT backcolor[4] = { 0.21f, 0.21f, 0.21f, 1.0f };
    SetRenderTarget(nullptr, m_RenderContext.pDSViewScene_Depth.Get(), backcolor);
    SetViewPort(m_RenderContext.ShadowTextureSize.width, m_RenderContext.ShadowTextureSize.height, m_RenderContext.pDXDC.Get());
    SetBlendState(BS::DEFAULT);
    SetRasterizerState(RS::CULLBACK);
    SetDepthStencilState(DS::DEPTH_ON);

#pragma endregion

    const auto& context = frame.context;
    if (frame.lights.empty())
        return;
    const auto& mainlight = frame.lights[0];

    //만약 에디터 카메라 기준으로도 뎁스 기록하려면 여기아래에 에딧카메라 if문해서 넣으면됨
    m_RenderContext.CameraCBuffer.mView = context.gameCamera.view;
    m_RenderContext.CameraCBuffer.mProj = context.gameCamera.proj;
    m_RenderContext.CameraCBuffer.mVP = context.gameCamera.viewProj;
    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));


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

            m_RenderContext.pDXDC->VSSetConstantBuffers(1, 1, m_RenderContext.pSkinCB.GetAddressOf());
        }
        else if (m_RenderContext.pSkinCB)
        {
            m_RenderContext.SkinCBuffer.boneCount = 0;
            UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pSkinCB.Get(), &m_RenderContext.SkinCBuffer, sizeof(SkinningConstBuffer));
            m_RenderContext.pDXDC->VSSetConstantBuffers(1, 1, m_RenderContext.pSkinCB.GetAddressOf());
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
                    const UINT stride = sizeof(RenderData::Vertex);
                    const UINT offset = 0;

                    DrawMesh(vb, ib, m_RenderContext.VS_Shadow.Get(), nullptr, useSubMesh, indexCount, indexStart);
                }
            }
        }
    }
}
