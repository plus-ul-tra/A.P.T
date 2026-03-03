#pragma once

#include "RenderPass.h"

#include <vector>

class DebugLinePass : public RenderPass
{
public:
	DebugLinePass(RenderContext& context, AssetLoader& assetLoader) : RenderPass(context, assetLoader) {}

	std::string_view GetName() const override { return "DebugLinePass"; }
	void Execute(const RenderData::FrameData& frame) override;

private:
	void EnsureVertexBuffer(size_t vertexCount);
	void UpdateVertices(const std::vector<DirectX::XMFLOAT3>& vertices);

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
	size_t m_VertexCapacity = 0;
};