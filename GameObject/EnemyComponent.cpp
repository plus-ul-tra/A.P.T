#include "EnemyComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "AIController.h"
#include "BTExecutor.h"
#include "Blackboard.h"
#include "BlackboardKeys.h"
#include "CombatBehaviorTreeFactory.h"
#include "TransformComponent.h"
#include "EnemyStatComponent.h"
#include "GameObject.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "GameManager.h"
#include "Scene.h"
#include "EnemyMovementComponent.h"
#include "GridSystemComponent.h"
#include "NodeComponent.h"
#include "PlayerCombatFSMComponent.h"
#include "MeshRenderer.h"
#include "SkeletalMeshRenderer.h"
#include "SkinningAnimationComponent.h"
#include "ServiceRegistry.h"
#include "BoxColliderComponent.h"
#include "CombatManager.h"
#include "AssetLoader.h"
#include <array>
#include <cmath>
#include <algorithm>
#include < utility >
#include <iostream>

REGISTER_COMPONENT(EnemyComponent)
REGISTER_PROPERTY_READONLY(EnemyComponent, Q)
REGISTER_PROPERTY_READONLY(EnemyComponent, R)
REGISTER_PROPERTY(EnemyComponent, MoveDistance)
REGISTER_PROPERTY(EnemyComponent, DebugSightLines)
REGISTER_PROPERTY(EnemyComponent, EndTurnDelay)
REGISTER_PROPERTY_HANDLE(EnemyComponent, DeathAnimationHandle)
REGISTER_PROPERTY_HANDLE(EnemyComponent, HoverInfoTextureHandle)
REGISTER_PROPERTY(EnemyComponent, DeathAnimation)
REGISTER_PROPERTY(EnemyComponent, DeathAnimationBlendTime)
REGISTER_PROPERTY(EnemyComponent, UseDeathAnimationBlend)

namespace
{
	void SetRenderAndCollisionVisibleRecursive(TransformComponent* transform, bool visible)
	{
		if (!transform)
		{
			return;
		}

		if (auto* owner = transform->GetOwner())
		{
			if (auto* meshRenderer = owner->GetComponent<MeshRenderer>())
			{
				meshRenderer->SetVisible(visible);
			}
			if (auto* skeletalRenderer = owner->GetComponent<SkeletalMeshRenderer>())
			{
				skeletalRenderer->SetVisible(visible);
			}
			if (auto* collider = owner->GetComponent<BoxColliderComponent>())
			{
				collider->SetIsActive(visible);
			}
		}

		for (auto* child : transform->GetChildrens())
		{
			SetRenderAndCollisionVisibleRecursive(child, visible);
		}
	}
}

EnemyComponent::EnemyComponent() {
	m_Facing = ERotationOffset::clock_9;
}

EnemyComponent::~EnemyComponent() {
	// Event Listener 쓰는 경우만
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
}

void EnemyComponent::SetDeathAnimationHandle(const AnimationHandle& value)
{
	m_DeathAnimationHandle = value;
	if (auto* loader = AssetLoader::GetActive())
	{
		loader->GetAnimationAssetReference(value, m_DeathAnimation.assetPath, m_DeathAnimation.assetIndex);
	}
}

void EnemyComponent::Start()
{
	m_BTExecutor = std::make_unique<BTExecutor>();
	m_BTExecutor->SetRoot(CombatBehaviorTreeFactory::BuildDefaultTree(&GetEventDispatcher()));
	m_AIController = std::make_unique<AIController>(*m_BTExecutor);
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return;
	}

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		(void)name;
		if (!object)
		{
			continue;
		}

		if (!m_GridSystem)
		{
			if (auto* grid = object->GetComponent<GridSystemComponent>())
			{
				m_GridSystem = grid;
			}
		}

		if (auto* player = object->GetComponent<PlayerComponent>())
		{
			m_TargetTransform = object->GetComponent<TransformComponent>();
			m_TargetPlayer = player;
			if (m_GridSystem)
			{
				break;
			}
		}
	}
}


///  시야 판별
//-----------------------------------
struct AxialDirection
{
	int q = 0;
	int r = 0;
};

