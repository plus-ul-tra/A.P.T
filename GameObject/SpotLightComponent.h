#pragma once
#include "LightComponent.h"

class SpotLightComponent : public LightComponent
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "SpotLightComponent";
	const char* GetTypeName() const override;

	SpotLightComponent();
	virtual ~SpotLightComponent() = default;

	void  SetDirection(const XMFLOAT3& direction) { m_Direction = direction;	  }
	const XMFLOAT3& GetDirection() const          { return m_Direction;			  }
	void  SetSpotInnerAngle(const float& angle)		  { m_SpotInnerAngle = angle;	  }
	const float& GetSpotInnerAngle() const		  { return m_SpotInnerAngle;	  }
	void  SetSpotOutterAngle(const float& angle)		  { m_SpotOutterAngle = angle;    }
	const float& GetSpotOutterAngle() const		  { return m_SpotOutterAngle;     }
	void  SetAttenuationRadius(const float& radius)	  { m_AttenuationRadius = radius; }
	const float& GetAttenuationRadius() const	  { return m_AttenuationRadius;   }
	void  SetRange(const float& range)					  { m_Range = range;			  }
	const float& GetRange() const				  { return m_Range;				  }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	void FillLightData(RenderData::LightData& data) const override;

protected:
	XMFLOAT3   m_Direction{ 1, 0, 0 };
	float      m_Range = 0.0f;
	float	   m_SpotInnerAngle = 0.0f;
	float	   m_SpotOutterAngle = 0.0f;
	float	   m_AttenuationRadius = 0.0f;
};

