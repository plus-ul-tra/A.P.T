#include "LightComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "TransformComponent.h"
#include <cassert>

REGISTER_COMPONENT(LightComponent);
REGISTER_PROPERTY(LightComponent, Color);
REGISTER_PROPERTY(LightComponent, Position);
REGISTER_PROPERTY(LightComponent, Intensity);
REGISTER_PROPERTY(LightComponent, CastShadow);

RenderData::LightData LightComponent::BuildLightData() const
{
	RenderData::LightData data{};
	FillLightData(data);
	return data;
}

void LightComponent::LightComponent::Update(float deltaTime)
{
	auto owner = GetOwner();
	if (owner == nullptr)
	{
		return;
	}
	if (auto* trans = owner->GetComponent<TransformComponent>())
	{
		m_Position = trans->GetWorldPos();
	}
}

void LightComponent::LightComponent::OnEvent(EventType type, const void* data)
{
}

void LightComponent::FillLightData(RenderData::LightData& data) const
{
	XMFLOAT3 resolvedPosition = m_Position;
	if (auto* owner = GetOwner())
	{
		if (auto* trans = owner->GetComponent<TransformComponent>())
		{
			resolvedPosition = trans->GetWorldPos();
		}
	}

	data.type = m_Type;
	data.posiiton = resolvedPosition;
	data.color = m_Color;
	data.intensity = m_Intensity;
	data.lightViewProj = m_LightViewProj;
	data.castShadow = m_CastShadow;
}
