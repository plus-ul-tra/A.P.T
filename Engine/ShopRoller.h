#pragma once

#include <vector>
#include <optional>

class  DiceSystem;
class  LogSystem;
class  GameDataRepository;
struct ItemDefinition;

struct ShopStock		//재고
{
	std::vector<int> equipmentItems;
	std::vector<int> consumableItems;
};

class ShopRoller
{
public:
	ShopStock RollStock(int floor, 
						const GameDataRepository& repository,
						DiceSystem& diceSystem,
						const std::vector<int>& excludedItems,
						LogSystem* logger = nullptr) const;

private:
	std::optional<int> PickRandomItem(const std::vector<const ItemDefinition*>& items, 
									  DiceSystem& diceSystem,
									  std::vector<int>& used,
									  const std::vector<int>& excludedItems) const;
};

