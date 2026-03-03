#include "pch.h"
#include "CombatResolver.h"
#include "DiceSystem.h"
#include "LogSystem.h"

#include <algorithm>
#include <iostream>

CombatRollResult CombatResolver::ResolveAttack(const AttackProfile& attack, 
                                               const DefenseProfile& defense, 
                                               DiceSystem& diceSystem,
                                               LogSystem* logger) const
{
    CombatRollResult result{};

    const CombatStatRoll combatStat = diceSystem.RollCombatStat(RandomDomain::Combat);
    result.roll  = combatStat.d20;
    result.total = result.roll + attack.attackModifier;

    const bool critical = attack.allowCritical && result.roll == 20;
    const bool autoFail = attack.autoFailOnOne && result.roll == 1;
	const int profileDiceCount = (attack.damageDiceCount > 0) ? attack.damageDiceCount : 1;
	const int profileDiceSides = (attack.damageDiceSides > 0)
		? attack.damageDiceSides
		: std::max(1, std::max(attack.minDamage, attack.maxDamage));

	std::cout << "[Combat] Attack profile: modifier=" << attack.attackModifier
		<< " diceRoll=" << profileDiceCount
		<< " diceType=" << profileDiceSides 
		<< " targetDefense=" << defense.defense << std::endl;

    if (autoFail)
    { 
        result.hit = HitResult::Miss;
    }
    else if (critical || result.total >= defense.defense)
    {
        result.hit = critical ? HitResult::Critical : HitResult::Hit;

		DiceConfig config{};
		int damageModifier = attack.damageModifier;
		if (attack.damageDiceCount > 0 && attack.damageDiceSides > 0)
		{
			config = DiceConfig{ attack.damageDiceCount, attack.damageDiceSides, 0 };
		}
		else
		{
			int maxDamage = std::max(attack.minDamage, attack.maxDamage);
			if (maxDamage <= 0)
			{
				maxDamage = 4;
			}

			config = DiceConfig{ 1, maxDamage, 0 };
			if (damageModifier == 0)
			{
				damageModifier = attack.attackModifier;
			}
		}

		const int rolledDamage = diceSystem.RollTotal(config, RandomDomain::Combat);
		result.damage = rolledDamage + damageModifier;


        if (critical)
        {
            result.damage *= 2;
        }
    }
	else
	{
		result.hit = HitResult::Miss;
	}

	std::cout << "[Combat] Roll=" << result.roll
		<< " Total=" << result.total
		<< " Defense=" << defense.defense
		<< " Result=";
	switch (result.hit)
	{
	case HitResult::Hit:
		std::cout << "Hit";
		break;
	case HitResult::Critical:
		std::cout << "Critical";
		break;
	default:
		std::cout << "Miss";
		break;
	}
	std::cout << " Damage=" << result.damage;
	if (critical)
	{
		std::cout << " (critical)";
	}
	if (autoFail)
	{
		std::cout << " (auto-fail)";
	}
	std::cout << std::endl;

    if (logger)
    {
		LogContext context{};
		context.actor = attack.attackerName.empty() ? "Actor" : attack.attackerName;
		context.target = attack.targetName;
		context.item = attack.itemName;

		if (attack.isThrow)
		{
			logger->AddLocalized(LogChannel::Combat, LogMessageType::Throw, context);
		}

		const LogMessageType type = (result.hit == HitResult::Miss) ? LogMessageType::Miss : LogMessageType::Attack;
		logger->AddLocalized(LogChannel::Combat, type, context);

		if (result.hit != HitResult::Miss && result.damage > 0)
		{
			LogContext damageContext = context;
			damageContext.value = result.damage;
			logger->AddLocalized(LogChannel::Combat, LogMessageType::Damage, damageContext);
		}

		if (result.hit != HitResult::Miss && attack.targetDied)
		{
			LogContext deathContext{};
			deathContext.actor = attack.targetName;
			logger->AddLocalized(LogChannel::Combat, LogMessageType::Death, deathContext);
		}
	}
    
    return result;
}
