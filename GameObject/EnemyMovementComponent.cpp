#include "EnemyMovementComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "Scene.h"
#include "TransformComponent.h"
#include "NodeComponent.h"
#include "EnemyStatComponent.h"
#include "EnemyComponent.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include "SoundManager.h"
#include "CombatManager.h"
#include <algorithm>
#include <cmath>
#include <limits>

#undef max
#undef min
REGISTER_COMPONENT(EnemyMovementComponent)
REGISTER_PROPERTY(EnemyMovementComponent, PatrolPoints)

static bool TryGetRotationFromStep(const AxialKey& previous, const AxialKey& current, ERotationOffset& outDir)
{
	const AxialKey delta{ current.q - previous.q, current.r - previous.r };
	constexpr std::array<std::pair<AxialKey, ERotationOffset>, 6> kDirections{ {
		{ { 1, 0 }, ERotationOffset::clock_3 },
		{ { 1, -1 }, ERotationOffset::clock_5 },
		{ { 0, -1 }, ERotationOffset::clock_7 },
		{ { -1, 0 }, ERotationOffset::clock_9 },
		{ { -1, 1 }, ERotationOffset::clock_11 },
		{ { 0, 1 }, ERotationOffset::clock_1 }
	} };

	for (const auto& [dir, rotation] : kDirections)
	{
		if (dir.q == delta.q && dir.r == delta.r)
		{
			outDir = rotation;
			return true;
		}
	}

	return false;
}

static bool TryGetRotationTowardTargetApprox(const AxialKey& start, const AxialKey& target, ERotationOffset& outDir)
{
	const int dq = target.q - start.q;
	const int dr = target.r - start.r;
	if (dq == 0 && dr == 0)
	{
		return false;
	}

	constexpr std::array<std::pair<AxialKey, ERotationOffset>, 6> kDirections{ {
		{ { 1, 0 }, ERotationOffset::clock_3 },
		{ { 1, -1 }, ERotationOffset::clock_5 },
		{ { 0, -1 }, ERotationOffset::clock_7 },
		{ { -1, 0 }, ERotationOffset::clock_9 },
		{ { -1, 1 }, ERotationOffset::clock_11 },
		{ { 0, 1 }, ERotationOffset::clock_1 }
	} };

	ERotationOffset best = ERotationOffset::clock_3;
	int bestScore = std::numeric_limits<int>::min();

	for (const auto& [dir, rotation] : kDirections)
	{
		const int score = dq * dir.q + dr * dir.r;
		if (score > bestScore)
		{
			bestScore = score;
			best = rotation;
		}
	}

	outDir = best;
	return true;
}

EnemyMovementComponent::~EnemyMovementComponent()
{
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
}

void EnemyMovementComponent::Start()
{
	GetSystem();
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
}

