#pragma once
#include "StatComponent.h"

class PlayerStatComponent : public StatComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerStatComponent";
	const char* GetTypeName() const override;

	PlayerStatComponent() = default;
	~PlayerStatComponent() override = default;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const int&   GetHealth() const						 { return m_Health;			   }
	void	     SetHealth(const int& value);
			     
	const int&   GetStrength() const					 { return m_Strength;		   }
	void		 SetStrength(const int& value);
			     
	const int&   GetAgility() const				         { return m_Agility;           }
	void	     SetAgility(const int& value);
			     
	const int&   GetSense() const						 { return m_Sense;			   }
	void	     SetSense(const int& value);
			     
	const int&   GetSkill() const						 { return m_Skill;		       }
	void	     SetSkill(const int& value);

	const int    CalculateStatModifier(int statValue) const;

	const int    GetTotalHealth() const { return m_Health + m_EquipmentHealthBonus; }
	const int    GetTotalStrength() const { return m_Strength + m_EquipmentStrengthBonus; }
	const int    GetTotalAgility() const { return m_Agility + m_EquipmentAgilityBonus; }
	const int    GetTotalSense() const { return m_Sense + m_EquipmentSenseBonus; }
	const int    GetTotalSkill() const { return m_Skill + m_EquipmentSkillBonus; }

	const int    GetCalculatedHealthModifier() const { return CalculateStatModifier(GetTotalHealth()); }
	const int    GetCalculatedStrengthModifier() const { return CalculateStatModifier(GetTotalStrength()); }
	const int    GetCalculatedAgilityModifier() const { return CalculateStatModifier(GetTotalAgility()); }
	const int    GetCalculatedSenseModifier() const { return CalculateStatModifier(GetTotalSense()); }
	const int    GetCalculatedSkillModifier() const { return CalculateStatModifier(GetTotalSkill()); }

	float        GetShopDiscountRate          () const;
	const int&   GetEquipmentHealthBonus() const { return m_EquipmentHealthBonus; }
	void         SetEquipmentHealthBonus(const int& value);
	const int&   GetEquipmentStrengthBonus() const { return m_EquipmentStrengthBonus; }
	void         SetEquipmentStrengthBonus(const int& value);
	const int&   GetEquipmentAgilityBonus() const { return m_EquipmentAgilityBonus; }
	void         SetEquipmentAgilityBonus(const int& value);
	const int&   GetEquipmentSenseBonus() const { return m_EquipmentSenseBonus; }
	void         SetEquipmentSenseBonus(const int& value);
	const int&   GetEquipmentSkillBonus() const { return m_EquipmentSkillBonus; }
	void         SetEquipmentSkillBonus(const int& value);
	void         SetEquipmentDefenseBonus	  (const int& value);
	const int&   GetEquipmentDefenseBonus     () const { return m_EquipmentDefenseBonus;			  }
	const int    GetMaxHealthForFloor	 	  (int currentFloor) const;
	const int    GetDefense				      () const { return GetCalculatedSenseModifier() + m_EquipmentDefenseBonus; }

private:
	void DispatchStatChangedEvent();

	int   m_Health				   = 12;
	int   m_Strength			   = 12;
	int   m_Agility				   = 12;
	int   m_Sense				   = 12;
	int   m_Skill				   = 12;	 
	int   m_EquipmentHealthBonus   = 0;
	int   m_EquipmentStrengthBonus = 0;
	int   m_EquipmentAgilityBonus  = 0;
	int   m_EquipmentSenseBonus    = 0;
	int   m_EquipmentSkillBonus    = 0;
	int   m_EquipmentDefenseBonus  = 0;
};

