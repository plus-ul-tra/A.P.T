#include "EnemyControllerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GridSystemComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "CombatManager.h"
#include "GameManager.h"

REGISTER_COMPONENT(EnemyControllerComponent)

EnemyControllerComponent::~EnemyControllerComponent()
{
	// 이벤트 리스너 제거 (Start에서 등록하면 반드시 제거해야 함)
	auto& disp = GetEventDispatcher();
	disp.RemoveListener(EventType::AIMoveRequested, this);
	disp.RemoveListener(EventType::AIRunOffMoveRequested, this);
	disp.RemoveListener(EventType::AIMaintainRangeRequested, this);
}

void EnemyControllerComponent::Start()
{
	GetSystem();

	// BT에서 올라오는 요청 이벤트를 여기서 받음
	auto& disp = GetEventDispatcher();
	disp.AddListener(EventType::AIMoveRequested, this);
	disp.AddListener(EventType::AIRunOffMoveRequested, this);
	disp.AddListener(EventType::AIMaintainRangeRequested, this);
}

void EnemyControllerComponent::Update(float deltaTime)
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;

	if (!gameManager || !m_GridSystem)
		return;

	const Phase phase = gameManager->GetPhase();

	if (phase == Phase::GameOver)
	{
		return;
	}

	// 1) 탐색 루프의 EnemyStep (다수 적 이동 끝나면 ExploreEnemyStepEnded 쏨)
	if (phase == Phase::ExplorationLoop)
	{
		if (gameManager->GetExplorationTurnState() != ExplorationTurnState::EnemyStep)
		{
			m_TurnEndRequested = false;
			m_ExploreEnemyIndex = 0;
			m_WaitingExploreDelay = false;
			m_ExploreDelayElapsed = 0.0f;
			m_ExploreDelayDuration = 0.0f;
			gameManager->SetExplorationActiveEnemyActorId(0);
			return;
		}


		// EnemyTurn인지 확인
		Turn currentTurn = Turn::PlayerTurn;
		bool hasEnemies = false;

		const auto& enemies = m_GridSystem->GetEnemies();
		for (const auto* enemy : enemies)
		{
			if (!enemy) continue;
			hasEnemies = true;
			currentTurn = enemy->GetCurrentTurn();
			break;
		}

		if (currentTurn != Turn::EnemyTurn)
		{
			m_TurnEndRequested = false;
			m_ExploreEnemyIndex = 0;
			m_WaitingExploreDelay = false;
			m_ExploreDelayElapsed = 0.0f;
			m_ExploreDelayDuration = 0.0f;
			gameManager->SetExplorationActiveEnemyActorId(0);
			return;
		}

		if (!hasEnemies)
		{
			if (!m_TurnEndRequested)
			{
				GetEventDispatcher().Dispatch(EventType::ExploreEnemyStepEnded, nullptr);
				m_TurnEndRequested = true;
			}
			gameManager->SetExplorationActiveEnemyActorId(0);
			return;
		}

		const int enemyCount = static_cast<int>(enemies.size());
		if (m_WaitingExploreDelay)
		{
			m_ExploreDelayElapsed += deltaTime;
			if (m_ExploreDelayElapsed < m_ExploreDelayDuration)
			{
				return;
			}

			m_WaitingExploreDelay = false;
			m_ExploreDelayElapsed = 0.0f;
			m_ExploreDelayDuration = 0.0f;
			++m_ExploreEnemyIndex;
		}

		while (m_ExploreEnemyIndex < enemyCount)
		{
			auto* enemy = enemies[m_ExploreEnemyIndex];
			if (!enemy)
			{
				++m_ExploreEnemyIndex;
				continue;
			}

			auto* enemyOwner = enemy->GetOwner();
			auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
			if (enemyStat && enemyStat->IsDead())
			{
				++m_ExploreEnemyIndex;
				continue;
			}
			break;
		}

		if (m_ExploreEnemyIndex >= enemyCount)
		{
			if (!m_TurnEndRequested)
			{
				GetEventDispatcher().Dispatch(EventType::ExploreEnemyStepEnded, nullptr);
				m_TurnEndRequested = true;
			}
			gameManager->SetExplorationActiveEnemyActorId(0);
			return;
		}

		auto* currentEnemy = enemies[m_ExploreEnemyIndex];
		if (currentEnemy)
		{
			gameManager->SetExplorationActiveEnemyActorId(currentEnemy->GetActorId());
		}

		auto* currentOwner = currentEnemy ? currentEnemy->GetOwner() : nullptr;
		auto* movement = currentOwner ? currentOwner->GetComponent<EnemyMovementComponent>() : nullptr;
		const bool isFinished = currentEnemy && currentEnemy->IsExploreTurnFinished()
			&& movement && movement->IsMoveComplete();

		if (!isFinished)
		{
			return;
		}

		gameManager->SetExplorationActiveEnemyActorId(0);
		m_WaitingExploreDelay = true;
		m_ExploreDelayElapsed = 0.0f;
		m_ExploreDelayDuration = currentEnemy ? currentEnemy->GetEndTurnDelay() : 0.0f;
		if (m_ExploreDelayDuration <= 0.0f)
		{
			m_WaitingExploreDelay = false;
			m_ExploreDelayDuration = 0.0f;
			++m_ExploreEnemyIndex;
		}

		return;
	}

	// 2) 전투(턴 기반): 여기서는 “AI 행동 요청 후 이동 완료되면 AITurnEndRequested”만 책임지게 하는 게 안전함
	/*if (phase == Phase::TurnBasedCombat && gameManager->GetCombatTurnState() == CombatTurnState::EnemyTurn)*/
	if (phase == Phase::TurnBasedCombat)
	{
		// AI가 이동/도주/거리유지 같은 “이동계열 요청”을 내렸고,
        // 그 이동이 끝났으면 적 턴 종료를 GameManager에 알려줌.
		//if (m_CombatMoveInProgress)
		if (gameManager->GetCombatTurnState() == CombatTurnState::EnemyTurn)
		{
			auto* currentEnemy = GetCurrentEnemy();
			bool currentEnemyDead = false;
			if (currentEnemy)
			{
				auto* enemyOwner = currentEnemy->GetOwner();
				auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
				currentEnemyDead = enemyStat && enemyStat->IsDead();
			}
			else
			{
				currentEnemyDead = true;
			}

			if (currentEnemyDead)
			{
				if (!m_CombatTurnEndRequested)
				{
					GetEventDispatcher().Dispatch(EventType::AITurnEndRequested, nullptr);
					m_CombatTurnEndRequested = true;
				}
				m_CombatMoveInProgress = false;
				return;
			}

			m_CombatTurnEndRequested = false;
			
			if (m_CombatMoveInProgress && IsCurrentEnemyMoveComplete())
			{
				GetEventDispatcher().Dispatch(EventType::AITurnEndRequested, nullptr);
				m_CombatMoveInProgress = false;
				m_CombatTurnEndRequested = true;
			}
		}
		else {
			m_CombatMoveInProgress = false;
			m_CombatTurnEndRequested = false;
		}
	}
}

