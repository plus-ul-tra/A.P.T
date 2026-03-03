#include "Border.h"
#include "ReflectionMacro.h"
#include <algorithm>

REGISTER_UI_COMPONENT(Border)
REGISTER_PROPERTY(Border, Padding)

void Border::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void Border::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

UIRect Border::GetContentRect(const UIRect& bounds) const
{
	UIRect content = bounds;
	content.x += m_Padding.left;
	content.y += m_Padding.top;
	content.width  = max(0.0f, bounds.width  - (m_Padding.left + m_Padding.right));
	content.height = max(0.0f, bounds.height - (m_Padding.top + m_Padding.bottom));

	return content;
}
