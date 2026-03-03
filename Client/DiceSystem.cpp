#include "pch.h"
#include "DiceSystem.h"

DiceRoll DiceSystem::Roll(const DiceConfig& config, RandomDomain domain)
{
    DiceRoll result;
    result.faces.reserve(config.count);
    result.bonus = config.bonus;

    for (int i = 0; i < config.count; ++i)
    {
        int face = m_Rng.NextInt(1, config.sides, domain);
        result.faces.push_back(face);
        result.total += face;
    }

    result.total += config.bonus;

    return result;
}

int DiceSystem::RollTotal(const DiceConfig& config, RandomDomain domain)
{
    return Roll(config, domain).total;
}

CombatStatRoll DiceSystem::RollCombatStat(RandomDomain domain)
{
    CombatStatRoll result;

    result.d20 = m_Rng.NextInt(1, 20, domain);
    result.config = GetStatConfigForD20(result.d20);
    result.roll = Roll(result.config, domain);

    return result;
}

// 횟수, 최댓값
DiceConfig DiceSystem::GetStatConfigForD20(int d20) const
{
    if (d20 < 0)  d20 = 0;
    if (d20 > 20) d20 = 20;

    if (d20 <= 5)
        return { 2, 12 };
    if (d20 <= 10)
        return { 3, 8 };
    if (d20 <= 15)
        return { 4, 6 };
    if (d20 <= 20)
        return { 6, 4 };

    return { 1, 6 };    //fallback 값 실제로 사용되지 않음
}