void EnemyControllerComponent::OnEvent(EventType type, const void* data)
{
	(void)data;

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (!gameManager || !m_GridSystem)
		return;

	// 전투 중 EnemyTurn일 때만 AI 이동계열 요청 처리
	if (gameManager->GetPhase() != Phase::TurnBasedCombat
		|| gameManager->GetCombatTurnState() != CombatTurnState::EnemyTurn)
		return;

	EnemyMovementComponent* move = GetCurrentEnemyMovement();
	if (!move)
		return;

	switch (type)
	{
	case EventType::AIMoveRequested:
		move->RequestMoveToTarget();
		m_CombatMoveInProgress = true;
		break;

	case EventType::AIRunOffMoveRequested:
		move->RequestRunOff();   
		m_CombatMoveInProgress = true;
		break;

	case EventType::AIMaintainRangeRequested:
		move->RequestMaintainRange();
		m_CombatMoveInProgress = true;
		break;

	default:
		break;
	}
}

void EnemyControllerComponent::GetSystem()
{
	auto* object = GetOwner();
	if (!object) { return; }
	auto* grid = object->GetComponent<GridSystemComponent>();
	if (!grid) { return;  }

	m_GridSystem = grid;
}

bool EnemyControllerComponent::CheckActiveEnemies()
{
	const auto& enemies = m_GridSystem->GetEnemies();

	for (const auto* enemy : enemies) {
		if (!enemy) continue;

		auto* owner = enemy->GetOwner();
		if (!owner) continue;

		auto* movement = owner->GetComponent<EnemyMovementComponent>();
		if (!movement) continue;

		if (!enemy->IsExploreTurnFinished())
			return false;

		if (!movement->IsMoveComplete())
			return false;
	}
	return true;
}

EnemyComponent* EnemyControllerComponent::GetCurrentEnemy() const
{
	if (!m_GridSystem)
	{
		return nullptr;
	}

	auto* scene = GetOwner() ? GetOwner()->GetScene() : nullptr;
	auto* combatManager = (scene && scene->GetServices().Has<CombatManager>())
		? &scene->GetServices().Get<CombatManager>()
		: nullptr;

	const int currentActorId = combatManager ? combatManager->GetCurrentActorId() : 0;
	if (currentActorId == 0)
	{
		return nullptr;
	}

	const auto& enemies = m_GridSystem->GetEnemies();
	for (auto* enemy : enemies)
	{
		if (!enemy)
		{
			continue;
		}
		if (enemy->GetActorId() == currentActorId)
		{
			return enemy;
		}
	}
	return nullptr;
}

// 전투에서 "현재 행동 중인 적"을 찾는 최소 구현
EnemyMovementComponent* EnemyControllerComponent::GetCurrentEnemyMovement()
{
	const auto& enemies = m_GridSystem->GetEnemies();

	auto* scene = GetOwner() ? GetOwner()->GetScene() : nullptr;
	auto* combatManager = (scene && scene->GetServices().Has<CombatManager>())
		? &scene->GetServices().Get<CombatManager>()
		: nullptr;

	const int currentActorId = combatManager ? combatManager->GetCurrentActorId() : 0;

	for (const auto* enemy : enemies)
	{
		if (!enemy) continue;

		// 전투 턴에서 현재 적(actor) 판정 기준이 따로 있으면 그걸로 바꿔야 함.
		// 지금은 최소로: EnemyTurn인 개체를 하나 집음. (수정 완)
		if (currentActorId != 0)
		{
			if (enemy->GetActorId() != currentActorId)
			{
				continue;
			}
		}
		else if (enemy->GetCurrentTurn() != Turn::EnemyTurn)
			continue;

		auto* owner = enemy->GetOwner();
		if (!owner) continue;

		return owner->GetComponent<EnemyMovementComponent>();
	}
	return nullptr;
}

bool EnemyControllerComponent::IsCurrentEnemyMoveComplete()
{
	EnemyMovementComponent* move = GetCurrentEnemyMovement();
	if (!move) return true; // 없으면 더 할 게 없다고 보고 턴 종료
	return move->IsMoveComplete();
}