#include "pch.h"
#include "LootRoller.h"
#include "GameDataRepository.h"
#include "DiceSystem.h"
#include "LogSystem.h"

float ComputeTotalWeight(const std::vector<DropEntry>& entries)
{
    float total = 0.0f;
    for (const auto& entry : entries)
    {
        total += entry.weight;
    }

    return total;
}

int ScaleWeight(float weight, int scale)
{
	if (weight <= 0.0f)
	{
		return 0;
	}
	const float scaled  = weight * static_cast<float>(scale);
	const int   rounded = static_cast<int>(scaled + 0.5f);
	return rounded < 1 ? 1 : rounded;
}

std::optional<LootRollResult> LootRoller::RollDrop(const EnemyDefinition& enemy,
                                                   const GameDataRepository& repository,
                                                   DiceSystem& diceSystem, 
                                                   LogSystem* logger) const
{
    const DropTableDefinition* table = repository.GetDropTable(enemy.dropTableGroup);
    if (!table || table->entries.empty())
    {
        if (logger)
        {
            logger->Add(LogChannel::Loot, "Drop Table Missing For Enemy : " + enemy.name);
        }
        return std::nullopt;
    }

    const float totalWeight = ComputeTotalWeight(table->entries);
    if (totalWeight <= 0.0f)
    {
        return std::nullopt;
    }

	constexpr int weightScale = 1000;
	const int totalWeightScaled = ScaleWeight(totalWeight, weightScale);
	if (totalWeightScaled <= 0)
	{
		return std::nullopt;
	}

    DiceConfig rollConfig{ 1, totalWeightScaled, 0 };
    const int roll = diceSystem.RollTotal(rollConfig, RandomDomain::Loot);
    int cursor = 0;
    for (const auto& entry : table->entries)
    {
        cursor += ScaleWeight(entry.weight, weightScale);
        if (roll <= cursor)
        {
            //if (enemy.id == 0)
            //{
            //    return std::nullopt;
            //}

            if (logger)
            {
				LogContext context{};
				context.actor = enemy.name;
				if (const ItemDefinition* item = repository.GetItem(entry.itemIndex))
				{
					context.item = item->name;
				}
				else
				{
					context.item = std::to_string(entry.itemIndex);
				}
				logger->AddLocalized(LogChannel::Loot, LogMessageType::Drop, context);
            }

            return LootRollResult{ entry.itemIndex };
        }
    }

    return std::nullopt;
}
