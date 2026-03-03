#pragma once

#include <vector>
#include <cstddef>
#include <unordered_set>

#include "GameState.h"

class EventDispatcher;
class DiceSystem;
class LogSystem;
class CombatResolver;
class AIController;

enum class AttackAreaType
{
	SingleTarget,
	Radius,
	Cone
};

enum class AttackType
{
	Melee,
	Throw
};

struct AttackRequest
{
	int actorId = 0;
	std::vector<int> targetIds;
	AttackAreaType areaType = AttackAreaType::SingleTarget;
	float radius = 0.0f;
	float coneAngle = 0.0f;
	AttackType attackType = AttackType::Melee;
};

struct CombatantSnapshot
{
	int  actorId = 0;
	int  initiativeBonus = 0;
	bool isPlayer = false;
};

struct InitiativeEntry
{
	int actorId = 0;
	int initiative = 0;
};

class CombatManager
{
	friend class GameManager;
	friend class EnemyComponent;
public:
	CombatManager(CombatResolver& combatResolver,
		DiceSystem& diceSystem,
		LogSystem* logger = nullptr);

	void HandlePlayerAttack(const AttackRequest& request);
	void TickAI(AIController& controller, float deltaTime);
	void EnterBattle(int initiatorId, int targetId);
	void ExitBattle();
	void SetCombatants(const std::vector<CombatantSnapshot>& combatants);
	bool AddCombatants(const std::vector<CombatantSnapshot>& combatants);
	void UpdateBattleOutcome(bool playerAlive, bool enemiesRemaining);
	void ResetSessionState();
	void SetEventDispatcher(EventDispatcher* dispatcher) { m_EventDispatcher = dispatcher; }

	Battle GetState() const { return m_State; }
	const std::vector<int>& GetInitiativeOrder() const { return m_InitiativeOrder; }
	int GetCurrentActorId() const;
	std::size_t GetCurrentTurnIndex() const { return m_CurrentTurnIndex; }
	bool IsActorInBattle(int actorId) const;
	void AdvanceTurnToNextPlayer();
	bool AdvanceTurnToNextEnemyOrPlayer();
	void HandlePlayerDiceDecisionRequested();
	void HandlePlayerDiceStatRollRequested();
	void HandlePlayerDiceContinueRequested();
	bool IsDiceFlowActive() const { return m_DiceFlowActive; }

private:
	void BuildInitiativeOrder();
	void FinalizeBattleStart();
	bool IsPlayerActorId(int actorId) const;
	bool CanAct(int actorId) const;
	void AdvanceTurn();

	Battle		 m_State = Battle::NonBattle;
	std::vector<CombatantSnapshot> m_Combatants;
	std::vector<int> m_InitiativeOrder;
	std::unordered_set<int> m_ActorIdsInBattle;
	std::size_t		 m_CurrentTurnIndex = 0;
	bool m_DiceFlowActive = false;
	bool m_PlayerDecisionReady = false;
	int m_PlayerDecisionD20 = 0;
	int m_PlayerInitiativeDiceBonus = 0;
	int m_PlayerInitiativeTotal = 0;
	std::vector<int> m_PlayerDecisionFaces;
	std::vector<std::vector<int>> m_PlayerStatRollFacesHistory;
	std::vector<int> m_PlayerStatRollTotalsHistory;
	int m_PlayerActorId = 1;
	std::vector<InitiativeEntry> m_PendingInitiativeEntries;

	CombatResolver& m_Resolver;
	DiceSystem& m_DiceSystem;
	LogSystem* m_LogSystem = nullptr;
	EventDispatcher* m_EventDispatcher = nullptr;
};

