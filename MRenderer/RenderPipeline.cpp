#include "RenderPipeline.h"

#include <algorithm>

void RenderPipeline::AddPass(std::unique_ptr<RenderPass> pass)
{
	if (!pass)
	{
		return;
	}

	m_Passes.push_back(std::move(pass));
}

bool RenderPipeline::RemovePass(std::string_view name)
{
	const auto it = std::remove_if(m_Passes.begin(), m_Passes.end(),
		[name](const std::unique_ptr<RenderPass>& pass)
		{
			return pass && pass->GetName() == name;
		});

	if (it == m_Passes.end())
	{
		return false;
	}

	m_Passes.erase(it, m_Passes.end());
	return true;
}

RenderPass* RenderPipeline::FindPass(std::string_view name)
{
	for (const auto& pass : m_Passes)
	{
		if (pass && pass->GetName() == name)
		{
			return pass.get();
		}
	}

	return nullptr;
}

const RenderPass* RenderPipeline::FindPass(std::string_view name) const
{
	for (const auto& pass : m_Passes)
	{
		if (pass && pass->GetName() == name)
		{
			return pass.get();
		}
	}

	return nullptr;
}

void RenderPipeline::Execute(const RenderData::FrameData& frame, ID3D11DeviceContext* dxdc)
{
	for (const auto& pass : m_Passes)
	{
		if (!pass || !pass->IsEnabled())
		{
			continue;
		}

		pass->Setup(frame);
		ID3D11ShaderResourceView* nullSRVs[128] = { nullptr };
		dxdc->PSSetShaderResources(0, 128, nullSRVs);
		pass->Execute(frame);
	}
}

void RenderPipeline::Clear()
{
	m_Passes.clear();
}
