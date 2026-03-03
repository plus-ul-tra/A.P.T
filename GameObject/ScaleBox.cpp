#include "ScaleBox.h"
#include "ReflectionMacro.h"
#include <algorithm>

REGISTER_UI_COMPONENT(ScaleBox)
REGISTER_PROPERTY(ScaleBox, Stretch)
REGISTER_PROPERTY(ScaleBox, StretchDirection)

void ScaleBox::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void ScaleBox::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

float ScaleBox::CalculateScale(const UISize& availableSize, const UISize& desiredSize) const
{
	if (m_Stretch == UIStretch::None)
	{
		return 1.0f;
	}

	if (desiredSize.width <= 0.0f || desiredSize.height <= 0.0f)
	{
		return 1.0f;
	}

	const float scaleX = availableSize.width / desiredSize.width;
	const float scaleY = availableSize.height / desiredSize.height;
	float scale = (m_Stretch == UIStretch::ScaleToFill) ? max(scaleX, scaleY) : min(scaleX, scaleY);

	if (m_StretchDirection == UIStretchDirection::DownOnly)
	{
		scale = min(scale, 1.0f);
	}
	else if (m_StretchDirection == UIStretchDirection::UpOnly)
	{
		scale = max(scale, 1.0f);
	}

	return scale;
}

UISize ScaleBox::CalculateScaledSize(const UISize& availableSize, const UISize& desiredSize) const
{
	const float scale = CalculateScale(availableSize, desiredSize);
	UISize scaled = desiredSize;
	scaled.width  *= scale;
	scaled.height *= scale;
	return scaled;
}
