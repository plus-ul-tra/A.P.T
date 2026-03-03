#include "DirectionalLightComponent.h"
#include "MathHelper.h"
#include "ReflectionMacro.h"
using namespace MathUtils;

REGISTER_COMPONENT_DERIVED(DirectionalLightComponent, LightComponent)
REGISTER_PROPERTY(DirectionalLightComponent, Direction)
REGISTER_PROPERTY(DirectionalLightComponent, Contrast)
REGISTER_PROPERTY(DirectionalLightComponent, Saturation)

DirectionalLightComponent::DirectionalLightComponent()
{
	m_LightViewProj = Identity();
	m_Type = LightType::Directional;
}

void DirectionalLightComponent::Update(float deltaTime)
{
}

void DirectionalLightComponent::OnEvent(EventType type, const void* data)
{
}

void DirectionalLightComponent::FillLightData(RenderData::LightData& data) const
{
	LightComponent::FillLightData(data);
	data.direction = m_Direction;
	data.contrast = m_Contrast;
	data.saturation = m_Saturation;
}
