#pragma once
#include "Component.h"

class StatComponent : public Component
{
public:
	static constexpr const char* StaticTypeName = "StatComponent";
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const int&   GetCurrentHP() const				 { return m_CurrentHP;		 }
	void	     SetCurrentHP(const int& value)		 { m_CurrentHP = value;		 }	
	void         ModifyHP    (int amount);
	bool         IsDead      () const				 {  return m_CurrentHP <= 0; }
																				 
	const float& GetRange() const				     { return m_Range;		     }
	void		 SetRange(const float& value)	     { m_Range = value;		     }
																			     
	const float& GetMoveDistance() const			 { return m_MoveDistance;    }
	void         SetMoveDistance(const float& value) { m_MoveDistance = value;   }

protected:
	int   m_CurrentHP      = 100;
	float m_Range		   = 0.0f;
	float m_MoveDistance   = 0.0f;
};

