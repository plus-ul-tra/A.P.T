#pragma once
#include "UIComponent.h"

class FloodSystemComponent;

class FloodUIComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "FloodUIComponent";
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const float& GetDisplayedWaterLevel    () const { return m_DisplayedWaterLevel;	   }
	const float& GetDisplayedTimeRemaining () const { return m_DisplayedTimeRemaining; }
	const bool&  GetDisplayedGameOver	   () const { return m_DisplayedGameOver;	   }

private:
	FloodSystemComponent* m_FloodSystem = nullptr;
	float m_DisplayedWaterLevel         = 0.0f;
	float m_DisplayedTimeRemaining      = 0.0f;
	bool  m_DisplayedGameOver	        = false;
};

