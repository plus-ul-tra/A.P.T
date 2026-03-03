#include "pch.h"
#include "GameDataRepository.h"

#include "CsvParser.h"

ItemCategory ParseCategory(const std::string& value)
{
	if (value == "Currency")  return ItemCategory::Currency;
	if (value == "Healing")   return ItemCategory::Healing;
	if (value == "Equipment") return ItemCategory::Equipment;
	if (value == "Throwable") return ItemCategory::Throwable;

	return ItemCategory::Currency;
}

bool GameDataRepository::LoadFromFiles(const DataSheetPaths& path, std::string* errorOut)
{
	if (!LoadItemsFromFile(path.itemsPath, errorOut))
	{
		return false;
	}

	if (!LoadDropTablesFromFile(path.dropTablesPath, errorOut))
	{
		return false;
	}


	if (!LoadEnemiesFromFile(path.enemiesPath, errorOut))
	{
		return false;
	}


	return true;
}

const ItemDefinition* GameDataRepository::GetItem(int index) const
{
	auto it = m_Items.find(index);
	
	if (it == m_Items.end())
	{
		return nullptr;
	}

	return &it->second;
}

const EnemyDefinition* GameDataRepository::GetEnemy(int id) const
{
	auto it = m_Enemies.find(id);

	if (it == m_Enemies.end())
	{
		return nullptr;
	}

	return &it->second;
}

const DropTableDefinition* GameDataRepository::GetDropTable(int difficultyGroup) const
{
	auto it = m_DropTables.find(difficultyGroup);

	if (it == m_DropTables.end())
	{
		return nullptr;
	}

	return &it->second;
}

std::vector<const ItemDefinition*> GameDataRepository::GetItemsByCategory(ItemCategory category, int maxDifficulty) const
{
	std::vector<const ItemDefinition*> result;
	for (const auto& [index, item] : m_Items)
	{
		if (item.category != category)
		{
			continue;
		}

		if (item.difficultyGroup > maxDifficulty)
		{
			continue;
		}

		result.push_back(&item);
	}

	return result;
}

bool GameDataRepository::LoadItemsFromFile(const std::string& path, std::string* errorOut)
{
	if (path.empty())
	{
		if (errorOut)
		{
			*errorOut = "Items Sheet path is empty";
		}

		return false;
	}

	m_Items.clear();
	std::vector<std::vector<std::string>> rows;
	HeaderMap header;
	if (!Load(path, rows, header, errorOut))
	{
		return false;
	}

	for (const auto& row : rows)
	{
		ItemDefinition item{};
		item.index					= ParseInt(GetField(row, header, "ItemKey"), 0);
		item.category				= static_cast<ItemCategory>(ParseInt(GetField(row, header, "ItemType")));		
		item.name					= GetField(row, header, "한글 이름");
		item.description			= GetField(row, header, "description");			//아이템 설명
		item.iconPath				= GetField(row, header, "IconPath");
		item.infoPath				= GetField(row, header, "InfoPath");
		item.meshPath				= GetField(row, header, "MeshPath");
		item.equipMeshPath			= GetField(row, header, "EquipMeshPath");
		item.basePrice				= ParseInt(GetField(row, header, "Price"), 0);
		item.difficultyGroup		= ParseInt(GetField(row, header, "DifficultyGroup"), 1);

		item.diceRoll				= ParseInt(GetField(row, header, "DiceRollCount"), 0);			
		item.diceType				= ParseInt(GetField(row, header, "DiceType"), 0);			
		item.baseModifier			= ParseInt(GetField(row, header, "BaseModifier"), 0);

		item.constitutionModifier	= ParseInt(GetField(row, header, "ConstitutionModifier"), 0);
		item.strengthModifier		= ParseInt(GetField(row, header, "StrengthModifier"), 0);
		item.agilityModifier		= ParseInt(GetField(row, header, "DexterityModifier"), 0);
		item.senseModifier			= ParseInt(GetField(row, header, "PerceptionModifier"), 0);
		item.skillModifier			= ParseInt(GetField(row, header, "SkillModifier"), 0);
		item.defenseBonus			= ParseInt(GetField(row, header, "defenseBonus"), 0);

		item.throwRange				= ParseInt(GetField(row, header, "Range"), 0);
		item.range					= ParseInt(GetField(row, header, "Range"), 0);
		item.actionPointCost		= ParseInt(GetField(row, header, "ConsumeActionPoints"), 1);

		if (item.index != 0)
		{
			m_Items[item.index] = std::move(item);
		}
	}

	return true;
}

