#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

enum class ItemCategory
{
	Currency,	// 재화
	Equipment,
	Healing,
	Throwable
};

struct ItemDefinition
{
	int			 index					= 0;
	ItemCategory category				= ItemCategory::Currency;
	std::string  name;
	std::string  description;
	std::string  iconPath;
	std::string  infoPath;
	std::string  meshPath;
	std::string  equipMeshPath;
				 
	int			 basePrice				= 0;
	int			 difficultyGroup		= 1;
				 										 
	int			 diceRoll				= 0;
	int			 diceType				= 0;
	int			 baseModifier			= 0;			//기본 데미지 보정
				 
	int			 constitutionModifier	= 0;
	int			 strengthModifier		= 0;
	int			 agilityModifier		= 0;
	int			 senseModifier			= 0;
	int			 skillModifier			= 0;
	int			 defenseBonus			= 0;
				 
	int			 throwRange				= 0;			// 투척
	int			 range					= 0;			// 근접
	int			 actionPointCost = 1;					// 행동 포인트 소모
};

struct EnemyDefinition
{
    int         id				   = 0;
    std::string name;
	int         type			   = 0;
    int         health			   = 0;
    int         initiativeModifier = 0;
    int         defense			   = 0;
    int         accuracyModifier   = 0;

    int         damageDiceCount    = 0;
    int         damageDiceValue    = 0;
    int         damageBonus		   = 0;

    int         range			   = 0;
    int         moveDistance	   = 0;
    float       sightDistance	   = 0.0f;
    float       sightAngle		   = 0.0f;
    int         difficultyGroup    = 1;
    int         dropTableGroup     = 1;
};

struct DropEntry
{
	int   itemIndex = 0;
	float weight	= 0.0f;
	int   minQuantity = 0;
	int   maxQuantity = 0;
};

struct DropTableDefinition
{
	int difficultyGroup = 1;
	std::vector<DropEntry> entries;
};

struct DataSheetPaths
{
	std::string itemsPath;
	std::string enemiesPath;
	std::string dropTablesPath;
};

class GameDataRepository
{
public:
	bool LoadFromFiles(const DataSheetPaths& path, std::string* errorOut = nullptr);

	const ItemDefinition*	   GetItem	   (int index)			 const;
	const EnemyDefinition*	   GetEnemy	   (int id)				 const;
	const DropTableDefinition* GetDropTable(int difficultyGroup) const;

	const std::unordered_map<int, ItemDefinition>&		GetItems     () const { return m_Items;		 }
	const std::unordered_map<int, EnemyDefinition>&		GetEnemies   () const { return m_Enemies;	 }
	const std::unordered_map<int, DropTableDefinition>& GetDropTables() const { return m_DropTables; }

	std::vector<const ItemDefinition*> GetItemsByCategory(ItemCategory category, int maxDifficulty) const;

private:
	bool LoadItemsFromFile	   (const std::string& path, std::string* errorOut);
	bool LoadEnemiesFromFile   (const std::string& path, std::string* errorOut);
	bool LoadDropTablesFromFile(const std::string& path, std::string* errorOut);

	std::unordered_map<int, ItemDefinition>		 m_Items;
	std::unordered_map<int, EnemyDefinition>	 m_Enemies;
	std::unordered_map<int, DropTableDefinition> m_DropTables;
};

