#pragma once

#include <vector>

struct CombatEnterEvent
{
	int initiatorId = 0;
	int targetId    = 0;
};

struct CombatExitEvent
{
};

struct CombatInitiativeBuiltEvent
{
	const std::vector<int>* initiativeOrder = nullptr;
};

struct CombatTurnAdvancedEvent
{
	int actorId = 0;
};

struct CombatAIRequestEvent
{
	int actorId = 0;
};