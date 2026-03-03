#pragma once

#include "FSM.h"
#include "EventDispatcher.h"
#include "Event.h"
#include "GameState.h"
#include "GameDataRepository.h"
#include "ShopRoller.h"
#include <string>
#include <vector>

class GameManager : public IEventListener
{
public:
	// Scene 넘어갈 때 가지고 가야할 Data
	struct PlayerPersistentData
	{ // 처음엔 초기셋팅
		bool hasData = false;
		int  weaponCost = 0;
		int  attackRange = 0;
		int  actorId = 1;
		int  money = 0;
		std::vector<std::string> inventoryItemIds;

		int  currentHP = 100;
		int  health = 0;
		int  strength = 0;
		int  agility = 0;
		int  sense = 0;
		int  skill = 0;
		int  equipmentDefenseBonus = 0;
	};
	GameManager();
	~GameManager();

	void SetEventDispatcher(EventDispatcher& eventDispatcher);
	void ClearEventDispatcher();

	void Reset();

	void OnEvent(EventType type, const void* data);
	void TurnReset();

	void Initial();

	void RequestSceneChange(const std::string& name);
	void SetServices(class ServiceRegistry* services);
	void SetActiveScene(class Scene* scene);
	void SetDataSheetPaths(const DataSheetPaths& paths);
	void SetFloorSceneNames(const std::vector<std::string>& names);
	void Update(float deltaTime);

	Turn GetTurn() const { return m_Turn; }
	Phase GetPhase() const { return m_Phase; }
	ExplorationTurnState GetExplorationTurnState() const { return m_ExplorationTurnState; }
	CombatTurnState GetCombatTurnState() const { return m_CombatTurnState; }
	int  GetExplorationActiveEnemyActorId() const { return m_ExplorationActiveEnemyActorId; }
	void SetExplorationActiveEnemyActorId(int actorId);
	bool IsExplorationInputAllowed() const;
	bool IsCombatInputAllowed() const;
	bool IsShopInputAllowed() const;
	int  GetCurrentFloor() const { return m_CurrentFloor; }
	void SetTurn(Turn turn);
	void SetPhase(Phase phase);
	void SetExplorationTurnState(ExplorationTurnState state);
	void SetCombatTurnState(CombatTurnState state);

	void CapturePlayerData(class Scene* scene);
	void ApplyPlayerData(class Scene* scene);
	void ClearPlayerData();
	class CombatManager* GetCombatManager() const;
private:
	void RegisterEventListeners();
	void UnregisterEventListeners();
	void DispatchTurnChanged();
	void SyncTurnFromActorId(int actorId);
	void OnPhaseEnter(Phase phase);
	void OnPhaseExit(Phase phase);
	void OnExplorationTurnStateEnter(ExplorationTurnState state);
	void OnExplorationTurnStateExit(ExplorationTurnState state);
	void OnCombatTurnStateEnter(CombatTurnState state);
	void OnCombatTurnStateExit(CombatTurnState state);

	class GameObject* FindPlayerObject(class Scene* scene) const;
	class GameDataRepository* GetGameDataRepository() const;
	class LogSystem* GetLogSystem() const;
	class RandomMachine* GetRandomMachine() const;
	class DiceSystem* GetDiceSystem() const;
	class ShopRoller* GetShopRoller() const;
	void LoadGameData();
	void InitializePlayer();
	void InitializeFloor();
	void AdvanceFloor();
	void SetPlayerShopState(bool inShop);
	void SetFloodSystemActive(bool active);
	void RefreshGridSystem();
	void DispatchPlayerFSMEvent(const std::string& eventName);
	void ResolveEnemyAttack(int actorId = 0);
	bool ResolveEnemyGroupTurn();
	std::vector<int> CollectOwnedItemIndices() const;
private:

	EventDispatcher* m_EventDispatcher = nullptr;
	ServiceRegistry* m_Services = nullptr;
	Scene* m_ActiveScene = nullptr;
	Turn   m_Turn;
	Battle m_BattleCheck;
	Phase  m_Phase; 
	ExplorationTurnState m_ExplorationTurnState;
	CombatTurnState m_CombatTurnState;
	float m_ExplorationTurnElapsed = 0.0f;
	float m_ExplorationTurnLimit = 10.0f; // 탐색 시간
	float m_CombatTurnElapsed = 0.0f;
	float m_CombatTurnLimit = 10.0f;      //전투 시간
	float m_PlayerActionInputLockElapsed = 0.0f;		//플레이어 입력 지연
	float m_PlayerActionInputLockDuration = 0.5f;
	float m_EnemyTurnDelayElapsed = 0.0f;
	float m_EnemyTurnDelayDuration = 0.5f;
	bool  m_InitCompletePending = false;
	bool  m_FloorReadyPending = false;
	bool  m_WaitingForFloorScene = false;
	bool  m_GameDataLoaded = false;
	bool  m_BlockPostCombatShop = false;
	int   m_CurrentFloor = 1;
	bool  m_SkipToPlayerTurn = false;
	bool  m_ResolveEnemyTurn = false;
	bool  m_PlayerActionInputLocked = false;
	bool  m_WaitingEnemyTurnDelay = false;
	bool  m_PendingPlayerDiceDecisionRequest = false;
	bool  m_PendingPlayerDiceStatRollRequest = false;
	bool  m_PendingPlayerDiceContinueRequest = false;
	int   m_PendingDiceAnimations = 0;
	int   m_RemainingEnemyTurns = 0;
	int   m_ExplorationActiveEnemyActorId = 0;

	std::vector<std::string> m_FloorSceneNames;
	DataSheetPaths m_DataPaths{};
	ShopStock m_CurrentShopStock{};

	PlayerPersistentData m_PlayerData;
};