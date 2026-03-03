#pragma once
#include "StatComponent.h"

class EnemyStatComponent : public StatComponent
{
public:
	static constexpr const char* StaticTypeName = "EnemyStatComponent";
	const char* GetTypeName() const override;

	EnemyStatComponent() = default;
	~EnemyStatComponent() override = default;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;
	void ResetCurrentHPToInitial();

	const int&   GetDefense() const						 { return m_Defense;			 }
	void	     SetDefense(const int& value)			 { m_Defense = value;			 }
			     
	const int&   GetInitiativeModifier() const			 { return m_InitiativeModifier;  }
	void	     SetInitiativeModifier(const int& value) { m_InitiativeModifier = value; }
			     
	const int&   GetAccuracyModifier() const		     { return m_AccuracyModifier;    }
	void	     SetAccuracyModifier(const int& value)   { m_AccuracyModifier = value;   }

	const int&   GetEnemyType() const					 { return m_EnemyType;			 }
	void	     SetEnemyType(const int& value)			 { m_EnemyType = value;			 }


	const int&   GetDiceRollCount() const				 { return m_DiceRollCount;		 }
	void	     SetDiceRollCount(const int& value)		 { m_DiceRollCount = value;		 }

	const int&   GetMaxDiceValue() const				 { return m_MaxDiceValue;		 }
	void	     SetMaxDiceValue(const int& value)		 { m_MaxDiceValue = value;		 }
					     
	const int&   GetAttackRange() const					 { return m_AttackRange; }
	void	     SetAttackRange(const int& value)		 { m_AttackRange = value; }

	const float& GetSightDistance() const			     { return m_SightDistance;       }
	void		 SetSightDistance(const float& value)    { m_SightDistance = value;      }
													     								 
	const float& GetSightAngle() const				     { return m_SightAngle;	         }
	void		 SetSightAngle(const float& value)       { m_SightAngle = value;	     }
													     								 
	const int&   GetDifficultyGroup() const			     { return m_DifficultyGroup;     }
	void	     SetDifficultyGroup(const int& value)    { m_DifficultyGroup = value;    }

	void	   SetInitialHP(const int& value) { m_InitialHP = value; }
	const int& GetInitialHP() const { return m_InitialHP; }

private:
	int   m_Defense			   = 0;
	int   m_InitiativeModifier = 0;
	int   m_AccuracyModifier   = 0;
	int   m_EnemyType = 0;
	int   m_DiceRollCount      = 0;
	int   m_MaxDiceValue       = 0;
	int   m_AttackRange		   = 1;
	float m_SightDistance      = 5.0f;
	float m_SightAngle         = 90.0f;
	int   m_DifficultyGroup    = 1;

	int   m_InitialHP = 100;   // 체력 주의
	bool  m_InitialHPCaptured = false;

};

