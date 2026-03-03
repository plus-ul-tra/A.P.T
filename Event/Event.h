#pragma once
#include "Windows.h"
#include <string>
#include <vector>

namespace Events
{
	struct KeyEvent
	{
		char key;
	};

	struct MouseState
	{
		POINT  pos{ 0, 0 };

		bool   leftPressed{ false };
		bool   rightPressed{ false };
		mutable bool   handled{ false };
	};

	struct SceneChangeRequest
	{
		std::string name;
	};

	struct TurnChanged
	{
		int turn = 0;
	};

	struct ActorEvent
	{
		int actorId = 0;
	};

	struct DiceRollEvent
	{
		int value = 0;
		int diceCount = 0;
		int diceSides = 0;
		int bonus = 0;
		std::string context;
		bool isTotal = false;
		std::vector<int> faces;
	};

	struct DiceDecisionFaceEvent
	{
		int index = 0;
		int value = 0;
		int selectedD20 = 0;
		std::string context;
	};

	struct DiceInitiativeResolvedEvent
	{
		std::vector<int> d20Faces;
		int selectedD20 = 0;
		int initiativeBonus = 0;
		int initiativeTotal = 0;
	};

	struct DiceStatResolvedEvent
	{
		int selectedD20 = 0;
		int diceCount = 0;
		int diceSides = 0;
		std::vector<int> faces;
		std::vector<std::vector<int>> facesHistory;
		std::vector<int> totalsHistory;
		int total = 0;
		int initiativeBonus = 0;
		int initiativeTotal = 0;
	};


	struct CombatNumberPopupEvent
	{
		int instigatorActorId = 0;
		int targetActorId = 0;
		int hpDelta = 0;
		bool isMiss = false;
		bool isCritical = false;
	};

	struct GoldAcquiredEvent
	{
		int amount = 0;
		int total = 0;
	};

	struct PlayerStatChangedEvent
	{
		int health = 0;
		int strength = 0;
		int agility = 0;
		int sense = 0;
		int skill = 0;
		int equipmentHealthBonus = 0;
		int equipmentStrengthBonus = 0;
		int equipmentAgilityBonus = 0;
		int equipmentSenseBonus = 0;
		int equipmentSkillBonus = 0;
		int equipmentDefenseBonus = 0;
		int defense = 0;
	};

	struct EnemyHoveredEvent
	{
		int actorId   = 0;
		int currentHp = 0;
		int maxHp     = 0;
		bool hasEnemy = false;
	};
}