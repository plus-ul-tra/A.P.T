#pragma once

#include <optional>

class  DiceSystem;
class  LogSystem;
class  GameDataRepository;
struct EnemyDefinition;

struct LootRollResult
{
	int itemIndex = 0;
};

class LootRoller
{
public:
	std::optional<LootRollResult> RollDrop(const EnemyDefinition& enemy,
										   const GameDataRepository& repository,
								           DiceSystem& diceSystem,
										   LogSystem* logger = nullptr) const;
};

