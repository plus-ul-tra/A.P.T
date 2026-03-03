#include "pch.h"
#include "CombatManager.h"
#include "AIController.h"
#include "CombatResolver.h"
#include "DiceSystem.h"
#include "Event.h"
#include "LogSystem.h"
#include "EventDispatcher.h"
#include "CombatEvents.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
	DiceConfig BuildStatConfigFromDecisionD20(const int decisionD20)
	{
		if (decisionD20 <= 5)
		{
			return { 2, 12, 0 };
		}
		if (decisionD20 <= 10)
		{
			return { 3, 8, 0 };
		}
		if (decisionD20 <= 15)
		{
			return { 4, 6, 0 };
		}
		return { 6, 4, 0 };
	}
}

CombatManager::CombatManager(CombatResolver& combatResolver, DiceSystem& diceSystem, LogSystem* logger)
	: m_Resolver(combatResolver), m_DiceSystem(diceSystem), m_LogSystem(logger)
{
}

void CombatManager::HandlePlayerAttack(const AttackRequest& request)
{
	if (!CanAct(request.actorId))
		return;

	if (request.targetIds.empty())
		return;

	std::cout << "[Combat] Player attack requested by actor " << request.actorId
		<< " targeting " << request.targetIds.front() << std::endl;

	if (m_State == Battle::NonBattle)
	{
		EnterBattle(request.actorId, request.targetIds.front());
	}

	//AdvanceTurn();
}

void CombatManager::TickAI(AIController& controller, float deltaTime)
{
	if (m_State != Battle::InBattle)
	{
		return;
	}

	controller.Tick(deltaTime);
}

void CombatManager::EnterBattle(int initiatorId, int targetId)
{
	(void)initiatorId;
	(void)targetId;

	m_State = Battle::InBattle;
	m_ActorIdsInBattle.clear();
	m_InitiativeOrder.clear();
	m_CurrentTurnIndex = 0;
	m_DiceFlowActive = false;
	m_PlayerDecisionReady = false;
	m_PlayerDecisionD20 = 0;
	m_PlayerInitiativeDiceBonus = 0;
	m_PlayerInitiativeTotal = 0;
	m_PlayerDecisionFaces.clear();
	m_PlayerStatRollFacesHistory.clear();
	m_PlayerStatRollTotalsHistory.clear();
	m_PendingInitiativeEntries.clear();

	std::cout << "[Combat] Enter battle: initiator=" << initiatorId
		<< " target=" << targetId << std::endl;

	if (m_EventDispatcher)
	{
		const CombatEnterEvent eventData{ initiatorId, targetId };
		m_EventDispatcher->Dispatch(EventType::CombatEnter, &eventData);
	}

	BuildInitiativeOrder();
}

void CombatManager::ExitBattle()
{
	m_State = Battle::NonBattle;
	m_InitiativeOrder.clear();
	m_ActorIdsInBattle.clear();
	m_CurrentTurnIndex = 0;
	m_DiceFlowActive = false;
	m_PlayerDecisionReady = false;
	m_PlayerDecisionD20 = 0;
	m_PlayerInitiativeDiceBonus = 0;
	m_PlayerInitiativeTotal = 0;
	m_PlayerDecisionFaces.clear();
	m_PlayerStatRollFacesHistory.clear();
	m_PlayerStatRollTotalsHistory.clear();
	m_PendingInitiativeEntries.clear();

	std::cout << "[Combat] Exit battle" << std::endl;
	if (m_EventDispatcher)
	{
		const CombatExitEvent eventData;
		m_EventDispatcher->Dispatch(EventType::CombatExit, &eventData);
		m_EventDispatcher->Dispatch(EventType::CombatEnded, nullptr);
	}
}

void CombatManager::SetCombatants(const std::vector<CombatantSnapshot>& combatants)
{
	m_Combatants = combatants;
}

