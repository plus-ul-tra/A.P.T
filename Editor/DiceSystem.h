#pragma once

#include <vector>
#include "RandomMachine.h"

struct DiceConfig
{
	int count = 1;
	int sides = 6;
	int bonus = 0;
};

struct DiceRoll
{
	std::vector<int> faces;
	int total = 0;
	int bonus = 0;
};

struct CombatStatRoll
{
	int        d20 = 0;
	DiceConfig config;
	DiceRoll   roll;
};

class DiceSystem
{
public:
	explicit DiceSystem(RandomMachine& randomMachine) : m_Rng(randomMachine) {}

	DiceRoll Roll(const DiceConfig& config, RandomDomain domain = RandomDomain::Combat);
	int RollTotal(const DiceConfig& config, RandomDomain domain = RandomDomain::Combat);

	CombatStatRoll RollCombatStat(RandomDomain domain = RandomDomain::Combat);

private:
	DiceConfig GetStatConfigForD20(int d20) const;
	RandomMachine& m_Rng;
};