void EnemyMovementComponent::Update(float deltaTime)
{
	(void)deltaTime;

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	auto* enemy = owner ? owner->GetComponent<EnemyComponent>() : nullptr;
	if (!gameManager)
		return;
	if (!enemy)
		return;

	// 탐색 EnemyStep 또는 전투 EnemyTurn에서만 움직임 허용
	const bool explorationEnemyStep =
		(gameManager->GetPhase() == Phase::ExplorationLoop &&
			gameManager->GetExplorationTurnState() == ExplorationTurnState::EnemyStep);

	const bool combatPhase = gameManager->GetPhase() == Phase::TurnBasedCombat;
	bool isInBattleActor = false;
	if (combatPhase && scene && scene->GetServices().Has<CombatManager>())
	{
		isInBattleActor = scene->GetServices().Get<CombatManager>().IsActorInBattle(enemy->GetActorId());
	}

	const bool combatEnemyTurn = (combatPhase && isInBattleActor && gameManager->GetCombatTurnState() == CombatTurnState::EnemyTurn);

	const bool combatNonBattleActor = combatPhase && !isInBattleActor;

	if (!explorationEnemyStep && !combatEnemyTurn && !combatNonBattleActor)
		return;

	if (explorationEnemyStep && enemy->GetCurrentTurn() != Turn::EnemyTurn)
		return;
	
	if (explorationEnemyStep
		&& gameManager->GetExplorationActiveEnemyActorId() != enemy->GetActorId())
	{
		return;
	}

	if (explorationEnemyStep && m_IsMoveComplete)
	{
		return;
	}

	if (combatNonBattleActor)
	{
		if (enemy->GetCurrentTurn() != Turn::EnemyTurn)
			return;
		if (m_IsMoveComplete)
			return;
	}
	
	bool hasRequest = false;

	// 1) 기존 탐색 이동 요청도 계속 지원
	if (enemy->ConsumeMoveRequest())
	{
		//const bool canApproachPlayerBySight = !combatNonBattleActor;
		m_PendingOrder = enemy->IsTargetVisible()
			? EMoveOrder::Approach
			: EMoveOrder::Patrol;
		hasRequest = true;
	}

	// 2) 전투/BT 쪽에서 직접 요청한 이동도 지원
	if (m_PendingOrder != EMoveOrder::None)
	{
		hasRequest = true;
	}

	if (!hasRequest)
		return;

	// 요청 실행 (실패하더라도 턴은 진행되게 "완료 처리"는 한다)
	switch (m_PendingOrder)
	{
	case EMoveOrder::RunOff:
		MoveRunOff();
		break;
	case EMoveOrder::MaintainRange:
		MoveMaintainRange();
		break;
	case EMoveOrder::Approach:
		MoveApproach();
		break;
	case EMoveOrder::Patrol:
	default:
		MovePatrol();
		break;
	}

	m_PendingOrder = EMoveOrder::None;
	m_IsMoveComplete = true;
	enemy->RefreshSightDebugLines();
}

void EnemyMovementComponent::OnEvent(EventType type, const void* data)
{
	if (type != EventType::TurnChanged || !data)
	{
		return;
	}

	const auto* payload = static_cast<const Events::TurnChanged*>(data);
	if (!payload)
	{
		return;
	}

	const auto turn = static_cast<Turn>(payload->turn);

	auto* scene = GetOwner() ? GetOwner()->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;

	// EnemyTurn 시작이면 매번 이동 완료 플래그 리셋
	if (turn == Turn::EnemyTurn && gameManager)
	{
		// 탐색/전투 둘 다 EnemyTurn이면 리셋
		m_IsMoveComplete = false;
	}
}

void EnemyMovementComponent::MoveRunOff()
{
	auto* owner = GetOwner();
	auto* enemy = owner ? owner->GetComponent<EnemyComponent>() : nullptr;
	if (!enemy || !m_GridSystem)
	{
		return;
	}	

	const int moveRange = enemy->GetMoveDistance();
	if (moveRange <= 0)
	{
		return;
	}

	AxialKey playerKey{};
	bool hasPlayer = false;
	for (auto* node : m_GridSystem->GetNodes())
	{
		if (node && node->GetState() == NodeState::HasPlayer)
		{
			playerKey = { node->GetQ(), node->GetR() };
			hasPlayer = true;
			break;
		}
	}

	if (!hasPlayer)
	{
		return;
	}

	const AxialKey start = { enemy->GetQ(), enemy->GetR() };
	NodeComponent* bestNode = nullptr;
	int bestDistance = -1;

	for (auto* node : m_GridSystem->GetNodes())
	{
		if (!node || !node->GetIsMoveable() || node->GetState() != NodeState::Empty)
		{
			continue;
		}

		const AxialKey candidate{ node->GetQ(), node->GetR() };
		const int distanceFromStart = m_GridSystem->GetShortestPathLength(start, candidate);
		if (distanceFromStart <= 0 || distanceFromStart > moveRange)
		{
			continue;
		}

		const int distanceFromPlayer = m_GridSystem->GetShortestPathLength(candidate, playerKey);
		if (distanceFromPlayer < 0)
		{
			continue;
		}

		if (distanceFromPlayer > bestDistance)
		{
			bestDistance = distanceFromPlayer;
			bestNode = node;
		}
	}

	if (!bestNode)
	{
		return;
	}

	AxialKey previous{};
	AxialKey next{};
	bool reachedTarget = false;
	if (SelectMoveKeyTowardTarget(start, { bestNode->GetQ(), bestNode->GetR() }, moveRange, previous, next, reachedTarget))
	{
		MoveToNode(previous, next);
	}
}

