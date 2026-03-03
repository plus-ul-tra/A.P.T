#include "pch.h"
#include "ShopRoller.h"
#include "GameDataRepository.h"
#include "DiceSystem.h"
#include "LogSystem.h"
#include <algorithm>

                                // floor : 난이도 그룹
ShopStock ShopRoller::RollStock(int floor, 
                                const GameDataRepository& repository,
                                DiceSystem& diceSystem,
                                const std::vector<int>& excludedItems,
                                LogSystem* logger) const
{
	ShopStock stock{};

    auto equipment = repository.GetItemsByCategory(ItemCategory::Equipment, floor);
    auto healing   = repository.GetItemsByCategory(ItemCategory::Healing,   floor);
    auto throwable = repository.GetItemsByCategory(ItemCategory::Throwable, floor);

    std::vector<int> used;
    for (int i = 0; i < 2; ++i)
    {
        if (auto choice = PickRandomItem(equipment, diceSystem, used, excludedItems))
        {
            stock.equipmentItems.push_back(*choice);
        }
    }

    std::vector<const ItemDefinition*> consumables = healing;
    consumables.insert(consumables.end(), throwable.begin(), throwable.end());
    if (auto choice = PickRandomItem(equipment, diceSystem, used, excludedItems))
    {
        stock.consumableItems.push_back(*choice);
    }

    if (logger)
    {
//      logger->Add(LogChannel::Shop, "Shop Stock Rolled. Equip = " + std::to_string(stock.equipmentItems.size())
//          + ", Comsumable = " + std::to_string(stock.consumableItems.size()));
        logger->Add(LogChannel::Shop, "Shop Stock Rolled. Equip = " + repository.GetItem(stock.equipmentItems.front())->name
            + repository.GetItem(stock.equipmentItems.front()++)->name + ", Comsumable = " + repository.GetItem(stock.consumableItems.front())->name);
    }

		return stock;
}

std::optional<int> ShopRoller::PickRandomItem(const std::vector<const ItemDefinition*>& items, 
                                              DiceSystem& diceSystem,
                                              std::vector<int>& used,
                                              const std::vector<int>& excludedItems) const
{
    if (items.empty())
        return std::nullopt;

    DiceConfig rollConfig{ 1, static_cast<int>(items.size()), 0 };
    const int index = diceSystem.RollTotal(rollConfig, RandomDomain::Shop) - 1;
    const ItemDefinition* item = items[static_cast<int>(index)];

    if (!item)
        return std::nullopt;

    if (std::find(used.begin(), used.end(), item->index) != used.end())
        return std::nullopt;

    if (std::find(excludedItems.begin(), excludedItems.end(), item->index) != excludedItems.end())
        return std::nullopt;

    used.push_back(item->index);
    return item->index;
}