bool CombatManager::AddCombatants(const std::vector<CombatantSnapshot>& combatants)
{
	if (combatants.empty())
	{
		return false;
	}

	bool added = false;
	for (const auto& combatant : combatants)
	{
		if (combatant.actorId == 0)
		{
			continue;
		}

		const bool alreadyRegistered = std::any_of(
			m_Combatants.begin(),
			m_Combatants.end(),
			[&combatant](const CombatantSnapshot& existing)
			{
				return existing.actorId == combatant.actorId;
			});

		if (alreadyRegistered)
		{
			continue;
		}

		m_Combatants.push_back(combatant);
		added = true;
	}

	if (!added)
	{
		return false;
	}

	const int currentActor = GetCurrentActorId();
	BuildInitiativeOrder();

	if (!m_DiceFlowActive && m_State == Battle::InBattle && currentActor != 0 && !m_InitiativeOrder.empty())
	{
		const auto it = std::find(m_InitiativeOrder.begin(), m_InitiativeOrder.end(), currentActor);
		if (it != m_InitiativeOrder.end())
		{
			m_CurrentTurnIndex = static_cast<std::size_t>(std::distance(m_InitiativeOrder.begin(), it));
		}
	}

	return true;
}


void CombatManager::UpdateBattleOutcome(bool playerAlive, bool enemiesRemaining)
{
	if (m_State != Battle::InBattle)
		return;

	if (!playerAlive || !enemiesRemaining)
		ExitBattle();
}

void CombatManager::ResetSessionState()
{
	m_State = Battle::NonBattle;
	m_Combatants.clear();
	m_InitiativeOrder.clear();
	m_ActorIdsInBattle.clear();
	m_CurrentTurnIndex = 0;
	m_DiceFlowActive = false;
	m_PlayerDecisionReady = false;
	m_PlayerDecisionD20 = 0;
	m_PlayerInitiativeDiceBonus = 0;
	m_PlayerInitiativeTotal = 0;
	m_PlayerDecisionFaces.clear();
	m_PlayerStatRollFacesHistory.clear();
	m_PlayerStatRollTotalsHistory.clear();
	m_PendingInitiativeEntries.clear();
}

int CombatManager::GetCurrentActorId() const
{
	if (m_InitiativeOrder.empty())
	{
		return 0;
	}
	return m_InitiativeOrder[m_CurrentTurnIndex];
}


void CombatManager::BuildInitiativeOrder()
{
	m_InitiativeOrder.clear();
	m_ActorIdsInBattle.clear();
	m_PendingInitiativeEntries.clear();

	if (m_Combatants.empty())
		return;

	bool requiresPlayerDiceFlow = false;

	for (const CombatantSnapshot& combatant : m_Combatants)
	{

		if (combatant.actorId == 0)
		{
			continue;
		}

		if (combatant.isPlayer)
		{
			m_PlayerActorId = combatant.actorId;
			m_PlayerInitiativeDiceBonus = combatant.initiativeBonus;
			requiresPlayerDiceFlow = true;
			continue;
		}

		if (!requiresPlayerDiceFlow)
		{
			const DiceConfig rollConfig{ 1, 20, 0 };
			const int roll = m_DiceSystem.RollTotal(rollConfig, RandomDomain::Combat);
			const int initiative = roll + combatant.initiativeBonus;

			std::cout << "[Combat] Initiative roll actor=" << combatant.actorId
				<< " d20=" << roll
				<< " bonus=" << combatant.initiativeBonus
				<< " total=" << initiative << std::endl;
			m_PendingInitiativeEntries.push_back({ combatant.actorId, initiative });
		}
	}

	if (requiresPlayerDiceFlow)
	{
		m_DiceFlowActive = true;
		m_PlayerDecisionReady = false;
		m_PlayerDecisionD20 = 0;
		m_PlayerInitiativeTotal = 0;

		if (m_EventDispatcher)
		{
			m_EventDispatcher->Dispatch(EventType::PlayerDiceUIOpen, nullptr);
			m_EventDispatcher->Dispatch(EventType::PlayerDiceUIReset, nullptr);
		}
		return;
	}

	FinalizeBattleStart();
}

