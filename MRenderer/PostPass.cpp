#include "PostPass.h"



void PostPass::Execute(const RenderData::FrameData& frame)
{
    if (m_RenderContext.isEditCam)
        return;

    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();
#pragma region Init
    ID3D11ShaderResourceView* nullSRVs[128] = { nullptr };
    dxdc->PSSetShaderResources(0, 128, nullSRVs);
    FLOAT backcolor[4] = { 0.21f, 0.21f, 0.21f, 1.0f };
    SetRenderTarget(m_RenderContext.pRTView_Post.Get(), nullptr, backcolor);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());
    SetBlendState(BS::DEFAULT);
    SetRasterizerState(RS::SOLID);
    SetDepthStencilState(DS::DEPTH_OFF);
    SetSamplerState();    
#pragma endregion

    //먼저 화면전체 Quad그리기
    const auto& context = frame.context;

    XMMATRIX tm = XMMatrixIdentity();
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, tm);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, tm);

    XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 1.0f, 100.0f);

    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, mProj);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, mProj);
    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));

    SetDirLight(frame);
    dxdc->VSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());
    dxdc->PSSetConstantBuffers(2, 1, m_RenderContext.pLightCB.GetAddressOf());


    //현재는 depthpass에서 먼저 그려주기 때문에 여기서 지워버리면 안된다. 지울 위치를 잘 찾아보자
    //ClearBackBuffer(D3D11_CLEAR_DEPTH, COLOR(0.21f, 0.21f, 0.21f, 1), m_RenderContext.pDXDC.Get(), m_RenderContext.pRTView.Get(), m_RenderContext.pDSView.Get(), 1, 0);

    dxdc->VSSetShader(m_RenderContext.VS_FSTriangle.Get(), nullptr, 0);
    dxdc->PSSetShader(m_RenderContext.PS_Post.Get(), nullptr, 0);
    dxdc->PSSetShaderResources(0, 1, m_RenderContext.pTexRvScene_Refraction.GetAddressOf());


    //ID3D11ShaderResourceView* depthSrv = m_RenderContext.pDepthRV.Get();
    //if (depthSrv)
    //{
    //    D3D11_SHADER_RESOURCE_VIEW_DESC depthSrvDesc = {};
    //    depthSrv->GetDesc(&depthSrvDesc);
    //    if (depthSrvDesc.ViewDimension != D3D11_SRV_DIMENSION_TEXTURE2D)
    //    {
    //        depthSrv = nullptr;
    //    }
    //}
    //dxdc->PSSetShaderResources(4, 1, &depthSrv);
    dxdc->PSSetShaderResources(4, 1, m_RenderContext.pDepthMSAARV.GetAddressOf());


    dxdc->PSSetShaderResources(6, 1, m_RenderContext.WaterNoise.GetAddressOf());
    dxdc->PSSetShaderResources(7, 1, m_RenderContext.pTexRvScene_EmissiveOrigin.GetAddressOf());
    dxdc->PSSetShaderResources(8, 1, m_RenderContext.pTexRvScene_Emissive[static_cast<UINT>(EmissiveLevel::HALF)].GetAddressOf());
    dxdc->PSSetShaderResources(9, 1, m_RenderContext.pTexRvScene_Emissive[static_cast<UINT>(EmissiveLevel::HALF2)].GetAddressOf());
    dxdc->PSSetShaderResources(10, 1, m_RenderContext.pTexRvScene_Emissive[static_cast<UINT>(EmissiveLevel::HALF3)].GetAddressOf());
    dxdc->PSSetShaderResources(31, 1, m_RenderContext.pTexRvScene_BlurOrigin.GetAddressOf());
    dxdc->PSSetShaderResources(32, 1, m_RenderContext.pTexRvScene_Blur[static_cast<UINT>(BlurLevel::HALF)].GetAddressOf());
    dxdc->PSSetShaderResources(33, 1, m_RenderContext.pTexRvScene_Blur[static_cast<UINT>(BlurLevel::HALF2)].GetAddressOf());
    dxdc->PSSetShaderResources(34, 1, m_RenderContext.pTexRvScene_Blur[static_cast<UINT>(BlurLevel::HALF3)].GetAddressOf());
    dxdc->PSSetShaderResources(35, 1, m_RenderContext.pTexRvScene_Blur[static_cast<UINT>(BlurLevel::HALF4)].GetAddressOf());

    m_RenderContext.DrawFSTriangle();


    ID3D11ShaderResourceView* nullSRV[128] = {};
    dxdc->PSSetShaderResources(0, 128, nullSRV);
    //m_RenderContext.MyDrawText(1920, 1080);


    //★아래 프레임데이터를 순회하면서 그리는게 필요없어 보이는데 어떻게 넘겨줄지 몰라서 일단 남김.
    //for (size_t index : GetQueue())
    //{
    //    for (const auto& [layer, items] : frame.renderItems)
    //    {

    //        const auto& item = items[index];

    //        m_RenderContext.BCBuffer.mWorld = item.world;

    //        UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &(m_RenderContext.BCBuffer), sizeof(m_RenderContext.BCBuffer));


    //        const auto* vertexBuffers = m_RenderContext.vertexBuffers;
    //        const auto* indexBuffers = m_RenderContext.indexBuffers;
    //        const auto* indexcounts = m_RenderContext.indexcounts;

    //        if (vertexBuffers && indexBuffers && indexcounts && item.mesh.IsValid())
    //        {
    //            const UINT bufferIndex = item.mesh.id;
    //            const auto vbIt = vertexBuffers->find(bufferIndex);
    //            const auto ibIt = indexBuffers->find(bufferIndex);
    //            const auto countIt = indexcounts->find(bufferIndex);
    //            if (vbIt != vertexBuffers->end() && ibIt != indexBuffers->end() && countIt != indexcounts->end())
    //            {
    //                ID3D11Buffer* vb = vbIt->second.Get();
    //                ID3D11Buffer* ib = ibIt->second.Get();
    //                UINT32 icount = countIt->second;
    //                if (vb && ib)
    //                {
    //                    const UINT stride = sizeof(RenderData::Vertex);
    //                    const UINT offset = 0;

    //                    //★여기 상태 설정하는것도 어떻게 할 지 정해진 뒤에 수정을 해야함

    //                    m_RenderContext.pDXDC->OMSetBlendState(m_RenderContext.BState[BS::ADD].Get(), NULL, 0xFFFFFFFF);
    //                    m_RenderContext.pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[DS::DEPTH_OFF].Get(), 0);
    //                    m_RenderContext.pDXDC->RSSetState(m_RenderContext.RState[RS::CULLBACK].Get());
    //                    m_RenderContext.pDXDC->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
    //                    m_RenderContext.pDXDC->IASetInputLayout(m_RenderContext.inputLayout.Get());
    //                    m_RenderContext.pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    //                    m_RenderContext.pDXDC->VSSetShader(m_RenderContext.VS.Get(), nullptr, 0);
    //                    m_RenderContext.pDXDC->PSSetShader(m_RenderContext.PS.Get(), nullptr, 0);
    //                    m_RenderContext.pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
    //                    m_RenderContext.pDXDC->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
    //                    m_RenderContext.pDXDC->DrawIndexed(icount, 0, 0);

    //                }
    //            }
    //        }
    //    }
    //}

}