void EnemyMovementComponent::MoveApproach()
{
	auto* owner = GetOwner();
	auto* enemy = owner ? owner->GetComponent<EnemyComponent>() : nullptr;
	if (!enemy || !m_GridSystem)
	{
		return;
	}

	const int moveRange = enemy->GetMoveDistance();
	if (moveRange <= 0)
	{
		return;
	}

	AxialKey playerKey{};
	bool hasPlayer = false;
	for (auto* node : m_GridSystem->GetNodes())
	{
		if (node && node->GetState() == NodeState::HasPlayer)
		{
			playerKey = { node->GetQ(), node->GetR() };
			hasPlayer = true;
			break;
		}
	}

	if (!hasPlayer)
	{
		return;
	}

	const AxialKey start{ enemy->GetQ(), enemy->GetR() };
	AxialKey previous{};
	AxialKey next{};
	bool reachedTarget = false;
	if (SelectMoveKeyTowardTarget(start, playerKey, moveRange, previous, next, reachedTarget))
	{
		MoveToNode(previous, next);
	}
}

void EnemyMovementComponent::MoveMaintainRange()
{
	auto* owner = GetOwner();
	auto* enemy = owner ? owner->GetComponent<EnemyComponent>() : nullptr;
	if (!enemy || !m_GridSystem)
	{
		return;
	}

	const int moveRange = enemy->GetMoveDistance();
	if (moveRange <= 0)
	{
		return;
	}

	AxialKey playerKey{};
	bool hasPlayer = false;
	for (auto* node : m_GridSystem->GetNodes())
	{
		if (node && node->GetState() == NodeState::HasPlayer)
		{
			playerKey = { node->GetQ(), node->GetR() };
			hasPlayer = true;
			break;
		}
	}

	if (!hasPlayer)
	{
		return;
	}

	int desiredRange = 3;
	if (auto* stat = owner->GetComponent<EnemyStatComponent>())
	{
		desiredRange = max(1, stat->GetMaxDiceValue());
	}

	const AxialKey start{ enemy->GetQ(), enemy->GetR() };
	const int currentDistance = m_GridSystem->GetShortestPathLength(start, playerKey);
	if (currentDistance >= 0 && std::abs(currentDistance - desiredRange) <= 1)
	{
		return;
	}

	NodeComponent* bestNode = nullptr;
	int bestDelta = std::numeric_limits<int>::max();

	for (auto* node : m_GridSystem->GetNodes())
	{
		if (!node || !node->GetIsMoveable() || node->GetState() != NodeState::Empty)
		{
			continue;
		}

		const AxialKey candidate{ node->GetQ(), node->GetR() };
		const int distanceFromStart = m_GridSystem->GetShortestPathLength(start, candidate);
		if (distanceFromStart <= 0 || distanceFromStart > moveRange)
		{
			continue;
		}

		const int distanceFromPlayer = m_GridSystem->GetShortestPathLength(candidate, playerKey);
		if (distanceFromPlayer < 0)
		{
			continue;
		}

		const int delta = std::abs(distanceFromPlayer - desiredRange);
		if (delta < bestDelta)
		{
			bestDelta = delta;
			bestNode = node;
		}
	}

	if (!bestNode)
	{
		return;
	}

	AxialKey previous{};
	AxialKey next{};
	bool reachedTarget = false;
	if (SelectMoveKeyTowardTarget(start, { bestNode->GetQ(), bestNode->GetR() }, moveRange, previous, next, reachedTarget))
	{
		MoveToNode(previous, next);
	}
}

void EnemyMovementComponent::RequestMoveToTarget()
{
	m_PendingOrder = EMoveOrder::Approach;
	m_IsMoveComplete = false;
}

void EnemyMovementComponent::RequestRunOff()
{
	m_PendingOrder = EMoveOrder::RunOff;
	m_IsMoveComplete = false;
}

void EnemyMovementComponent::RequestMaintainRange()
{
	m_PendingOrder = EMoveOrder::MaintainRange;
	m_IsMoveComplete = false;
}

