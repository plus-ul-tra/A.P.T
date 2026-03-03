#include "RectLightComponent.h"
// 구현 후순위 같아서 등록 안하겠음

RectLightComponent::RectLightComponent()
{
	m_Type = LightType::Rect;
}

void RectLightComponent::Update(float deltaTime)
{
}

void RectLightComponent::OnEvent(EventType type, const void* data)
{
}