void CombatManager::FinalizeBattleStart()
{
	if (m_State != Battle::InBattle)
	{
		return;
	}

	// Dice UI 플로우 중에는 엔티티 선제권 롤을 여기서 확정한다.
	  // (버튼 입력 전 선제권이 이미 결정되는 인상을 방지)
	if (m_DiceFlowActive)
	{
		m_PendingInitiativeEntries.clear();
		for (const CombatantSnapshot& combatant : m_Combatants)
		{
			if (combatant.actorId == 0 || combatant.isPlayer)
			{
				continue;
			}

			const DiceConfig rollConfig{ 1, 20, 0 };
			const int roll = m_DiceSystem.RollTotal(rollConfig, RandomDomain::Combat);
			const int initiative = roll + combatant.initiativeBonus;
			std::cout << "[Combat] Initiative roll actor=" << combatant.actorId
				<< " d20=" << roll
				<< " bonus=" << combatant.initiativeBonus
				<< " total=" << initiative << std::endl;
			m_PendingInitiativeEntries.push_back({ combatant.actorId, initiative });
		}
	}

	if (m_PlayerDecisionReady && m_PlayerActorId != 0)
	{
		m_PendingInitiativeEntries.push_back({ m_PlayerActorId, m_PlayerInitiativeTotal });
	}

	std::sort(m_PendingInitiativeEntries.begin(), m_PendingInitiativeEntries.end(),
		[](const InitiativeEntry& left, const InitiativeEntry& right)
		{
			return left.initiative > right.initiative;
		});

	for (const InitiativeEntry& entry : m_PendingInitiativeEntries)
	{
		m_InitiativeOrder.push_back(entry.actorId);
		m_ActorIdsInBattle.insert(entry.actorId);
	}

	if (!m_InitiativeOrder.empty())
	{
		std::ostringstream order;
		order << "[Combat] Initiative order:";
		for (int actorId : m_InitiativeOrder)
		{
			order << " " << actorId;
		}
		std::cout << order.str() << std::endl;
	}

	if (m_EventDispatcher)
	{
		const CombatInitiativeBuiltEvent eventData{ &m_InitiativeOrder };
		m_EventDispatcher->Dispatch(EventType::CombatInitiativeBuilt, &eventData);
		m_EventDispatcher->Dispatch(EventType::CombatInitComplete, nullptr);
	}

	m_CurrentTurnIndex = 0;
	if (m_EventDispatcher && !m_InitiativeOrder.empty())
	{
		const CombatTurnAdvancedEvent eventData{ m_InitiativeOrder[m_CurrentTurnIndex] };
		m_EventDispatcher->Dispatch(EventType::CombatTurnAdvanced, &eventData);
	}

	if (!m_InitiativeOrder.empty())
	{
		std::cout << "[Combat] Turn start: actor=" << m_InitiativeOrder[m_CurrentTurnIndex] << std::endl;
	}


	m_DiceFlowActive = false;
	m_PlayerDecisionReady = false;
	m_PlayerDecisionD20 = 0;
	m_PlayerDecisionFaces.clear();
	m_PlayerInitiativeTotal = 0;
	m_PendingInitiativeEntries.clear();
}