void EnemyMovementComponent::RotateTowardTarget(int targetQ, int targetR)
{
	auto* owner = GetOwner();
	auto* enemy = owner ? owner->GetComponent<EnemyComponent>() : nullptr;
	auto* enemyTransform = owner ? owner->GetComponent<TransformComponent>() : nullptr;
	if (!enemy || !enemyTransform)
	{
		return;
	}

	if (!m_GridSystem)
	{
		GetSystem();
	}

	if (!m_GridSystem)
	{
		return;
	}

	const AxialKey startKey{ enemy->GetQ(), enemy->GetR() };
	const AxialKey targetKey{ targetQ, targetR };
	if (startKey.q == targetKey.q && startKey.r == targetKey.r)
	{
		return;
	}

	const auto path = m_GridSystem->GetShortestPath(startKey, targetKey);
	
	ERotationOffset rotation{};
	bool hasRotation = false;
	if (path.size() >= 2)
	{
		hasRotation = TryGetRotationFromStep(startKey, path[1], rotation);
	}
	if (!hasRotation)
	{
		hasRotation = TryGetRotationTowardTargetApprox(startKey, targetKey, rotation);
	}

	if (hasRotation)
	{
		SetEnemyRotation(enemyTransform, rotation);
		enemy->SetFacing(rotation);
		enemy->RefreshSightDebugLines();
	}
}

void EnemyMovementComponent::MovePatrol()
{
	auto* owner = GetOwner();
	auto* enemy = owner ? owner->GetComponent<EnemyComponent>() : nullptr;
	if (!enemy || !m_GridSystem)
		return;

	const int moveRange = enemy->GetMoveDistance();

	if (moveRange <= 0 || !m_GridSystem) {
		return; 
	}

	const AxialKey start{ enemy->GetQ(), enemy->GetR() };

	if (m_HasPatrolPoints)
	{
		for (size_t attempt = 0; attempt < m_PatrolPoints.size(); ++attempt)
		{
			const PatrolPoint& patrolPoint = m_PatrolPoints[m_PatrolIndex];
			const AxialKey target{ patrolPoint.q, patrolPoint.r };

			if (start.q == target.q && start.r == target.r)
			{
				m_PatrolIndex = (m_PatrolIndex + 1) % m_PatrolPoints.size();
				continue;
			}

			AxialKey previous{};
			AxialKey next{};
			bool reachedTarget = false;
			if (SelectMoveKeyTowardTarget(start, target, moveRange, previous, next, reachedTarget))
			{
				if (MoveToNode(previous, next) && reachedTarget)
				{
					m_PatrolIndex = (m_PatrolIndex + 1) % m_PatrolPoints.size();
				}
				return;
			}

			break;
		}
	}

	NodeComponent* bestNode = nullptr;
	int bestDistance = 100;

	for (auto* node : m_GridSystem->GetNodes())
	{
		if (!node) continue;
		if (!node->GetIsMoveable() || node->GetState() != NodeState::Empty)
			continue;

		const AxialKey target{ node->GetQ(), node->GetR() };
		const int distance = m_GridSystem->GetShortestPathLength(start, target);
		if (distance <= 0 || distance > moveRange)
			continue;

		if (distance < bestDistance)
		{
			bestDistance = distance;
			bestNode = node;
		}
	}

	if (!bestNode)
		return;

	const AxialKey target{ bestNode->GetQ(), bestNode->GetR() };
	AxialKey previous{};
	AxialKey next{};
	bool reachedTarget = false;
	if (SelectMoveKeyTowardTarget(start, target, moveRange, previous, next, reachedTarget))
	{
		MoveToNode(previous, next);
	}
}

void EnemyMovementComponent::SetPatrolPoints(const std::array<PatrolPoint, 3>& points)
{
	m_PatrolPoints = points;
	m_HasPatrolPoints = HasValidPatrolPoints(m_PatrolPoints);
	m_PatrolIndex = 0;
}

