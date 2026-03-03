#pragma once

#include <imgui.h>
#include <string>
#include "RenderTargetContext.h"

class EditorViewport
{
public:
	//EditorViewport() = default;
	explicit EditorViewport(std::string windowName = "Editor"): m_WindowName(std::move(windowName)){}
	~EditorViewport() = default;

	bool Draw(const RenderTargetContext& renderTarget); 
	const ImVec2& GetViewportSize() const { return m_ViewportSize;}

	bool HasViewportRect() const { return m_HasViewportRect; }
	const ImVec2& GetViewportRectMin() const { return m_ViewportRectMin; }
	const ImVec2& GetViewportRectMax() const { return m_ViewportRectMax; }
	ImDrawList* GetDrawList() const { return m_DrawList; }
	bool IsHovered() const { return m_IsHovered; }
private:
	std::string m_WindowName; 
	ImVec2 m_ViewportSize{ 0.0f, 0.0f };
	bool m_IsHovered = false;
	ImDrawList* m_DrawList = nullptr; //release
	bool m_HasViewportRect = false;
	ImVec2 m_ViewportRectMin{ 0.0f, 0.0f };
	ImVec2 m_ViewportRectMax{ 0.0f, 0.0f };
};