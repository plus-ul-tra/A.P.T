#pragma once
#include "Component.h"

class FloodSystemComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "FloodSystemComponent";
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const float& GetTurnTimeLimitSeconds() const			     { return m_TurnTimeLimitSeconds;	   }
	void		 SetTurnTimeLimitSeconds(const float& seconds)   { m_TurnTimeLimitSeconds = seconds;   }
																  									   
	const float& GetWaterLevel() const						     { return m_WaterLevel;				   }
	const float& GetWaterRisePerSecond() const				     { return m_WaterRisePerSecond;		   }
	void		 SetWaterRisePerSecond(const float& value)	     { m_WaterRisePerSecond = value;	   }
																  									   
	const float& GetRiseIntervalSeconds() const				     { return m_RiseIntervalSeconds;	   }
	void		 SetRiseIntervalSeconds(const float& seconds)    { m_RiseIntervalSeconds = seconds;	   }
																  									   
	const float& GetRiseStepAmount() const					     { return m_RiseStepAmount;			   }
	void		 SetRiseStepAmount(const float& value)		     { m_RiseStepAmount = value;		   }
																  									   
	const float& GetCorrectionMin() const					     { return m_CorrectionMin;			   }
	void		 SetCorrectionMin(const float& value)		     { m_CorrectionMin = value;			   }
																  									   
	const float& GetCorrectionMax() const				         { return m_CorrectionMax;			   }
	void		 SetCorrectionMax(const float& value)		     { m_CorrectionMax = value;			   }
																  									   
	const float& GetCorrectionDistanceMax() const			     { return m_CorrectionDistanceMax;	   }
	void		 SetCorrectionDistanceMax(const float& value)    { m_CorrectionDistanceMax = value;	   }

	const float& GetShopCorrectionMultiplier() const		     { return m_ShopCorrectionMultiplier;  }
	void		 SetShopCorrectionMultiplier(const float& value) { m_ShopCorrectionMultiplier = value; }

	const float  GetTurnElapsed  () const					     { return m_TurnElapsed;			   }
	const float  GetTurnRemaining() const;					    
															    
	const bool&  GetGameOver     () const						 { return m_IsGameOver;			       }

private:
	void  MarkGameOver();
	float ComputeCorrectionFactor() const;
	bool  ShouldAdvance() const;
	float GetPlayerFloodHeightThreshold() const;

	float m_TurnElapsed				 = 0.0f;
	float m_TurnTimeLimitSeconds	 = 30.0f;
	float m_WaterLevel				 = 0.0f;
	float m_WaterRisePerSecond		 = 0.1f;
	float m_RiseIntervalSeconds		 = 10.0f;
	float m_RiseStepAmount			 = 0.1f;
	float m_CorrectionMin			 = 0.5f;
	float m_CorrectionMax			 = 2.0f;
	float m_CorrectionDistanceMax	 = 20.0f;
	float m_ShopCorrectionMultiplier = 1.0f;
	bool  m_IsGameOver			     = false;
};

