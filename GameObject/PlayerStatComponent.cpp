#include "PlayerStatComponent.h"
#include "Event.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(PlayerStatComponent, StatComponent)
REGISTER_PROPERTY(PlayerStatComponent, Health)
REGISTER_PROPERTY(PlayerStatComponent, Strength)
REGISTER_PROPERTY(PlayerStatComponent, Agility)
REGISTER_PROPERTY(PlayerStatComponent, Sense)
REGISTER_PROPERTY(PlayerStatComponent, Skill)
REGISTER_PROPERTY_READONLY(PlayerStatComponent, EquipmentHealthBonus)
REGISTER_PROPERTY_READONLY(PlayerStatComponent, EquipmentStrengthBonus)
REGISTER_PROPERTY_READONLY(PlayerStatComponent, EquipmentAgilityBonus)
REGISTER_PROPERTY_READONLY(PlayerStatComponent, EquipmentSenseBonus)
REGISTER_PROPERTY_READONLY(PlayerStatComponent, EquipmentSkillBonus)
REGISTER_PROPERTY_READONLY(PlayerStatComponent, EquipmentDefenseBonus)



void PlayerStatComponent::Update(float deltaTime)
{
}

void PlayerStatComponent::OnEvent(EventType type, const void* data)
{
}

void PlayerStatComponent::SetHealth(const int& value)
{
	if (m_Health == value)
	{
		return;
	}

	m_Health = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetStrength(const int& value)
{
	if (m_Strength == value)
	{
		return;
	}

	m_Strength = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetAgility(const int& value)
{
	if (m_Agility == value)
	{
		return;
	}

	m_Agility = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetSense(const int& value)
{
	if (m_Sense == value)
	{
		return;
	}

	m_Sense = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetSkill(const int& value)
{
	if (m_Skill == value)
	{
		return;
	}

	m_Skill = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetEquipmentDefenseBonus(const int& value)
{
	if (m_EquipmentDefenseBonus == value)
	{
		return;
	}

	m_EquipmentDefenseBonus = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetEquipmentHealthBonus(const int& value)
{
	if (m_EquipmentHealthBonus == value)
	{
		return;
	}

	m_EquipmentHealthBonus = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetEquipmentStrengthBonus(const int& value)
{
	if (m_EquipmentStrengthBonus == value)
	{
		return;
	}

	m_EquipmentStrengthBonus = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetEquipmentAgilityBonus(const int& value)
{
	if (m_EquipmentAgilityBonus == value)
	{
		return;
	}

	m_EquipmentAgilityBonus = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetEquipmentSenseBonus(const int& value)
{
	if (m_EquipmentSenseBonus == value)
	{
		return;
	}

	m_EquipmentSenseBonus = value;
	DispatchStatChangedEvent();
}

void PlayerStatComponent::SetEquipmentSkillBonus(const int& value)
{
	if (m_EquipmentSkillBonus == value)
	{
		return;
	}

	m_EquipmentSkillBonus = value;
	DispatchStatChangedEvent();
}

const int PlayerStatComponent::CalculateStatModifier(int statValue) const
{
	int diff = statValue - 12;
	return diff / 2;
}

float PlayerStatComponent::GetShopDiscountRate() const
{
	const int modifier = GetCalculatedSkillModifier();
	return static_cast<const float>(modifier) / 10.0f;
}

const int PlayerStatComponent::GetMaxHealthForFloor(int currentFloor) const
{
	return GetTotalHealth() + (GetCalculatedHealthModifier() * currentFloor);
}

void PlayerStatComponent::DispatchStatChangedEvent()
{
	Events::PlayerStatChangedEvent payload{};
	payload.health = m_Health;
	payload.strength = m_Strength;
	payload.agility = m_Agility;
	payload.sense = m_Sense;
	payload.skill = m_Skill;
	payload.equipmentHealthBonus = m_EquipmentHealthBonus;
	payload.equipmentStrengthBonus = m_EquipmentStrengthBonus;
	payload.equipmentAgilityBonus = m_EquipmentAgilityBonus;
	payload.equipmentSenseBonus = m_EquipmentSenseBonus;
	payload.equipmentSkillBonus = m_EquipmentSkillBonus;
	payload.equipmentDefenseBonus = m_EquipmentDefenseBonus;
	payload.defense = GetDefense();
	GetEventDispatcher().Dispatch(EventType::PlayerStatChanged, &payload);
}