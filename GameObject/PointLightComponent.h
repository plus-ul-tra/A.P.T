#pragma once
#include "LightComponent.h"

class PointLightComponent : public LightComponent
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "PointLightComponent";
	const char* GetTypeName() const override;

	PointLightComponent();
	virtual ~PointLightComponent() = default;

	void  SetAttenuationRadius(const float& radius)  { m_AttenuationRadius = radius; }
	const float& GetAttenuationRadius() const { return m_AttenuationRadius;   }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	void FillLightData(RenderData::LightData& data) const override;

protected:
	float m_AttenuationRadius;
};

