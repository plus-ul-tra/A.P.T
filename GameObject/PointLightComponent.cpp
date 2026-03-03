#include "PointLightComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(PointLightComponent, LightComponent)
REGISTER_PROPERTY(PointLightComponent, AttenuationRadius)

PointLightComponent::PointLightComponent()
{
	m_Type = LightType::Point;
}

void PointLightComponent::Update(float deltaTime)
{
	LightComponent::Update(deltaTime);
}

void PointLightComponent::OnEvent(EventType type, const void* data)
{
}

void PointLightComponent::FillLightData(RenderData::LightData& data) const
{
	LightComponent::FillLightData(data);
	data.range = m_AttenuationRadius;
	data.attenuationRadius = m_AttenuationRadius;
}