constexpr std::array<AxialDirection, 6> kFacingDirections{ {
	{ 0, 1 },   // clock_1
	{ 1, 0 },   // clock_3
	{ 1, -1 },  // clock_5
	{ 0, -1 },  // clock_7
	{ -1, 0 },  // clock_9
	{ -1, 1 }   // clock_11
} };

int AxialDistance(int q1, int r1, int q2, int r2)
{
	const int dq = q1 - q2;
	const int dr = r1 - r2;
	const int ds = dq + dr;
	return (std::abs(dq) + std::abs(dr) + std::abs(ds)) / 2;
}

std::pair<AxialDirection, AxialDirection> GetLateralDirections(int facingIndex)
{
	
	const int dirCount = static_cast<int>(kFacingDirections.size());
	const int leftIndex = (facingIndex + dirCount - 1) % dirCount;
	const int rightIndex = (facingIndex + 1) % dirCount;
	return { kFacingDirections[leftIndex], kFacingDirections[rightIndex] };
}

bool IsTargetVisibleOnHexLine(
	GridSystemComponent* grid,
	int selfQ,
	int selfR,
	int facingIndex,
	int sightRange)
{
	if (!grid || sightRange <= 0)
		return false;

	const AxialDirection forwardDir = kFacingDirections[facingIndex];

	// 좌우 lateral 방향
	auto lateralDirs = GetLateralDirections(facingIndex);
	AxialDirection leftDir = lateralDirs.first;
	AxialDirection rightDir = lateralDirs.second;

	// 3개의 lane 시작점 origin
	std::array<std::pair<int, int>, 3> laneOrigins =
	{
		std::make_pair(selfQ + leftDir.q,  selfR + leftDir.r),   // 위줄
		std::make_pair(selfQ+ forwardDir.q,             selfR+ forwardDir.r),               // 가운데줄
		std::make_pair(selfQ + rightDir.q, selfR + rightDir.r)  // 아래줄
	};

	// lane별 block 상태
	std::array<bool, 3> blocked{ false, false, false };

	// 각 lane을 forward로 쭉 검사
	for (int step = 0; step < sightRange; ++step)
	{
		for (int lane = 0; lane < 3; ++lane)
		{
			if (blocked[lane])
				continue;

			int q = laneOrigins[lane].first + forwardDir.q * step;
			int r = laneOrigins[lane].second + forwardDir.r * step;

			NodeComponent* node = grid->GetNodeByKey({ q, r });

			// 막히면 그 lane은 끝
			if (!node || !node->GetIsSight())
			{
				blocked[lane] = true;
				continue;
			}

			// 플레이어 발견
			if (node->GetState() == NodeState::HasPlayer)
				return true;
		}
	}

	return false;
}

//-----------------------------------

void EnemyComponent::SyncFacingFromTransform()
{
	auto* owner = GetOwner();
	auto* transform = owner ? owner->GetComponent<TransformComponent>() : nullptr;
	if (!transform)
	{
		return;
	}

	const XMFLOAT3 forward = transform->GetForward();
	const XMFLOAT2 flatForward{ forward.x, forward.z };
	const float flatLengthSq = flatForward.x * flatForward.x + flatForward.y * flatForward.y;
	if (flatLengthSq <= 1e-6f)
	{
		return;
	}

	const float invLen = 1.0f / std::sqrt(flatLengthSq);
	const XMFLOAT2 normalizedForward{ flatForward.x * invLen, flatForward.y * invLen };

	constexpr std::array<std::pair<ERotationOffset, float>, 6> kYawCandidates{ {
		{ ERotationOffset::clock_1, -150.0f },
		{ ERotationOffset::clock_3, -90.0f },
		{ ERotationOffset::clock_5, -30.0f },
		{ ERotationOffset::clock_7, 30.0f },
		{ ERotationOffset::clock_9, 90.0f },
		{ ERotationOffset::clock_11, 150.0f }
	} };

	ERotationOffset bestFacing = m_Facing;
	float bestDot = -std::numeric_limits<float>::infinity();

	for (const auto& [candidateFacing, yawDeg] : kYawCandidates)
	{
		const float yawRad = XMConvertToRadians(yawDeg);
		const XMFLOAT2 candidateForward{ std::sin(yawRad), std::cos(yawRad) };
		const float dot = normalizedForward.x * candidateForward.x
			+ normalizedForward.y * candidateForward.y;
		if (dot > bestDot)
		{
			bestDot = dot;
			bestFacing = candidateFacing;
		}
	}

	m_Facing = bestFacing;
}


