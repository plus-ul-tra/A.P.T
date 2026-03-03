#include "SizeBox.h"
#include "ReflectionMacro.h"
#include <algorithm>

REGISTER_UI_COMPONENT(SizeBox)
REGISTER_PROPERTY(SizeBox, WidthOverride)
REGISTER_PROPERTY(SizeBox, HeightOverride)
REGISTER_PROPERTY(SizeBox, HasWidthOverride)
REGISTER_PROPERTY(SizeBox, HasHeightOverride)
REGISTER_PROPERTY(SizeBox, MinSize)
REGISTER_PROPERTY(SizeBox, MaxSize)

void SizeBox::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void SizeBox::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

UISize SizeBox::GetDesiredSize(const UISize& contentSize) const
{
	UISize result = contentSize;
	if (m_HasWidthOverride)
	{
		result.width = m_WidthOverride;
	}

	if (m_HasHeightOverride)
	{
		result.height = m_HeightOverride;
	}
	if (m_MaxSize.width > 0.0f)
	{
		result.width = min(result.width, m_MaxSize.width);
	}
	if (m_MaxSize.height > 0.0f)
	{
		result.height = min(result.height, m_MaxSize.height);
	}
	if (m_MinSize.width > 0.0f)
	{
		result.width = max(result.width, m_MinSize.width);
	}
	if (m_MinSize.height > 0.0f)
	{
		result.height = max(result.height, m_MinSize.height);
	}
	return result;
}