void EnemyMovementComponent::SetEnemyRotation(TransformComponent* transComp, ERotationOffset dir)
{
	if (!transComp)
	{
		return;
	}

	switch (dir)
	{
	case ERotationOffset::clock_1:
		transComp->SetRotationEuler({ 0.0f,-150.0f ,0.0f });
		break;
	case ERotationOffset::clock_3:
		transComp->SetRotationEuler({ 0.0f,-90.0f ,0.0f });
		break;
	case ERotationOffset::clock_5:
		transComp->SetRotationEuler({ 0.0f,-30.0f ,0.0f });
		break;
	case ERotationOffset::clock_7:
		transComp->SetRotationEuler({ 0.0f,30.0f ,0.0f });
		break;
	case ERotationOffset::clock_9:
		transComp->SetRotationEuler({ 0.0f,90.0f ,0.0f });
		break;
	case ERotationOffset::clock_11:
		transComp->SetRotationEuler({ 0.0f,150.0f ,0.0f });
		break;
	default:
		break;
	}
}

bool EnemyMovementComponent::SelectMoveKeyTowardTarget(const AxialKey& start, const AxialKey& target, int moveRange, AxialKey& outPrevious, AxialKey& outNext, bool& outReachedTarget) const
{
	if (!m_GridSystem || moveRange <= 0)
	{
		return false;
	}

	const auto path = m_GridSystem->GetShortestPath(start, target);
	if (path.size() <= 1)
	{
		return false;
	}

	const size_t maxIndex = min(path.size() - 1, static_cast<size_t>(moveRange));
	size_t chosenIndex = 0;

	for (size_t i = maxIndex; i > 0; --i)
	{
		NodeComponent* node = m_GridSystem->GetNodeByKey(path[i]);
		if (!node || !node->GetIsMoveable() || node->GetState() != NodeState::Empty)
		{
			continue;
		}

		chosenIndex = i;
		break;
	}

	if (chosenIndex == 0)
	{
		return false;
	}

	outPrevious = path[chosenIndex - 1];
	outNext = path[chosenIndex];
	outReachedTarget = chosenIndex == path.size() - 1;
	return true;
}


bool EnemyMovementComponent::MoveToNode(const AxialKey& previous, const AxialKey& current)
{
	auto* owner = GetOwner();
	auto* enemy = owner ? owner->GetComponent<EnemyComponent>() : nullptr;
	auto* enemyTransform = owner ? owner->GetComponent<TransformComponent>() : nullptr;
	if (!enemy || !enemyTransform || !m_GridSystem)
	{
		return false;
	}

	auto* targetNode = m_GridSystem->GetNodeByKey(current);
	if (!targetNode)
	{
		return false;
	}

	auto* targetOwner = targetNode->GetOwner();
	auto* targetTransform = targetOwner ? targetOwner->GetComponent<TransformComponent>() : nullptr;
	if (!targetTransform)
	{
		return false;
	}

	ERotationOffset rotation{};
	if (TryGetRotationFromStep(previous, current, rotation))
	{
		SetEnemyRotation(enemyTransform, rotation);
		enemy->SetFacing(rotation);
	}

	enemyTransform->SetPosition(targetTransform->GetPosition());
	if (!(previous == current))
	{
		m_GridSystem->UpdateActorNodeState(previous, current, NodeState::HasEnemy);
	}
	enemy->SetQR(current.q, current.r);
	//Sound
	auto* scene = owner->GetScene();
	if (scene)
	{
		auto& services = scene->GetServices();
		if (services.Has<SoundManager>())
		{
			services.Get<SoundManager>().SFX_Shot(L"Move_Player");
		}
	}

	return true;
}


bool EnemyMovementComponent::HasValidPatrolPoints(const std::array<PatrolPoint, 3>& points) const
{
	for (const auto& point : points)
	{
		if (point.q != 0 || point.r != 0)
		{
			return true;
		}
	}

	return false;
}


void EnemyMovementComponent::GetSystem()
{
	auto* scene = GetOwner() -> GetScene();
	if (!scene){ return;}
	const auto& objects = scene->GetGameObjects();
	for (const auto& [name, object] : objects)
	{
		if (!object)
		{
			continue;
		}

		if (auto* grid = object->GetComponent<GridSystemComponent>())
		{
			m_GridSystem = grid;
		}
	}
}
