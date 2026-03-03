#pragma once
#pragma once

#include "RenderPass.h"

#include <array>

class FrustumPass : public RenderPass
{
public:
	FrustumPass(RenderContext& context, AssetLoader& assetLoader) : RenderPass(context, assetLoader) {}

	std::string_view GetName() const override { return "FrustumPass"; }
	void Execute(const RenderData::FrameData& frame) override;

private:
	void EnsureVertexBuffer();
	void UpdateVertices(const std::array<DirectX::XMFLOAT3, 24>& vertices);

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_VertexBuffer;
};
