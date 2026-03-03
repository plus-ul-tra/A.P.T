#include "BlurPass.h"

void BlurPass::Execute(const RenderData::FrameData& frame)
{
    if (m_RenderContext.isEditCam)
        return;
    ID3D11DeviceContext* dxdc = m_RenderContext.pDXDC.Get();

#pragma region Init
    FLOAT backcolor[4] = { 0.21f, 0.21f, 0.21f, 1.0f };
    SetRenderTarget(m_RenderContext.pRTView_BlurOrigin.Get(), nullptr, backcolor);
    SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, dxdc);
    SetBlendState(BS::DEFAULT);
    SetRasterizerState(RS::SOLID);
    SetDepthStencilState(DS::DEPTH_OFF);
    SetSamplerState();

#pragma endregion


    XMMATRIX tm = XMMatrixIdentity();
    XMStoreFloat4x4(&m_RenderContext.BCBuffer.mWorld, tm);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mView, tm);

    XMMATRIX mProj = XMMatrixOrthographicOffCenterLH(0, (float)m_RenderContext.WindowSize.width, (float)m_RenderContext.WindowSize.height, 0.0f, 1.0f, 100.0f);

    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mProj, mProj);
    XMStoreFloat4x4(&m_RenderContext.CameraCBuffer.mVP, mProj);
    UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &(m_RenderContext.CameraCBuffer), sizeof(CameraConstBuffer));

    if (m_RenderContext.pRTScene_Refraction && m_RenderContext.pRTScene_RefractionMSAA)
    {
        dxdc->ResolveSubresource(
            m_RenderContext.pRTScene_Refraction.Get(),
            0,
            m_RenderContext.pRTScene_RefractionMSAA.Get(),
            0,
            DXGI_FORMAT_R16G16B16A16_FLOAT);
    }

    dxdc->PSSetShaderResources(0, 1, m_RenderContext.pTexRvScene_Refraction.GetAddressOf());

    dxdc->VSSetShader(m_RenderContext.VS_FSTriangle.Get(), nullptr, 0);
    dxdc->PSSetShader(m_RenderContext.PS_Quad.Get(), nullptr, 0);

    D3D11_VIEWPORT vp;
    UINT count = 1;
    dxdc->RSGetViewports(&count, &vp);

    m_RenderContext.DrawFSTriangle();
    //원본 그리기 종료

    //작게 그리기 시작
    SetRenderTarget(nullptr, nullptr, backcolor);

    SetBlendState(BS::DEFAULT);
    SetRasterizerState(RS::SOLID);
    SetDepthStencilState(DS::DEPTH_OFF);
    SetSamplerState();


    if (m_RenderContext.pRTScene_BlurOrigin && m_RenderContext.pRTScene_BlurOriginMSAA)
    {
        dxdc->ResolveSubresource(
            m_RenderContext.pRTScene_BlurOrigin.Get(),
            0,
            m_RenderContext.pRTScene_BlurOriginMSAA.Get(),
            0,
            DXGI_FORMAT_R16G16B16A16_FLOAT);
    }

    dxdc->PSSetShaderResources(0, 1, m_RenderContext.pTexRvScene_BlurOrigin.GetAddressOf());
    dxdc->VSSetShader(m_RenderContext.VS_FSTriangle.Get(), nullptr, 0);
    dxdc->PSSetShader(m_RenderContext.PS_Quad.Get(), nullptr, 0);


    SetRenderTarget(m_RenderContext.pRTView_Blur[static_cast<UINT>(BlurLevel::HALF)].Get(), nullptr, backcolor);
    SetViewPort(m_RenderContext.WindowSize.width / 2, m_RenderContext.WindowSize.height / 2, dxdc);
    dxdc->PSSetShaderResources(0, 1,
        m_RenderContext.pTexRvScene_BlurOrigin.GetAddressOf());
    m_RenderContext.DrawFSTriangle();


    SetRenderTarget(m_RenderContext.pRTView_Blur[static_cast<UINT>(BlurLevel::HALF2)].Get(), nullptr, backcolor);
    SetViewPort(m_RenderContext.WindowSize.width / 4, m_RenderContext.WindowSize.height / 4, dxdc);
    dxdc->PSSetShaderResources(0, 1,
        m_RenderContext.pTexRvScene_Blur[static_cast<UINT>(BlurLevel::HALF)].GetAddressOf());
    m_RenderContext.DrawFSTriangle();


    SetRenderTarget(m_RenderContext.pRTView_Blur[static_cast<UINT>(BlurLevel::HALF3)].Get(), nullptr, backcolor);
    SetViewPort(m_RenderContext.WindowSize.width / 8, m_RenderContext.WindowSize.height / 8, dxdc);
    dxdc->PSSetShaderResources(0, 1,
        m_RenderContext.pTexRvScene_Blur[static_cast<UINT>(BlurLevel::HALF2)].GetAddressOf());
    m_RenderContext.DrawFSTriangle();

    SetRenderTarget(m_RenderContext.pRTView_Blur[static_cast<UINT>(BlurLevel::HALF4)].Get(), nullptr, backcolor);
    SetViewPort(m_RenderContext.WindowSize.width / 16, m_RenderContext.WindowSize.height / 16, dxdc);
    dxdc->PSSetShaderResources(0, 1,
        m_RenderContext.pTexRvScene_Blur[static_cast<UINT>(BlurLevel::HALF3)].GetAddressOf());
    m_RenderContext.DrawFSTriangle();

}