void CombatManager::HandlePlayerDiceDecisionRequested()
{
	if (!m_DiceFlowActive || m_State != Battle::InBattle)
	{
		std::cout << "[Combat] Ignore PlayerDiceDecisionRequested. diceFlow=" << m_DiceFlowActive
			<< " state=" << static_cast<int>(m_State) << std::endl;
		return;
	}

	const DiceConfig d20x3{ 3, 20, 0 };
	const DiceRoll roll = m_DiceSystem.Roll(d20x3, RandomDomain::Combat);
	if (roll.faces.empty())
	{
		std::cout << "[Combat] Decision roll faces empty." << std::endl;
		return;
	}

	m_PlayerDecisionReady = false;
	m_PlayerInitiativeTotal = 0;
	m_PlayerDecisionFaces = roll.faces;
	m_PlayerStatRollFacesHistory.clear();
	m_PlayerStatRollTotalsHistory.clear();
	m_PlayerDecisionD20 = *std::max_element(roll.faces.begin(), roll.faces.end());
	std::cout << "[Combat] Decision roll faces=";
	for (size_t i = 0; i < roll.faces.size(); ++i)
	{
		std::cout << (i == 0 ? "" : ",") << roll.faces[i];
	}
	std::cout << " selectedD20=" << m_PlayerDecisionD20 << std::endl;

	if (m_EventDispatcher)
	{
		for (size_t i = 0; i < roll.faces.size(); ++i)
		{
			const std::string context = "InitiativeDecisionRoll_" + std::to_string(i + 1);
			const Events::DiceDecisionFaceEvent faceEvent{ static_cast<int>(i), roll.faces[i], m_PlayerDecisionD20, context };
			std::cout << "[Combat] Dispatch PlayerDiceDecisionFaceRolled context=" << context
				<< " value=" << roll.faces[i] << std::endl;
			m_EventDispatcher->Dispatch(EventType::PlayerDiceDecisionFaceRolled, &faceEvent);

			const Events::DiceRollEvent oneDieEvent{ roll.faces[i], 1, d20x3.sides, 0, context, false, { roll.faces[i] } };
			m_EventDispatcher->Dispatch(EventType::DiceRolled, &oneDieEvent);
		}

		const Events::DiceRollEvent rollEvent{ m_PlayerDecisionD20, d20x3.count, d20x3.sides, 0, "InitiativeDecisionRoll", false, roll.faces };
		std::cout << "[Combat] Dispatch DiceRolled context=InitiativeDecisionRoll value=" << m_PlayerDecisionD20 << std::endl;
		m_EventDispatcher->Dispatch(EventType::DiceRolled, &rollEvent);

		const DiceConfig mappedStatConfig = BuildStatConfigFromDecisionD20(m_PlayerDecisionD20);
		const Events::DiceInitiativeResolvedEvent resultEvent{ roll.faces, m_PlayerDecisionD20, m_PlayerInitiativeDiceBonus, 0 };
		const Events::DiceRollEvent diceTypeEvent{ mappedStatConfig.sides, 1, mappedStatConfig.sides, 0, "InitiativeDiceType", true, { mappedStatConfig.sides } };
		m_EventDispatcher->Dispatch(EventType::PlayerDiceTypeDetermined, &diceTypeEvent);
		std::cout << "[Combat] Dispatch PlayerDiceDecisionResult selectedD20=" << m_PlayerDecisionD20 << std::endl;
		m_EventDispatcher->Dispatch(EventType::PlayerDiceInitiativeResolved, &resultEvent);
		m_EventDispatcher->Dispatch(EventType::PlayerDiceDecisionResult, &resultEvent);
	}
}