void EnemyComponent::RefreshSightDebugLines()
{
	if (!m_DebugSightLines)
	{
		ClearSightDebug();
		return;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		ClearSightDebug();
		return;
	}

	float sightDistance = 0.0f;
	if (auto* stat = owner->GetComponent<EnemyStatComponent>())
	{
		sightDistance = stat->GetSightDistance();
	}
	else
	{
		sightDistance = 100.0f;
	}

	const int sightRange = static_cast<int>(std::floor(sightDistance));
	UpdateSightDebugLines(sightRange);
}

void EnemyComponent::ClearSightDebug()
{
	for (auto* node : m_SightDebugNodes)
	{
		if (node)
		{
			node->SetSightHighlight(0.0f, false);
		}
	}
	m_SightDebugNodes.clear();
}

void EnemyComponent::UpdateSightDebugLines(int sightRange)
{
	if (!m_GridSystem || sightRange <= 0)
	{
		ClearSightDebug();
		return;
	}

	const int facingIndex = static_cast<int>(m_Facing);
	const int dirCount = static_cast<int>(kFacingDirections.size());
	if (facingIndex < 0 || facingIndex >= dirCount)
	{
		ClearSightDebug();
		return;
	}

	ClearSightDebug();

	const AxialDirection forwardDir = kFacingDirections[facingIndex];
	auto lateralDirs = GetLateralDirections(facingIndex);
	const AxialDirection leftDir = lateralDirs.first;
	const AxialDirection rightDir = lateralDirs.second;

	// 원하는 형태: 위/가운데/아래 3줄이 "캐릭터 인접칸"에서 출발
	std::array<std::pair<int, int>, 3> laneOrigins =
	{
		std::make_pair(m_Q + leftDir.q,  m_R + leftDir.r),       // 위줄 시작: 인접칸
		std::make_pair(m_Q + forwardDir.q,   m_R+ forwardDir.r), // 가운데 시작: 본인
		std::make_pair(m_Q + rightDir.q, m_R + rightDir.r)       // 아래줄 시작: 인접칸
	};

	std::array<bool, 3> blocked{ false, false, false };

	// step을 0부터: 위/아래가 "인접칸"부터 바로 찍힘
	// 가운데는 self부터 찍히는 게 싫으면 lane==1만 step=1부터 시작하도록 분기하면 됨
	for (int step = 0; step < sightRange; ++step)
	{
		for (int lane = 0; lane < 3; ++lane)
		{
			if (blocked[lane]) continue;
			int startStep = (lane == 1) ? 1 : 0;
			int q = laneOrigins[lane].first + forwardDir.q * step;
			int r = laneOrigins[lane].second + forwardDir.r * step;

			NodeComponent* node = m_GridSystem->GetNodeByKey({ q, r });
			if (!node || !node->GetIsSight())
			{
				blocked[lane] = true;
				continue;
			}

			if (std::find(m_SightDebugNodes.begin(), m_SightDebugNodes.end(), node) == m_SightDebugNodes.end())
			{
				node->SetSightHighlight(0.6f, true);
				m_SightDebugNodes.push_back(node);
			}
		}
	}
}


