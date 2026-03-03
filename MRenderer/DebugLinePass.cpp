#include "DebugLinePass.h"

#include <algorithm>
#include <cstdint>
#include <cstring>

void AppendAabbLines(std::vector<DirectX::XMFLOAT3>& vertices,
	const RenderData::RenderItem& item)
{
	if (!item.hasBounds)
	{
		return;
	}

	const auto world = DirectX::XMLoadFloat4x4(&item.world);
	const auto localToWorld = DirectX::XMLoadFloat4x4(&item.localToWorld);
	const auto worldMatrix = DirectX::XMMatrixMultiply(localToWorld, world);

	const DirectX::XMFLOAT3 min = item.boundsMin;
	const DirectX::XMFLOAT3 max = item.boundsMax;

	const DirectX::XMFLOAT3 corners[8] = {
		{ min.x, min.y, min.z },
		{ max.x, min.y, min.z },
		{ max.x, max.y, min.z },
		{ min.x, max.y, min.z },
		{ min.x, min.y, max.z },
		{ max.x, min.y, max.z },
		{ max.x, max.y, max.z },
		{ min.x, max.y, max.z }
	};

	DirectX::XMFLOAT3 worldCorners[8]{};
	for (size_t i = 0; i < 8; ++i)
	{
		const auto corner = DirectX::XMLoadFloat3(&corners[i]);
		const auto transformed = DirectX::XMVector3TransformCoord(corner, worldMatrix);
		DirectX::XMStoreFloat3(&worldCorners[i], transformed);
	}

	const uint8_t edges[24] = {
		0, 1, 1, 2, 2, 3, 3, 0,
		4, 5, 5, 6, 6, 7, 7, 4,
		0, 4, 1, 5, 2, 6, 3, 7
	};

	for (size_t i = 0; i < 24; i += 2)
	{
		vertices.push_back(worldCorners[edges[i]]);
		vertices.push_back(worldCorners[edges[i + 1]]);
	}
}

void DebugLinePass::EnsureVertexBuffer(size_t vertexCount)
{
	if (m_VertexBuffer && m_VertexCapacity >= vertexCount)
	{
		return;
	}

	m_VertexBuffer.Reset();
	m_VertexCapacity = vertexCount;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth = static_cast<UINT>(sizeof(DirectX::XMFLOAT3) * m_VertexCapacity);
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	m_RenderContext.pDevice->CreateBuffer(&desc, nullptr, m_VertexBuffer.GetAddressOf());
}

void DebugLinePass::UpdateVertices(const std::vector<DirectX::XMFLOAT3>& vertices)
{
	if (vertices.empty())
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE mapped = {};
	if (FAILED(m_RenderContext.pDXDC->Map(m_VertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped)))
	{
		return;
	}

	std::memcpy(mapped.pData, vertices.data(), sizeof(DirectX::XMFLOAT3) * vertices.size());
	m_RenderContext.pDXDC->Unmap(m_VertexBuffer.Get(), 0);
}

void DebugLinePass::Execute(const RenderData::FrameData& frame)
{
	if (!m_RenderContext.isEditCam)
	{
		return;
	}

	std::vector<DirectX::XMFLOAT3> lineVertices;
	for (const auto& [layer, items] : frame.renderItems)
	{
		(void)layer;
		for (const auto& item : items)
		{
			AppendAabbLines(lineVertices, item);

			if (!item.skeleton.IsValid() || (item.globalPoseCount == 0 && item.skinningPaletteCount == 0))
			{
				continue;
			}

			const RenderData::Skeleton* skeleton = m_AssetLoader.GetSkeletons().Get(item.skeleton);
			if (!skeleton || skeleton->bones.empty())
			{
				continue;
			}

			const auto itemWorld = DirectX::XMLoadFloat4x4(&item.world);

			const bool hasGlobalPose = item.globalPoseCount > 0;
			if (hasGlobalPose)
			{
				const size_t poseStart = item.globalPoseOffset;
				if (poseStart >= frame.globalPoses.size())
				{
					continue;
				}

				const size_t available = frame.globalPoses.size() - poseStart;
				size_t boneCount = min(skeleton->bones.size(), available);
				boneCount = min(boneCount, static_cast<size_t>(item.globalPoseCount));

				for (size_t i = 0; i < boneCount; ++i)
				{
					const int parentIndex = skeleton->bones[i].parentIndex;
					if (parentIndex < 0 || static_cast<size_t>(parentIndex) >= boneCount)
					{
						continue;
					}

					const auto& childPoseF = frame.globalPoses[poseStart + i];
					const auto& parentPoseF = frame.globalPoses[poseStart + static_cast<size_t>(parentIndex)];

					const auto childPose = DirectX::XMLoadFloat4x4(&childPoseF);
					const auto parentPose = DirectX::XMLoadFloat4x4(&parentPoseF);

					const auto worldChild = DirectX::XMMatrixMultiply(childPose, itemWorld);
					const auto worldParent = DirectX::XMMatrixMultiply(parentPose, itemWorld);

					DirectX::XMFLOAT4X4 gC, gP;
					DirectX::XMStoreFloat4x4(&gC, worldChild);
					DirectX::XMStoreFloat4x4(&gP, worldParent);

					lineVertices.push_back({ gC._41, gC._42, gC._43 });
					lineVertices.push_back({ gP._41, gP._42, gP._43 });
				}
			}
			else
			{
				const size_t paletteStart = item.skinningPaletteOffset;
				if (paletteStart >= frame.skinningPalettes.size())
				{
					continue;
				}

				const size_t available = frame.skinningPalettes.size() - paletteStart;
				size_t boneCount = min(skeleton->bones.size(), available);
				boneCount = min(boneCount, static_cast<size_t>(item.skinningPaletteCount));

				for (size_t i = 0; i < boneCount; ++i)
				{
					const int parentIndex = skeleton->bones[i].parentIndex;
					if (parentIndex < 0 || static_cast<size_t>(parentIndex) >= boneCount)
					{
						continue;
					}

					const auto& skinChildF = frame.skinningPalettes[paletteStart + i];
					const auto& skinParentF = frame.skinningPalettes[paletteStart + static_cast<size_t>(parentIndex)];

					const auto skinChild = DirectX::XMLoadFloat4x4(&skinChildF);
					const auto skinParent = DirectX::XMLoadFloat4x4(&skinParentF);

					const auto invBindChild = DirectX::XMLoadFloat4x4(&skeleton->bones[i].inverseBindPose);
					const auto invBindParent = DirectX::XMLoadFloat4x4(&skeleton->bones[static_cast<size_t>(parentIndex)].inverseBindPose);

					const auto bindChild = DirectX::XMMatrixInverse(nullptr, invBindChild);
					const auto bindParent = DirectX::XMMatrixInverse(nullptr, invBindParent);

					const auto globalChild = DirectX::XMMatrixMultiply(skinChild, bindChild);
					const auto globalParent = DirectX::XMMatrixMultiply(skinParent, bindParent);

					const auto worldChild = DirectX::XMMatrixMultiply(globalChild, itemWorld);
					const auto worldParent = DirectX::XMMatrixMultiply(globalParent, itemWorld);

					DirectX::XMFLOAT4X4 gC, gP;
					DirectX::XMStoreFloat4x4(&gC, worldChild);
					DirectX::XMStoreFloat4x4(&gP, worldParent);

					lineVertices.push_back({ gC._41, gC._42, gC._43 });
					lineVertices.push_back({ gP._41, gP._42, gP._43 });
				}
			}
		}
	}

	if (lineVertices.empty())
	{
		return;
	}

	EnsureVertexBuffer(lineVertices.size());
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