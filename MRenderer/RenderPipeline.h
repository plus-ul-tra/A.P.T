#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "RenderData.h"
#include "RenderPass.h"

// 여러 RenderPass를 순서대로 관리 및 실행하는 파이프라인 컨테이너
// 
class RenderPipeline
{
public:
	void AddPass(std::unique_ptr<RenderPass> pass);
	bool RemovePass(std::string_view name);
	RenderPass* FindPass(std::string_view name);
	const RenderPass* FindPass(std::string_view name) const;
	void Execute(const RenderData::FrameData& frame, ID3D11DeviceContext* dxdc);

	size_t GetPassCount() const { return m_Passes.size(); }
	void Clear();

private:
	std::vector<std::unique_ptr<RenderPass>> m_Passes;
};
