#include "SpotLightComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(SpotLightComponent, LightComponent)
REGISTER_PROPERTY(SpotLightComponent, Direction)
REGISTER_PROPERTY(SpotLightComponent, Range)
REGISTER_PROPERTY(SpotLightComponent, SpotInnerAngle)
REGISTER_PROPERTY(SpotLightComponent, SpotOutterAngle)
REGISTER_PROPERTY(SpotLightComponent, AttenuationRadius)

SpotLightComponent::SpotLightComponent()
{
	m_Type = LightType::Spot;
}

void SpotLightComponent::Update(float deltaTime)
{
	LightComponent::Update(deltaTime);
}

void SpotLightComponent::OnEvent(EventType type, const void* data)
{
}

void SpotLightComponent::FillLightData(RenderData::LightData& data) const
{
	LightComponent::FillLightData(data);
	data.range = m_Range;
	data.direction = m_Direction;
	data.spotOutterAngle = m_SpotOutterAngle;
	data.spotInnerAngle = m_SpotInnerAngle;
	data.attenuationRadius = m_AttenuationRadius;
}
