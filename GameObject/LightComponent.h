#pragma once
#include "Component.h"
#include "RenderData.h"
using namespace RenderData;

class LightComponent : public Component
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "LightComponent";
	const char* GetTypeName() const override;

	LightComponent() = default;
	virtual ~LightComponent() = default;

	void SetType(const RenderData::LightType& type) { m_Type = type; }
	RenderData::LightType GetType() const    { return m_Type; }

	void  SetColor(const XMFLOAT3& color){ m_Color = color; }
	const XMFLOAT3& GetColor() const     { return m_Color;  }

	void  SetIntensity(const float& intensity)		  { m_Intensity = intensity; }
	const float& GetIntensity() const         { return m_Intensity;      }

	void  SetPosition(const XMFLOAT3& position){ m_Position = position; }
	const XMFLOAT3& GetPosition() const        { return m_Position;     }

	void SetCastShadow			(const bool& castShadow) { m_CastShadow = castShadow; }
	const bool& GetCastShadow   () const          { return m_CastShadow;       }

	virtual RenderData::LightData BuildLightData() const;

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	virtual void FillLightData(RenderData::LightData& data) const;

protected:
	LightType  m_Type = LightType::None;
	XMFLOAT3   m_Color        {};
	XMFLOAT3   m_Position	  {};
	float	   m_Intensity = 0.0f;
	XMFLOAT4X4 m_LightViewProj{};
	bool       m_CastShadow = true;
};


