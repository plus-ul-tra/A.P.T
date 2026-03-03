#include "UIObject.h"
#include "Object.h"
#include "UIButtonComponent.h"
#include "UISliderComponent.h"
#include "UIFSMComponent.h"
#include "UIComponent.h"
#include "Reflection.h"

UIObject::UIObject(EventDispatcher& eventDispatcher) : Object(eventDispatcher)
{

}

//void UIObject::Render(std::vector<UIRenderInfo>& renderInfo)
//{
//	for (auto& image : GetComponents<UIImageComponent>())
//	{
//		UIRenderInfo info;
//		info.bitmap = image->GetTexture();
//		info.anchoredPosition = m_RectTransform->GetPosition();
//		info.anchor = m_RectTransform->GetAnchor();
//		info.sizeDelta = { 0, 0 }; 
//		info.layer = m_ZOrder;
//		if (m_RectTransform->GetParent())
//		{
//			auto parentSize = m_RectTransform->GetParent()->GetSize();
//			info.parentSize = parentSize;
//		}
//		else
//		{
//			auto size = m_RectTransform->GetSize();
//			info.parentSize = { size.x, size.y };
//		}
//		info.pivot = m_RectTransform->GetPivot();
//		// Opacity 적용
//		info.opacity = image->GetOpacity();
//		info.srcRect = D2D1_RECT_F{ image->GetUV().left, image->GetUV().top, image->GetUV().right, image->GetUV().bottom };
//		renderInfo.emplace_back(info);
//	}
//	for(auto& slider : GetComponents<UISliderComponent>())
//	{
//		UIRenderInfo frameInfo;
//		auto frameImage = slider->GetFrame()->GetComponent<UIImageComponent>();
//		frameInfo.bitmap = frameImage->GetTexture();
//		auto frameRect = slider->GetFrame()->GetComponent<RectTransformComponent>();
//		frameInfo.anchor = frameRect->GetAnchor();
//		frameInfo.anchoredPosition = frameRect->GetPosition();
//		frameInfo.sizeDelta = { 0, 0 };
//		frameInfo.layer = m_ZOrder;
//		if (frameRect->GetParent())
//		{
//			auto parentSize = frameRect->GetParent()->GetSize();
//			frameInfo.parentSize = parentSize;
//		}
//		else
//		{
//			auto size = frameRect->GetSize();
//			frameInfo.parentSize = size;
//		}
//		frameInfo.pivot = frameRect->GetPivot();
//		// Opacity 적용
//		frameInfo.opacity = frameImage->GetOpacity();
//		frameInfo.srcRect = D2D1_RECT_F{ frameImage->GetUV().left, frameImage->GetUV().top, frameImage->GetUV().right, frameImage->GetUV().bottom };
//		frameInfo.useSrcRect = true;
//		renderInfo.emplace_back(frameInfo);
//		
//		UIRenderInfo fillInfo;
//		auto fillImage = slider->GetFill()->GetComponent<UIImageComponent>();
//		fillInfo.bitmap = fillImage->GetTexture();
//		auto fillRect = slider->GetFill()->GetComponent<RectTransformComponent>();
//		fillInfo.anchor = fillRect->GetAnchor();
//		fillInfo.anchoredPosition = fillRect->GetPosition();
//		fillInfo.sizeDelta = { 0, 0 };
//		fillInfo.layer = m_ZOrder;
//		if (fillRect->GetParent())
//		{
//			auto parentSize = fillRect->GetParent()->GetSize();
//			fillInfo.parentSize = parentSize;
//		}
//		else
//		{
//			auto size = fillRect->GetSize();
//			fillInfo.parentSize = size;
//		}
//		fillInfo.pivot = fillRect->GetPivot();
//		// Opacity 적용
//		fillInfo.opacity = fillImage->GetOpacity();
//		fillInfo.srcRect = D2D1_RECT_F{ fillImage->GetUV().left, fillImage->GetUV().top, fillImage->GetUV().right, fillImage->GetUV().bottom };
//		fillInfo.useSrcRect = true;
//		renderInfo.emplace_back(fillInfo);
//	}
//	for (auto& grid : GetComponents<UIGridComponent>())
//	{
//		for (auto item : grid->GetItems())
//		{
//			for (auto image : item->GetComponents<UIImageComponent>())
//			{
//				UIRenderInfo info;
//				info.bitmap = image->GetTexture();
//				auto rect = item->GetComponent<RectTransformComponent>();
//				info.anchoredPosition = rect->GetPosition();
//				info.anchor = rect->GetAnchor();
//				info.sizeDelta = { 0, 0 };
//				info.layer = m_ZOrder;
//				if (rect->GetParent())
//				{
//					auto parentSize = rect->GetParent()->GetSize();
//					info.parentSize = parentSize;
//				}
//				else
//				{
//					auto size = rect->GetSize();
//					info.parentSize = { size.x, size.y };
//				}
//				info.pivot = rect->GetPivot();
//				// Opacity 적용
//				info.opacity = image->GetOpacity();
//				info.srcRect = D2D1_RECT_F{ image->GetUV().left, image->GetUV().top, image->GetUV().right, image->GetUV().bottom };
//				if (!item->m_IsVisible)
//					info.parentSize = { 0, 0 };
//				renderInfo.emplace_back(info);
//			}
//		}
//	}
//}
//
//void UIObject::Render(std::vector<UITextInfo>& renderInfo)
//{
//	for (auto& text : GetComponents<UITextComponent>())
//	{
//		UITextInfo info;
//		info.anchoredPosition = m_RectTransform->GetPosition() + text->GetPosition();
//		info.anchor = m_RectTransform->GetAnchor();
//		info.sizeDelta = { 0, 0 };
//		info.layer = m_ZOrder;
//		if (m_RectTransform->GetParent())
//		{
//			auto parentSize = m_RectTransform->GetParent()->GetSize();
//			info.parentSize = parentSize;
//		}
//		else
//		{
//			auto size = m_RectTransform->GetSize();
//			info.parentSize = { size.x, size.y };
//		}
//		info.pivot = m_RectTransform->GetPivot();
//		// Opacity 적용
//
//		info.color = text->GetColor();
//		info.fontSize = text->GetFontSize();
//		info.text = text->GetText();
//		info.textLayout = text->GetTextLayout();
//		renderInfo.emplace_back(info);
//	}
//}