void CombatManager::HandlePlayerDiceStatRollRequested()
{
	if (!m_DiceFlowActive || m_State != Battle::InBattle || m_PlayerDecisionD20 <= 0)
	{
		std::cout << "[Combat] Ignore PlayerDiceStatRollRequested. diceFlow=" << m_DiceFlowActive
			<< " state=" << static_cast<int>(m_State)
			<< " selectedD20=" << m_PlayerDecisionD20 << std::endl;
		return;
	}

	const std::vector<int> decisionFaces = m_PlayerDecisionFaces.empty()
		? std::vector<int>{ m_PlayerDecisionD20 }
	: m_PlayerDecisionFaces;

	m_PlayerStatRollFacesHistory.clear();
	m_PlayerStatRollTotalsHistory.clear();

	int selectedHistoryIndex = -1;
	for (size_t historyIdx = 0; historyIdx < decisionFaces.size(); ++historyIdx)
	{
		const int decisionFace = decisionFaces[historyIdx];
		const DiceConfig statConfig = BuildStatConfigFromDecisionD20(decisionFace);
		const DiceRoll statRoll = m_DiceSystem.Roll(statConfig, RandomDomain::Combat);

		m_PlayerStatRollFacesHistory.push_back(statRoll.faces);
		m_PlayerStatRollTotalsHistory.push_back(statRoll.total);

		if (selectedHistoryIndex < 0 && decisionFace == m_PlayerDecisionD20)
		{
			selectedHistoryIndex = static_cast<int>(historyIdx);
		}
	}

	if (selectedHistoryIndex < 0)
	{
		selectedHistoryIndex = 0;
	}

	const DiceConfig selectedConfig = BuildStatConfigFromDecisionD20(decisionFaces[static_cast<size_t>(selectedHistoryIndex)]);
	const auto& selectedFaces = m_PlayerStatRollFacesHistory[static_cast<size_t>(selectedHistoryIndex)];
	const int selectedTotal = m_PlayerStatRollTotalsHistory[static_cast<size_t>(selectedHistoryIndex)];
	m_PlayerInitiativeTotal = selectedTotal + m_PlayerInitiativeDiceBonus;
	m_PlayerDecisionReady = true;

	std::cout << "[Combat] Stat roll groups=" << decisionFaces.size()
		<< " selectedIndex=" << selectedHistoryIndex
		<< " selectedCount=" << selectedConfig.count
		<< " selectedSides=" << selectedConfig.sides
		<< " selectedTotal=" << selectedTotal
		<< " bonus=" << m_PlayerInitiativeDiceBonus
		<< " initiativeTotal=" << m_PlayerInitiativeTotal << std::endl;

	if (m_EventDispatcher)
	{
		for (size_t historyIdx = 0; historyIdx < m_PlayerStatRollFacesHistory.size(); ++historyIdx)
		{
			const int decisionFace = decisionFaces[historyIdx];
			const DiceConfig statConfig = BuildStatConfigFromDecisionD20(decisionFace);
			const auto& faces = m_PlayerStatRollFacesHistory[historyIdx];
			const int total = m_PlayerStatRollTotalsHistory[historyIdx];
			const std::string rollContext = "InitiativeStatRoll_" + std::to_string(historyIdx + 1);

			const Events::DiceRollEvent diceTypeEvent{ statConfig.sides, 1, statConfig.sides, 0, rollContext, true, { statConfig.sides } };

			m_EventDispatcher->Dispatch(EventType::PlayerDiceTypeDetermined, &diceTypeEvent);

			for (size_t faceIdx = 0; faceIdx < faces.size(); ++faceIdx)
			{
				const std::string context = rollContext + "_" + std::to_string(faceIdx + 1);
				const Events::DiceRollEvent oneDieEvent{ faces[faceIdx], 1, statConfig.sides, 0, context, false, { faces[faceIdx] } };
				std::cout << "[Combat] Dispatch DiceRolled context=" << context << " value=" << faces[faceIdx] << std::endl;
				m_EventDispatcher->Dispatch(EventType::DiceRolled, &oneDieEvent);
			}

			const Events::DiceRollEvent statRollEvent{ total, statConfig.count, statConfig.sides, 0, rollContext, true, faces };
			std::cout << "[Combat] Dispatch DiceRolled context=" << rollContext << " value=" << total << std::endl;
			m_EventDispatcher->Dispatch(EventType::DiceRolled, &statRollEvent);
		}
		const Events::DiceStatResolvedEvent resultEvent{
			m_PlayerDecisionD20,
				selectedConfig.count,
			selectedConfig.sides,
			selectedFaces,
			m_PlayerStatRollFacesHistory,
			m_PlayerStatRollTotalsHistory,
			selectedTotal,
			m_PlayerInitiativeDiceBonus,
			m_PlayerInitiativeTotal };
		m_EventDispatcher->Dispatch(EventType::PlayerDiceStatResolved, &resultEvent);

		std::cout << "[Combat] Dispatch PlayerDiceStatResolved"
			<< " currentFaces=" << selectedFaces.size()
			<< " historyCount=" << m_PlayerStatRollFacesHistory.size()
			<< " totalsCount=" << m_PlayerStatRollTotalsHistory.size()
			<< std::endl;
	}
}