void EnemyComponent::Update(float deltaTime) {
	auto* owner = GetOwner();
	if (!owner || !m_AIController)
	{
		return;
	}

	auto* scene = owner->GetScene();
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;

	SyncFacingFromTransform();
	
	if (gameManager && gameManager->GetPhase() == Phase::GameOver)
	{
		return;
	}

	if (gameManager && gameManager->GetPhase() == Phase::ExplorationLoop
		&& m_CurrentTurn == Turn::EnemyTurn
		&& m_ExploreTurnFinished)
	{
		return;
	}
	if (gameManager && gameManager->GetPhase() == Phase::ExplorationLoop
		&& m_CurrentTurn == Turn::EnemyTurn
		&& !m_ExploreTurnFinished)
	{
		if (gameManager->GetExplorationActiveEnemyActorId() != GetActorId())
		{
			m_ExploreDelayRemaining = max(0.0f, m_ExploreDelayRemaining - deltaTime);
			return;
		}
		if (!m_MoveRequested)
		{
			m_MoveRequested = true;
		}
	}

	if (!m_TargetPlayer && scene)
	{
		for (const auto& [name, object] : scene->GetGameObjects())
		{
			(void)name;
			if (!object)
			{
				continue;
			}

			if (!m_GridSystem)
			{
				if (auto* grid = object->GetComponent<GridSystemComponent>())
				{
					m_GridSystem = grid;
				}
			}

			if (auto* player = object->GetComponent<PlayerComponent>())
			{
				m_TargetTransform = object->GetComponent<TransformComponent>();
				m_TargetPlayer = player;
				if (m_GridSystem)
				{
					break;
				}
			}
		}
	}

	auto& bb = m_AIController->GetBlackboard();
	bb.Set(BlackboardKeys::IsInCombat, gameManager && gameManager->GetPhase() == Phase::TurnBasedCombat);
	float endTurnDelay = 0.0f;
	if (!bb.TryGet(BlackboardKeys::EndTurnDelay, endTurnDelay))
	{
		bb.Set(BlackboardKeys::EndTurnDelay, m_EndTurnDelay);
	}

	bool isAlive = true;
	if (auto* stat = owner->GetComponent<EnemyStatComponent>())
	{
		isAlive = !stat->IsDead();
	}

	if (isAlive) {
		m_DeathReported = false; 
		m_DeathAnimationStarted = false;
		m_DeathAnimationCompleted = false;
	}
	bb.Set(BlackboardKeys::IsAlive, isAlive);

	if (!isAlive)
	{
		auto* anim = owner->GetComponent<SkinningAnimationComponent>();
		if (!m_DeathAnimationStarted)
		{
			m_DeathAnimationStarted = true;
			m_DeathAnimationCompleted = true;

			AnimationHandle deathClip = m_DeathAnimationHandle;
			if (!deathClip.IsValid() && !m_DeathAnimation.assetPath.empty())
			{
				if (auto* loader = AssetLoader::GetActive())
				{
					deathClip = loader->ResolveAnimation(m_DeathAnimation.assetPath, m_DeathAnimation.assetIndex);
				}
			}

			if (anim && deathClip.IsValid())
			{
				if (m_UseDeathAnimationBlend)
				{
					anim->StartBlend(deathClip, max(0.0f, m_DeathAnimationBlendTime));
				}
				else
				{
					anim->SetClipHandle(deathClip);
				}

				auto playback = anim->GetPlayback();
				playback.looping = false;
				playback.playing = true;
				playback.reverse = false;
				anim->SetPlayback(playback);
				anim->SeekTime(0.0f);
				m_DeathAnimationCompleted = false;
			}
		}

		if (anim)
		{
			const auto& playback = anim->GetPlayback();
			if (!playback.playing)
			{
				m_DeathAnimationCompleted = true;
			}
		}

		const bool shouldHide = m_DeathAnimationCompleted;
		if (auto* transform = owner->GetComponent<TransformComponent>())
		{
			SetRenderAndCollisionVisibleRecursive(transform, !shouldHide);
		}


		m_TargetVisible = false;
		ClearSightDebug();
		if (!m_DeathReported && gameManager && gameManager->GetPhase() == Phase::TurnBasedCombat)
		{
			m_DeathReported = true;
			if (scene && scene->GetServices().Has<CombatManager>())
			{
				bool playerAlive = true;
				if (m_TargetPlayer)
				{
					if (auto* playerOwner = m_TargetPlayer->GetOwner())
					{
						if (auto* playerStat = playerOwner->GetComponent<PlayerStatComponent>())
						{
							playerAlive = !playerStat->IsDead();
						}
					}
				}

				bool enemiesRemaining = false;
				if (m_GridSystem)
				{
					for (auto* enemy : m_GridSystem->GetEnemies())
					{
						if (!enemy)
						{
							continue;
						}
						if (!scene->GetServices().Get<CombatManager>().IsActorInBattle(enemy->GetActorId()))
						{
							continue;
						}


						auto* enemyOwner = enemy->GetOwner();
						auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
						if (enemyStat && !enemyStat->IsDead())
						{
							enemiesRemaining = true;
							break;
						}
					}
				}

				scene->GetServices().Get<CombatManager>().UpdateBattleOutcome(playerAlive, enemiesRemaining);
			}
		}
		return;
	}

	if (auto* meshRenderer = owner->GetComponent<MeshRenderer>())
	{
		meshRenderer->SetVisible(true);
	}
	if (auto* skeletalRenderer = owner->GetComponent<SkeletalMeshRenderer>())
	{
		skeletalRenderer->SetVisible(true);
	}
	if (auto* collider = owner->GetComponent<BoxColliderComponent>())
	{
		collider->SetIsActive(true);
	}

	bool isInBattleActor = false;
	if (gameManager && gameManager->GetPhase() == Phase::TurnBasedCombat)
	{
		bool canActThisTurn = true;
		if (scene && scene->GetServices().Has<CombatManager>())
		{
			const auto& combatManager = scene->GetServices().Get<CombatManager>();
			isInBattleActor = combatManager.IsActorInBattle(GetActorId());
			if (isInBattleActor)
			{
				canActThisTurn = combatManager.CanAct(GetActorId());
			}
		}

		if (!isInBattleActor)
		{
			bb.Set(BlackboardKeys::IsInCombat, false);
			if (m_TargetPlayer)
			{
				const int joinRange = 1;
				const int distance = AxialDistance(m_Q, m_R, m_TargetPlayer->GetQ(), m_TargetPlayer->GetR());
				if (distance <= joinRange)
				{
					auto* playerOwner = m_TargetPlayer->GetOwner();
					if (playerOwner)
					{
						if (auto* combatFsm = playerOwner->GetComponent<PlayerCombatFSMComponent>())
						{
							combatFsm->RequestCombatEnter(m_TargetPlayer->GetActorId(), GetActorId());
						}
					}
				}
			}
		}
		else if (!canActThisTurn)
		{
			return;
		}
		else if (gameManager->GetCombatTurnState() != CombatTurnState::EnemyTurn)
		{
			return;
		}
	}

	bb.Set(BlackboardKeys::IsBattleActor, isInBattleActor);
	bb.Set(BlackboardKeys::ActorId, GetActorId());
	bb.Set(BlackboardKeys::SelfQ, m_Q);
	bb.Set(BlackboardKeys::SelfR, m_R);
	bb.Set(BlackboardKeys::FacingDirection, static_cast<int>(m_Facing));
	float sightDistance = 0.0f;

	int attackRange = 1;
	bool preferRanged = false;
	if (auto* stat = owner->GetComponent<EnemyStatComponent>())
	{
		sightDistance = stat->GetSightDistance();
		attackRange   = max(1, stat->GetAttackRange());
		preferRanged = attackRange > 1;
		bb.Set(BlackboardKeys::SightDistance, sightDistance);
		bb.Set(BlackboardKeys::SightAngle,    stat->GetSightAngle());
		bb.Set(BlackboardKeys::ThrowRange,    static_cast<float>(stat->GetMaxDiceValue()));
		bb.Set(BlackboardKeys::MeleeRange,    1.0f);
		bb.Set(BlackboardKeys::HP,			  stat->GetCurrentHP());
	}
	else
	{
		sightDistance = 100.0f;
		preferRanged = false;
		bb.Set(BlackboardKeys::SightDistance, sightDistance);
		bb.Set(BlackboardKeys::SightAngle, 180.0f);
		bb.Set(BlackboardKeys::ThrowRange, 3.0f);
		bb.Set(BlackboardKeys::MeleeRange, 1.0f);
		bb.Set(BlackboardKeys::HP, 30);
	}

	if (m_TargetPlayer)
	{
		bb.Set(BlackboardKeys::TargetQ, m_TargetPlayer->GetQ());
		bb.Set(BlackboardKeys::TargetR, m_TargetPlayer->GetR());
	}

	const bool hasHexData = m_GridSystem && m_TargetPlayer;
	bb.Set(BlackboardKeys::HasHexSightData, hasHexData);
	bool targetVisible = false;
	if (hasHexData)
	{
		const int sightRange = static_cast<int>(std::floor(sightDistance));

		if (m_DebugSightLines)
		{
			UpdateSightDebugLines(sightRange);
		}
		else
		{
			ClearSightDebug();
		}

		targetVisible = IsTargetVisibleOnHexLine(
			m_GridSystem,
			m_Q,
			m_R,
			static_cast<int>(m_Facing),
			sightRange);
		bb.Set(BlackboardKeys::HasTargetHexLine, targetVisible); // player 발견 -> 전투 연결
	}
	else if (m_DebugSightLines)
	{
		ClearSightDebug();
	}

	m_TargetVisible = targetVisible;

	if (gameManager && gameManager->GetPhase() == Phase::ExplorationLoop && m_TargetPlayer && hasHexData)
	{
		const int distance = AxialDistance(m_Q, m_R, m_TargetPlayer->GetQ(), m_TargetPlayer->GetR());
		if (targetVisible && distance <= attackRange)
		{
			auto* playerOwner = m_TargetPlayer->GetOwner();
			if (playerOwner)
			{
				if (auto* combatFsm = playerOwner->GetComponent<PlayerCombatFSMComponent>())
				{
					//combatFsm->RequestCombatEnter(GetActorId(), m_TargetPlayer->GetActorId());
					combatFsm->RequestCombatEnter(m_TargetPlayer->GetActorId(), GetActorId());
				}
			}
			return;
		}
	}

	bb.Set(BlackboardKeys::PreferRanged, preferRanged);
	bb.Set(BlackboardKeys::MaintainRange, false);

	m_AIController->Tick(deltaTime);

	if (gameManager && gameManager->GetPhase() == Phase::ExplorationLoop
		&& m_CurrentTurn == Turn::EnemyTurn)
	{
		bool exploreEndTurnRequested = false;
		if (bb.TryGet(BlackboardKeys::ExploreEndTurnRequested, exploreEndTurnRequested)
			&& exploreEndTurnRequested)
		{
			m_ExploreTurnFinished = true;
			bb.Set(BlackboardKeys::ExploreEndTurnRequested, false);
		}
	}

	//if (!gameManager || gameManager->GetPhase() != Phase::TurnBasedCombat)
	const bool canUseExplorationStyleMoveRequest =
		(!gameManager || gameManager->GetPhase() != Phase::TurnBasedCombat || !isInBattleActor);

	if (canUseExplorationStyleMoveRequest)
	{
		bool moveRequested = false;
		bool runOffRequested = false;
		bool maintainRangeRequested = false;
		if (bb.TryGet(BlackboardKeys::MoveRequested, moveRequested) && moveRequested)
		{
			m_MoveRequested = true;
			bb.Set(BlackboardKeys::MoveRequested, false);
		}
		if (bb.TryGet(BlackboardKeys::RequestRunOffMove, runOffRequested) && runOffRequested)
		{
			m_MoveRequested = true;
			bb.Set(BlackboardKeys::RequestRunOffMove, false);
		}
		if (bb.TryGet(BlackboardKeys::RequestMaintainRange, maintainRangeRequested) && maintainRangeRequested)
		{
			m_MoveRequested = true;
			bb.Set(BlackboardKeys::RequestMaintainRange, false);
		}
	}
}

void EnemyComponent::OnEvent(EventType type, const void* data)
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

	m_CurrentTurn = static_cast<Turn>(payload->turn);

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;

	if (gameManager && gameManager->GetPhase() == Phase::ExplorationLoop)
	{
		if (m_CurrentTurn != Turn::EnemyTurn)
		{
			m_ExploreTurnFinished = false;
			m_ExploreDelayRemaining = 0.0f;
			m_ExploreMoveIssued = false;
		}
	}

	if (m_CurrentTurn == Turn::EnemyTurn
		&& (!gameManager || gameManager->GetPhase() == Phase::ExplorationLoop))
	{
		m_ExploreTurnFinished = false;
		m_ExploreDelayRemaining = 0.0f;
		m_MoveRequested = false;
		m_ExploreMoveIssued = false;
	}
	else
	{
		m_MoveRequested = false;
		m_ExploreMoveIssued = false;
	}
}

bool EnemyComponent::ConsumeMoveRequest()
{
	if (!m_MoveRequested)
	{
		return false;
	}

	m_MoveRequested = false;
	return true;
}