void UIObject::Serialize(nlohmann::json& j) const
{
	j["name"] = m_Name;
	j["bounds"] = { {"x", m_Bounds.x}, {"y", m_Bounds.y}, {"w", m_Bounds.width}, {"h", m_Bounds.height} };
	j["hasBounds"] = m_HasBounds;
	j["parent"] = m_ParentName;
	j["anchorMin"] = { {"x", m_AnchorMin.x}, {"y", m_AnchorMin.y} };
	j["anchorMax"] = { {"x", m_AnchorMax.x}, {"y", m_AnchorMax.y} };
	j["pivot"] = { {"x", m_Pivot.x}, {"y", m_Pivot.y} };
	j["rotation"] = m_RotationDegrees;
	j["zOrder"] = m_ZOrder;
	j["visible"] = m_IsVisible;

	nlohmann::json components = nlohmann::json::array();
	for (const auto& [typeName, comps] : m_Components)
	{
		for (const auto& comp : comps)
		{
			if (!comp)
			{
				continue;
			}
			nlohmann::json compJson;
			compJson["type"] = typeName;
			nlohmann::json data = nlohmann::json::object();
			comp->Serialize(data);
			compJson["data"] = data;
			components.push_back(compJson);
		}
	}
	j["components"] = components;
}

void UIObject::Deserialize(const nlohmann::json& j)
{
	m_Name = j.value("name", m_Name);
	if (j.contains("bounds"))
	{
		const auto& bounds = j.at("bounds");
		m_Bounds.x = bounds.value("x", m_Bounds.x);
		m_Bounds.y = bounds.value("y", m_Bounds.y);
		m_Bounds.width = bounds.value("w", m_Bounds.width);
		m_Bounds.height = bounds.value("h", m_Bounds.height);
		m_HasBounds = j.value("hasBounds", true);
	}
	m_ParentName = j.value("parent", "");
	if (j.contains("anchorMin"))
	{
		const auto& anchorMin = j.at("anchorMin");
		m_AnchorMin.x = anchorMin.value("x", m_AnchorMin.x);
		m_AnchorMin.y = anchorMin.value("y", m_AnchorMin.y);
	}
	if (j.contains("anchorMax"))
	{
		const auto& anchorMax = j.at("anchorMax");
		m_AnchorMax.x = anchorMax.value("x", m_AnchorMax.x);
		m_AnchorMax.y = anchorMax.value("y", m_AnchorMax.y);
	}
	if (j.contains("pivot"))
	{
		const auto& pivot = j.at("pivot");
		m_Pivot.x = pivot.value("x", m_Pivot.x);
		m_Pivot.y = pivot.value("y", m_Pivot.y);
	}
	m_RotationDegrees = j.value("rotation", m_RotationDegrees);
	m_ZOrder = j.value("zOrder", m_ZOrder);
	m_IsVisible = j.value("visible", m_IsVisible);

	m_Components.clear();
	if (j.contains("components"))
	{
		for (const auto& compJson : j.at("components"))
		{
			const std::string typeName = compJson.value("type", "");
			if (typeName.empty())
			{
				continue;
			}
			Component* component = AddComponentByTypeName(typeName);
			if (!component)
			{
				continue;
			}
			if (compJson.contains("data"))
			{
				component->Deserialize(compJson.at("data"));
			}
		}
	}
	UpdateInteractableFlags();
	if (auto* baseComponent = GetComponent<UIComponent>())
	{
		baseComponent->SetVisible(m_IsVisible);
		baseComponent->SetZOrder(m_ZOrder);
		SetOpacityFromComponent(baseComponent->GetOpacity());
	}
}