void CombatManager::HandlePlayerDiceContinueRequested()
{
	if (!m_DiceFlowActive || !m_PlayerDecisionReady)
	{
		std::cout << "[Combat] Ignore PlayerDiceContinueRequested. diceFlow=" << m_DiceFlowActive
			<< " decisionReady=" << m_PlayerDecisionReady << std::endl;
		return;
	}

	if (m_EventDispatcher)
	{
		std::cout << "[Combat] Dispatch PlayerDiceUIReset + PlayerDiceUIClose" << std::endl;
		m_EventDispatcher->Dispatch(EventType::PlayerDiceUIReset, nullptr);
		m_EventDispatcher->Dispatch(EventType::PlayerDiceUIClose, nullptr);
	}

	FinalizeBattleStart();
}

bool CombatManager::IsPlayerActorId(int actorId) const
{
	for (const auto& combatant : m_Combatants)
	{
		if (combatant.actorId == actorId)
		{
			return combatant.isPlayer;
		}
	}
	return actorId == 1;
}

bool CombatManager::IsActorInBattle(int actorId) const
{
	return actorId != 0 && m_ActorIdsInBattle.find(actorId) != m_ActorIdsInBattle.end();
}

void CombatManager::AdvanceTurnToNextPlayer()
{
	if (m_InitiativeOrder.empty())
	{
		return;
	}

	const std::size_t maxSteps = m_InitiativeOrder.size();
	for (std::size_t step = 0; step < maxSteps; ++step)
	{
		m_CurrentTurnIndex = (m_CurrentTurnIndex + 1) % m_InitiativeOrder.size();
		if (IsPlayerActorId(m_InitiativeOrder[m_CurrentTurnIndex]))
		{
			break;
		}
	}

	if (m_EventDispatcher)
	{
		const CombatTurnAdvancedEvent eventData{ m_InitiativeOrder[m_CurrentTurnIndex] };
		m_EventDispatcher->Dispatch(EventType::CombatTurnAdvanced, &eventData);
	}

	std::cout << "[Combat] Turn advanced: actor=" << m_InitiativeOrder[m_CurrentTurnIndex] << std::endl;
}

bool CombatManager::AdvanceTurnToNextEnemyOrPlayer()
{
	if (m_InitiativeOrder.empty())
	{
		return false;
	}

	const std::size_t maxSteps = m_InitiativeOrder.size();
	for (std::size_t step = 0; step < maxSteps; ++step)
	{
		m_CurrentTurnIndex = (m_CurrentTurnIndex + 1) % m_InitiativeOrder.size();
		if (!IsPlayerActorId(m_InitiativeOrder[m_CurrentTurnIndex]))
		{
			if (m_EventDispatcher)
			{
				const CombatTurnAdvancedEvent eventData{ m_InitiativeOrder[m_CurrentTurnIndex] };
				m_EventDispatcher->Dispatch(EventType::CombatTurnAdvanced, &eventData);
			}

			std::cout << "[Combat] Turn advanced: actor=" << m_InitiativeOrder[m_CurrentTurnIndex] << std::endl;
			return true;
		}
	}

	AdvanceTurnToNextPlayer();
	return false;
}

bool CombatManager::CanAct(int actorId) const
{
	if (m_InitiativeOrder.empty())
		return false;

	return m_InitiativeOrder[m_CurrentTurnIndex] == actorId;
}

void CombatManager::AdvanceTurn()
{
	if (m_InitiativeOrder.empty())
		return;

	m_CurrentTurnIndex = (m_CurrentTurnIndex + 1) % m_InitiativeOrder.size();

	if (m_EventDispatcher)
	{
		const CombatTurnAdvancedEvent eventData{ m_InitiativeOrder[m_CurrentTurnIndex] };
		m_EventDispatcher->Dispatch(EventType::CombatTurnAdvanced, &eventData);
	}

	std::cout << "[Combat] Turn advanced: actor=" << m_InitiativeOrder[m_CurrentTurnIndex] << std::endl;
}