bool GameDataRepository::LoadEnemiesFromFile(const std::string& path, std::string* errorOut)
{
	if (path.empty())
	{
		if (errorOut)
		{
			*errorOut = "Enemies sheet path is empty.";
		}
		return false;
	}

	m_Enemies.clear();
	std::vector<std::vector<std::string>> rows;
	HeaderMap header;
	if (!Load(path, rows, header, errorOut))
	{
		return false;
	}

	for (const auto& row : rows)
	{
		EnemyDefinition enemy{};
		enemy.id				 = ParseInt(GetField(row, header, "id"), 0);
		enemy.name				 = GetField(row, header, "name");
		enemy.type				 = ParseInt(GetField(row, header, "type"), 0);
		enemy.health			 = ParseInt(GetField(row, header, "health"), 0);
		enemy.initiativeModifier = ParseInt(GetField(row, header, "initiativeModifier"), 0);
		enemy.defense			 = ParseInt(GetField(row, header, "defense"), 0);
		enemy.accuracyModifier   = ParseInt(GetField(row, header, "accuracyModifier"), 0);
		enemy.damageDiceCount    = ParseInt(GetField(row, header, "damageDiceCount"), 0);
		enemy.damageDiceValue    = ParseInt(GetField(row, header, "damageDiceValue"), 0);
		enemy.damageBonus        = ParseInt(GetField(row, header, "damageBonus"), 0);
		enemy.range				 = ParseInt(GetField(row, header, "range"), 0);
		enemy.moveDistance		 = ParseInt(GetField(row, header, "moveDistance"), 0);
		enemy.sightDistance		 = ParseFloat(GetField(row, header, "sightDistance"), 0.0f);
		enemy.sightAngle		 = ParseFloat(GetField(row, header, "sightAngle"), 0.0f);
		enemy.difficultyGroup	 = ParseInt(GetField(row, header, "difficultyGroup"), 1);
		enemy.dropTableGroup	 = ParseInt(GetField(row, header, "dropTableGroup"), enemy.difficultyGroup);

		if (enemy.id != 0)
		{
			m_Enemies[enemy.id] = std::move(enemy);
		}
	}

	return true;
}

bool GameDataRepository::LoadDropTablesFromFile(const std::string& path, std::string* errorOut)
{
	if (path.empty())
	{
		if (errorOut)
		{
			*errorOut = "Drop tables sheet path is empty.";
		}
		return false;
	}

	m_DropTables.clear();
	std::vector<std::vector<std::string>> rows;
	HeaderMap header;
	if (!Load(path, rows, header, errorOut))
	{
		return false;
	}

	for (const auto& row : rows)
	{
		const int difficultyGroup = ParseInt(GetField(row, header, "difficultyGroup"), 1);

		auto& table = m_DropTables[difficultyGroup];
		table.difficultyGroup = difficultyGroup;

		for (int index = 1; index <= 10; ++index)
		{
			const std::string keyField = "DropItemKey" + std::to_string(index);
			const std::string qtyField = "Quantity" + std::to_string(index);

			const int itemIndex = ParseInt(GetField(row, header, keyField), 0);
			if (itemIndex <= 0)
			{
				continue;
			}

			DropEntry drop{};
			drop.itemIndex = itemIndex;
			drop.weight = ParseFloat(GetField(row, header, qtyField), 0.0f);

			if (index == 1)
			{
				drop.minQuantity = ParseInt(GetField(row, header, "DropGoldMin"), 0);
				drop.maxQuantity = ParseInt(GetField(row, header, "DropGoldMax"), 0);
			}

			if (drop.weight > 0.0f)
			{
				table.entries.push_back(drop);
			}
		}
	}

	return true;
}
