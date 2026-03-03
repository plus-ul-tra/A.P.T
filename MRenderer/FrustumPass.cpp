#include "FrustumPass.h"

#include <DirectXCollision.h>
#include <cstring>

void FrustumPass::EnsureVertexBuffer()
{
	if (m_VertexBuffer)
	{
		return;
	}

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = static_cast<UINT>(sizeof(DirectX::XMFLOAT3) * 24);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	m_RenderContext.pDevice->CreateBuffer(&desc, nullptr, m_VertexBuffer.GetAddressOf());
}

void FrustumPass::UpdateVertices(const std::array<DirectX::XMFLOAT3, 24>& vertices)
{
	D3D11_MAPPED_SUBRESOURCE mapped = {};
	if (FAILED(m_RenderContext.pDXDC->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		return;
	}

	std::memcpy(mapped.pData, vertices.data(), sizeof(DirectX::XMFLOAT3) * vertices.size());
	m_RenderContext.pDXDC->Unmap(m_VertexBuffer.Get(), 0);
}

void FrustumPass::Execute(const RenderData::FrameData& frame)
{
	if (!m_RenderContext.isEditCam)
	{
		return;
	}

	EnsureVertexBuffer();

	const auto& cameraContext = frame.context.gameCamera;
	if (cameraContext.width == 0 || cameraContext.height == 0)
	{
		return;
	}

	DirectX::BoundingFrustum frustum;
	const auto proj = DirectX::XMLoadFloat4x4(&cameraContext.proj);
	DirectX::BoundingFrustum::CreateFromMatrix(frustum, proj);

	const auto view = DirectX::XMLoadFloat4x4(&cameraContext.view);
	const auto invView = DirectX::XMMatrixInverse(nullptr, view);
	DirectX::BoundingFrustum worldFrustum;
	frustum.Transform(worldFrustum, invView);

	DirectX::XMFLOAT3 corners[8] = {};
	worldFrustum.GetCorners(corners);

	const std::array<DirectX::XMFLOAT3, 24> lineVertices = {
		corners[0], corners[1],
		corners[1], corners[2],
		corners[2], corners[3],
		corners[3], corners[0],
		corners[4], corners[5],
		corners[5], corners[6],
		corners[6], corners[7],
		corners[7], corners[4],
		corners[0], corners[4],
		corners[1], corners[5],
		corners[2], corners[6],
		corners[3], corners[7]
	};

	UpdateVertices(lineVertices);

	DirectX::XMFLOAT4X4 identity;
	DirectX::XMStoreFloat4x4(&identity, DirectX::XMMatrixIdentity());
	m_RenderContext.BCBuffer.mWorld = identity;
	m_RenderContext.BCBuffer.mWorldInvTranspose = identity;
	UpdateDynamicBuffer(m_RenderContext.pDXDC.Get(), m_RenderContext.pBCB.Get(), &m_RenderContext.BCBuffer, sizeof(BaseConstBuffer));

	SetCameraCB(frame);
	SetBlendState(BS::DEFAULT);
	SetRasterizerState(RS::WIREFRM);
	SetDepthStencilState(DS::DEPTH_ON);
	//SetRenderTarget(m_RenderContext.pRTView_Imgui_edit.Get(), m_RenderContext.pDSViewScene_Depth.Get());
	SetViewPort(m_RenderContext.WindowSize.width, m_RenderContext.WindowSize.height, m_RenderContext.pDXDC.Get());

	UINT stride = sizeof(DirectX::XMFLOAT3);
	UINT offset = 0;
	ID3D11Buffer* vb = m_VertexBuffer.Get();
	ID3D11DeviceContext* dc = m_RenderContext.pDXDC.Get();

	dc->IASetInputLayout(m_RenderContext.InputLayout_P.Get());
	dc->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	dc->VSSetShader(m_RenderContext.VS_P.Get(), nullptr, 0);
	dc->PSSetShader(m_RenderContext.PS_Frustum.Get(), nullptr, 0);
	dc->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->PSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	dc->VSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());
	dc->PSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());

	dc->Draw(static_cast<UINT>(lineVertices.size()), 0);
}
