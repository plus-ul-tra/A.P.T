#include "pch.h"
#include "EditorViewport.h"

namespace
{
	void SetOpaqueBlendState(const ImDrawList*, const ImDrawCmd*)
	{
		ImGui_ImplDX11_RenderState* renderState =
			static_cast<ImGui_ImplDX11_RenderState*>(ImGui::GetPlatformIO().Renderer_RenderState);


		if (!renderState || !renderState->Device || !renderState->DeviceContext)
			return;


		static Microsoft::WRL::ComPtr<ID3D11BlendState> s_OpaqueBlendState;
		if (!s_OpaqueBlendState)
		{
			D3D11_BLEND_DESC blendDesc = {};
			blendDesc.RenderTarget[0].BlendEnable = FALSE;
			blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
			renderState->Device->CreateBlendState(&blendDesc, s_OpaqueBlendState.GetAddressOf());
		}


		const FLOAT blendFactor[4] = { 0,0,0,0 };
		renderState->DeviceContext->OMSetBlendState(s_OpaqueBlendState.Get(), blendFactor, 0xFFFFFFFF);
	}
}

bool EditorViewport::Draw(const RenderTargetContext& renderTarget)
{
	ImGui::Begin(m_WindowName.c_str());
	m_DrawList = ImGui::GetWindowDrawList();
	m_IsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
	m_HasViewportRect = false;
	ImVec2 available = ImGui::GetContentRegionAvail();
	ImVec2 clamped = ImVec2((std::max)(1.0f, available.x), (std::max)(1.0f, available.y));

	const bool sizeChanged = (static_cast<int>(clamped.x) != static_cast<int>(m_ViewportSize.x))
		|| (static_cast<int>(clamped.y) != static_cast<int>(m_ViewportSize.y));
	m_ViewportSize = clamped;

	if (renderTarget.GetShaderResourceView())
	{
		//ImGui::Image(reinterpret_cast<ImTextureID>(renderTarget.GetShaderResourceView()), clamped, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

		// ★
		m_DrawList->AddCallback(SetOpaqueBlendState, nullptr);
		ImGui::Image(
			reinterpret_cast<ImTextureID>(renderTarget.GetShaderResourceView()),
			clamped,
			ImVec2(0.0f, 0.0f),
			ImVec2(1.0f, 1.0f));
		m_DrawList->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
		m_ViewportRectMin = ImGui::GetItemRectMin();
		m_ViewportRectMax = ImGui::GetItemRectMax();
		m_HasViewportRect = true;
	}
	else
	{
		ImGui::Text("Scene render target is not ready.");
	}

	ImGui::End();

	return sizeChanged;
}