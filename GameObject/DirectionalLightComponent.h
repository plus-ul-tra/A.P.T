#pragma once
#include "LightComponent.h"

class DirectionalLightComponent : public LightComponent
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "DirectionalLightComponent";
	const char* GetTypeName() const override;

	DirectionalLightComponent();
	virtual ~DirectionalLightComponent() = default;

	void  SetDirection(const XMFLOAT3& direction) { m_Direction = direction;	  }
	const XMFLOAT3& GetDirection() const          { return m_Direction;			  }
	void  SetContrast(const float& contrast) { m_Contrast = contrast; }
	const float& GetContrast() const { return m_Contrast; }
	void  SetSaturation(const float& saturation) { m_Saturation = saturation; }
	const float& GetSaturation() const { return m_Saturation; }

	void  SetLightViewProj(const XMFLOAT4X4& viewProj) { m_LightViewProj = viewProj; }
	const XMFLOAT4X4& GetLightViewProj() const { return m_LightViewProj; }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	void FillLightData(RenderData::LightData& data) const override;

protected:
	XMFLOAT3   m_Direction{ 1, 0, 0 };
	float      m_Contrast = 1.0f;
	float      m_Saturation = 1.0f;
	XMFLOAT4X4 m_LightViewProj{};
};