void UIObject::SetZOrder(int zOrder)
{
	m_ZOrder = zOrder;
	if (auto* baseComponent = GetComponent<UIComponent>())
	{
		if (baseComponent->GetZOrder() != zOrder)
		{
			baseComponent->SetZOrder(zOrder);
		}
	}
}

void UIObject::SetZOrderFromComponent(int zOrder)
{
	m_ZOrder = zOrder;
}

bool UIObject::HitCheck(const POINT& pos)
{
	if (!IsVisible())
		return false;

	if (!m_HasBounds)
		return false;

	return pos.x >= m_Bounds.x
		&& pos.y >= m_Bounds.y
		&& pos.x <= (m_Bounds.x + m_Bounds.width)
		&& pos.y <= (m_Bounds.y + m_Bounds.height);
}

bool UIObject::IsFullScreen()
{
	return m_IsFullScreen;
}

void UIObject::SetIsVisible(bool isVisible)
{
	m_IsVisible = isVisible;

	if (auto* baseComponent = GetComponent<UIComponent>())
	{
		if (baseComponent->GetVisible() != isVisible)
		{
			baseComponent->SetVisible(isVisible);
		}
	}
}

void UIObject::SetIsVisibleFromComponent(bool isVisible)
{
	m_IsVisible = isVisible;
}

void UIObject::SetIsVisibleFromParent(bool isVisible)
{
	m_IsVisibleFromParent = isVisible;
}

void UIObject::SetOpacity(float opacity)
{
	m_Opacity = opacity;

	if (auto* baseComponent = GetComponent<UIComponent>())
	{
		if (baseComponent->GetOpacity() != opacity)
		{
			baseComponent->SetOpacity(opacity);
		}
	}
}

void UIObject::SetOpacityFromComponent(float opacity)
{
	m_Opacity = opacity;
}

bool UIObject::IsVisible() const
{
	return m_IsVisible && m_IsVisibleFromParent;
}

void UIObject::UpdateInteractableFlags()
{
	hasButton = !GetComponents<UIButtonComponent>().empty();
	hasSlider = !GetComponents<UISliderComponent>().empty();
	hasUIFSM = !GetComponents<UIFSMComponent>().empty();
}
