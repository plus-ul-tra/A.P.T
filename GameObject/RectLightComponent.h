#pragma once
#include "LightComponent.h"

class RectLightComponent : public LightComponent
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "RectLightComponent";
	const char* GetTypeName() const override;

	RectLightComponent();
	virtual ~RectLightComponent() = default;

	void  SetAttenuationRadius(const float& radius)  { m_AttenuationRadius = radius; }
	const float& GetAttenuationRadius() const { return m_AttenuationRadius;   }

	void SetSourceWidth(const float& width)    { m_SourceWidth = width; }
	const float& GetSourceWidth() const { return m_SourceWidth;  }

	void SetSourceHeight(const float& height)	 { m_SourceHeight = height; }
	const float& GetSourceHeight() const { return m_SourceHeight;	}

	void SetBarnDoorAngle(const float& angle)	  { m_BarnDoorAngle = angle; }
	const float& GetBarnDoorAngle() const { return m_BarnDoorAngle;  }

	void SetBarnDoorLength(const float& length)	{ m_BarnDoorLength = length; }
	const float& GetBarnDoorLength() const  { return m_BarnDoorLength;	 }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	float	   m_AttenuationRadius = 0.0f;
	float      m_SourceWidth       = 64.0f;
	float      m_SourceHeight      = 64.0f;
	float      m_BarnDoorAngle     = 60.0f;		// 차광 각도 <- 에지를 잘라내는 개념
												// 값이 작을수록 → 더 많이 가림 (타이트)		
												// 값이 클수록 → 거의 안 가림

	float      m_BarnDoorLength = 20.0f;		// 값이 클수록 
												// 빛이 더 멀리서 차단됨
												// 에지가 더 날카로워짐
};

