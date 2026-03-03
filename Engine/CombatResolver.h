#pragma once

#include <string>

class DiceSystem;
class LogSystem;

enum class HitResult
{
	Miss,
	Hit,
	Critical
};

struct AttackProfile
{
	int attackModifier = 0;
	int minDamage      = 0;
	int maxDamage      = 0;
	int bonusDamage	   = 0;
	int damageDiceCount = 0;
	int damageDiceSides = 0;
	int damageModifier = 0;
	bool allowCritical = true;
	bool autoFailOnOne = false;
	std::string attackerName;
	std::string targetName;
	std::string itemName;
	bool isThrow       = false;
	bool targetDied    = false;
};

struct DefenseProfile
{
	int defense = 0;
};

struct CombatRollResult
{
	HitResult hit = HitResult::Miss;
	int roll   = 0;
	int total  = 0;
	int damage = 0;
};

class CombatResolver
{
public:
	CombatRollResult ResolveAttack(const AttackProfile& attack,
								   const DefenseProfile& defense,
								   DiceSystem& diceSystem,
								   LogSystem* logger = nullptr) const;
};

