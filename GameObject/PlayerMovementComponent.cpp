#include "PlayerMovementComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Event.h"
#include "TransformComponent.h"
#include "ServiceRegistry.h"
#include "InputManager.h"
#include "Scene.h"
#include "RayHelper.h"
#include "CameraObject.h"
#include "BoxColliderComponent.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "GridSystemComponent.h"
#include "PlayerMoveFSMComponent.h"
#include "PlayerFSMComponent.h"
#include "EnemyComponent.h"
#include "GameObject.h"
#include "GameState.h"
#include "GameManager.h"
#include <array>
#include <cfloat>

REGISTER_COMPONENT(PlayerMovementComponent)
REGISTER_PROPERTY(PlayerMovementComponent, DragSpeed)
REGISTER_PROPERTY(PlayerMovementComponent, HoldThreshold)

static bool TryGetRotationFromStep(const AxialKey& previous, const AxialKey& current, RotationOffset& outDir)
{
	const AxialKey delta{ current.q - previous.q, current.r - previous.r };
	constexpr std::array<std::pair<AxialKey, RotationOffset>, 6> kDirections{ {
		{ { 1, 0 }, RotationOffset::clock_3 },
		{ { 1, -1 }, RotationOffset::clock_5 },
		{ { 0, -1 }, RotationOffset::clock_7 },
		{ { -1, 0 }, RotationOffset::clock_9 },
		{ { -1, 1 }, RotationOffset::clock_11 },
		{ { 0, 1 }, RotationOffset::clock_1 }
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
namespace
{
	void DispatchPlayerStateEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerFSMComponent>())
			fsm->DispatchEvent(eventName);
	}

	void DispatchMoveEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerMoveFSMComponent>())
			fsm->DispatchEvent(eventName);
	}
}

PlayerMovementComponent::PlayerMovementComponent()
{
}

PlayerMovementComponent::~PlayerMovementComponent()
{
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().RemoveListener(EventType::Dragged, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClickUp, this);
	GetEventDispatcher().RemoveListener(EventType::MouseRightClick, this);
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
}

void PlayerMovementComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::KeyDown, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().AddListener(EventType::Dragged, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClickUp, this);
	GetEventDispatcher().AddListener(EventType::MouseRightClick, this);
	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
		return;

	const auto& objects = scene->GetGameObjects();
	for (const auto& [name, object] : objects)
	{
		if (!object)
			continue;

		if (auto* grid = object->GetComponent<GridSystemComponent>())
		{
			m_GridSystem = grid;
			break;
		}
	}
}

void PlayerMovementComponent::Update(float deltaTime)
{
	if (m_IsWaitingForDrag)
	{
		auto* owner = GetOwner();
		if (!owner) return;

		auto* scene = owner->GetScene();
		if (!scene || !scene->GetServices().Has<InputManager>()) return;

		auto& input = scene->GetServices().Get<InputManager>();

		// Mouse released: treat as short click.
		if (!input.IsLeftPressed())
		{
			m_IsWaitingForDrag = false;
			if (m_ObjectBehindPlayer)
			{
				auto* player = owner->GetComponent<PlayerComponent>();
				if (player)
				{
					if (auto* enemy = player->ResolveCombatTarget(m_ObjectBehindPlayer))
					{
						player->HandleCombatClick(enemy);
					}
				}
			}
			m_ObjectBehindPlayer = nullptr;
			m_HoldTimer = 0.0f;
			return;
		}

		m_HoldTimer += deltaTime;
		if (m_HoldTimer >= m_HoldThreshold)
		{
			m_IsWaitingForDrag = false;

			auto* transComp = owner->GetComponent<TransformComponent>();
			auto* player = owner->GetComponent<PlayerComponent>();
			if (transComp && player)
			{
				player->ClearCombatSelection();
				m_HasDragRay = true;
				m_DragStartPos = transComp->GetPosition();
				m_DragStartNode = m_GridSystem ? m_GridSystem->GetNodeByKey({ player->GetQ(), player->GetR() }) : nullptr;

				DispatchPlayerStateEvent(owner, "Move_Start");
				DispatchMoveEvent(owner, "Move_Select");
			}
			m_ObjectBehindPlayer = nullptr;
		}
	}
}

void PlayerMovementComponent::OnEvent(EventType type, const void* data)
{
	// 턴 변경: 플레이어 턴 아닐 때는 Cancel 이벤트만 발행
	if (type == EventType::TurnChanged)
	{
		const auto* payload = static_cast<const Events::TurnChanged*>(data);
		if (!payload) return;

		if (static_cast<Turn>(payload->turn) != Turn::PlayerTurn)
		{
			auto* owner = GetOwner();
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Revoke");
			if (auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr)
			{
				player->ClearCombatSelection();
			}

			// 입력 상태 정리(프리뷰/원복은 FSM 액션에서 처리)
			m_HasDragRay = false;
			m_DragStartNode = nullptr;
			m_IsWaitingForDrag = false;
			m_ObjectBehindPlayer = nullptr;
		}
		return;
	}

	const auto* mouseData = static_cast<const Events::MouseState*>(data);
	if (!mouseData) return;
	if (mouseData->handled) return;

	auto* owner = GetOwner();
	if (!owner) return;

	auto* scene = owner->GetScene();
	if (!scene) return;

	auto* gameManager = scene->GetGameManager();
	//if (gameManager && !gameManager->IsExplorationInputAllowed())
	// 탐새(이동) 입력과/  전투입력
	if (gameManager)
	{
		const bool allowInput = gameManager->IsExplorationInputAllowed() || gameManager->IsCombatInputAllowed();
		if (!allowInput)
		{
			return;
		}
	}

	if (!scene->GetServices().Has<InputManager>())
		return;

	auto& input = scene->GetServices().Get<InputManager>();
	auto camera = scene->GetGameCamera();
	if (!camera) return;

	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!transComp) return;

	auto* player = owner->GetComponent<PlayerComponent>();
	if (!player) return;

	auto* moveFsm = owner->GetComponent<PlayerMoveFSMComponent>();
	if (!moveFsm) return;

	if (type == EventType::KeyDown)
	{
		auto* player = owner->GetComponent<PlayerComponent>();


		// Turn Check.
		if(!player || player->GetCurrentTurn() != Turn::PlayerTurn)
		{
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Revoke");
			m_HasDragRay = false;
			return;
		}

		auto keyData = static_cast<const Events::KeyEvent*>(data);
		if (!keyData) return;

		if(keyData->key == VK_ESCAPE)
			DispatchMoveEvent(owner, "Move_Revoke");
	}

	if (type == EventType::MouseRightClick)
	{
		player->ClearCombatSelection();
		// 턴 아니면 cancel
		if (player->GetCurrentTurn() != Turn::PlayerTurn)
		{
			if (moveFsm->HasPendingTarget())
			{
				const int tQ = moveFsm->GetPeningQ();
				const int tR = moveFsm->GetPeningR();
				if (!player->CommitMove(tQ, tR))
					transComp->SetPosition(m_DragStartPos);
				else
					ApplyRotationForMove(tQ, tR);
			}
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Revoke");
			player->ClearCombatSelection();
			m_HasDragRay = false;
			m_IsWaitingForDrag = false;
			m_ObjectBehindPlayer = nullptr;
			return;
		}

		DispatchMoveEvent(owner, "Move_Revoke");
		player->ClearCombatSelection();
	}

	// MouseUp: Commit/Cancel 의사만 FSM에 전달
	if (type == EventType::MouseLeftClickUp)
	{
		// 턴 아니면 cancel
		if (player->GetCurrentTurn() != Turn::PlayerTurn)
		{
			DispatchPlayerStateEvent(owner, "Move_Cancel");
			DispatchMoveEvent(owner, "Move_Revoke");
			player->ClearCombatSelection();
			m_HasDragRay = false;
			m_IsWaitingForDrag = false;
			m_ObjectBehindPlayer = nullptr;
			return;
		}

		if (m_IsWaitingForDrag)
		{
			m_IsWaitingForDrag = false;
			if (m_ObjectBehindPlayer)
			{
				if (auto* enemy = player->ResolveCombatTarget(m_ObjectBehindPlayer))
				{
					player->HandleCombatClick(enemy);
				}
			}
			m_ObjectBehindPlayer = nullptr;
			return;
		}

		bool hasMoved = false;
		if (moveFsm->HasPendingTarget() && m_DragStartNode)
		{
			if (moveFsm->GetPeningQ() != m_DragStartNode->GetQ()
				|| moveFsm->GetPeningR() != m_DragStartNode->GetR())
			{
				hasMoved = true;
			}
		}

		if (hasMoved)
		{
			DispatchMoveEvent(owner, "Move_Confirm");
		}
		else
		{
			DispatchMoveEvent(owner, "Move_Revoke");
		}

		m_HasDragRay = false;
		return;
	}

	if (!input.IsPointInViewport(mouseData->pos))
		return;

	// MouseDown: Select 의사만 FSM에 전달
	if (type == EventType::MouseLeftClick || type == EventType::MouseLeftDoubleClick)
	{
		if (player->GetCurrentTurn() != Turn::PlayerTurn)
			return;

		Ray pickRay{};
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
			return;

		auto& gameObjects = scene->GetGameObjects();
		float enemyT = FLT_MAX;
		EnemyComponent* bestEnemy = nullptr;
		float otherT = FLT_MAX;
		GameObject* bestOther = nullptr;

		for (const auto& [name, object] : gameObjects)
		{
			if (!object) continue;

			auto* otherCollider = object->GetComponent<BoxColliderComponent>();
			if (!otherCollider || !otherCollider->HasBounds())
				continue;

			float hitT = 0.0f;
			if (!otherCollider->IntersectsRay(pickRay.m_Pos, pickRay.m_Dir, hitT))
				continue;

			if (object.get() == owner)
			{
				continue;
			}

			if (auto* enemy = player->ResolveCombatTarget(object.get()))
			{
				if (hitT < enemyT)
				{
					enemyT = hitT;
					bestEnemy = enemy;
				}
			}
			else
			{
				if (hitT < otherT)
				{
					otherT = hitT;
					bestOther = object.get();
				}
			}
		}


		bool tileUnderPlayerHit = false;
		if (bestOther)
		{
			if (auto* node = bestOther->GetComponent<NodeComponent>())
			{
				if (node->GetQ() == player->GetQ() && node->GetR() == player->GetR())
				{
					tileUnderPlayerHit = true;
				}
			}
		}

		if (tileUnderPlayerHit)
		{
			m_IsWaitingForDrag = true;
			m_HoldTimer = 0.0f;
			m_ClickStartPos = mouseData->pos;

			if (bestEnemy)
				m_ObjectBehindPlayer = static_cast<GameObject*>(bestEnemy->GetOwner());
			else
				m_ObjectBehindPlayer = bestOther;

			auto* nodeTransform = bestOther ? bestOther->GetComponent<TransformComponent>() : nullptr;
			if (nodeTransform)
				m_DragOffset = { 0.0f, transComp->GetPosition().y - nodeTransform->GetPosition().y, 0.0f };
			else
				m_DragOffset = { 0.0f, 0.0f, 0.0f };

			m_DragRayOrigin = pickRay.m_Pos;
			m_DragRayDir = pickRay.m_Dir;
			return;
		}

		m_IsWaitingForDrag = false;
		if (bestEnemy)
		{
			player->HandleCombatClick(bestEnemy);
			return;
		}
		player->ClearCombatSelection();
		return;
	}

	// Dragged: 레이만 갱신
	if (type != EventType::Dragged)
		return;

	if (m_IsWaitingForDrag)
	{
		const float dx = static_cast<float>(mouseData->pos.x - m_ClickStartPos.x);
		const float dy = static_cast<float>(mouseData->pos.y - m_ClickStartPos.y);
		if (dx * dx + dy * dy > 100.0f)
		{
			m_IsWaitingForDrag = false;
			m_ObjectBehindPlayer = nullptr;
			player->ClearCombatSelection();

			auto* transComp = owner->GetComponent<TransformComponent>();
			auto* player = owner->GetComponent<PlayerComponent>();
			if (transComp && player)
			{
				m_HasDragRay = true;
				m_DragStartPos = transComp->GetPosition();
				m_DragStartNode = m_GridSystem ? m_GridSystem->GetNodeByKey({ player->GetQ(), player->GetR() }) : nullptr;

				DispatchPlayerStateEvent(owner, "Move_Start");
				DispatchMoveEvent(owner, "Move_Select");
			}
		}
	}

	// 드래그 활성은 FSM이 관리하지만, 레이는 계속 갱신해줘야 프리뷰가 움직임
	if (!moveFsm->IsDraggingActive())
		return;

	Ray dragRay{};
	if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, dragRay))
		return;

	m_DragRayOrigin = dragRay.m_Pos;
	m_DragRayDir = dragRay.m_Dir;
	m_HasDragRay = true;
}

void PlayerMovementComponent::ApplyRotationForMove(int targetQ, int targetR)
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!transComp || !m_GridSystem)
		return;

	auto* player = owner->GetComponent<PlayerComponent>();
	if (!player)
		return;

	const AxialKey startKey = m_DragStartNode
		? AxialKey{ m_DragStartNode->GetQ(), m_DragStartNode->GetR() }
	: AxialKey{ player->GetQ(), player->GetR() };
	const AxialKey targetKey{ targetQ, targetR };
	const auto path = m_GridSystem->GetShortestPath(startKey, targetKey);

	// 경로가 2개 이상일 때 마지막 스텝 방향을 사용한다. (직전 노드 -> 목적지)
	if (path.size() < 2)
		return;

	const AxialKey& previousKey = path[path.size() - 2];
	const AxialKey& currentKey = path.back();
	RotationOffset rotation{};
	if (TryGetRotationFromStep(previousKey, currentKey, rotation))
	{
		SetPlayerRotation(transComp, rotation);
	}
}


void PlayerMovementComponent::RotateTowardTarget(int targetQ, int targetR)
{
	auto* owner = GetOwner();
	if (!owner || !m_GridSystem)
	{
		return;
	}

	auto* player = owner->GetComponent<PlayerComponent>();
	auto* transComp = owner->GetComponent<TransformComponent>();
	if (!player || !transComp)
	{
		return;
	}

	const AxialKey startKey{ player->GetQ(), player->GetR() };
	const AxialKey targetKey{ targetQ, targetR };
	if (startKey.q == targetKey.q && startKey.r == targetKey.r)
	{
		return;
	}

	const auto path = m_GridSystem->GetShortestPath(startKey, targetKey);
	if (path.size() < 2)
	{
		return;
	}

	RotationOffset rotation{};
	if (TryGetRotationFromStep(startKey, path[1], rotation))
	{
		SetPlayerRotation(transComp, rotation);
	}
}


bool PlayerMovementComponent::IsDragging() const
{
	auto* owner = GetOwner();
	auto* moveFSM = owner ? owner->GetComponent<PlayerMoveFSMComponent>() : nullptr;
	return moveFSM && moveFSM->IsDraggingActive();
}



void PlayerMovementComponent::SetPlayerRotation(TransformComponent* transComp, RotationOffset dir)
{
	if (!transComp){return;}

	switch (dir)
	{
	case RotationOffset::clock_1:
		transComp->SetRotationEuler({ 0.0f,-150.0f ,0.0f });
		break;
	case RotationOffset::clock_3:
		transComp->SetRotationEuler({ 0.0f,-90.0f ,0.0f });
		break;
	case RotationOffset::clock_5:
		transComp->SetRotationEuler({ 0.0f,-30.0f ,0.0f });
		break;
	case RotationOffset::clock_7:
		transComp->SetRotationEuler({ 0.0f,30.0f ,0.0f });
		break;
	case RotationOffset::clock_9:
		transComp->SetRotationEuler({ 0.0f,90.0f ,0.0f });
		break;
	case RotationOffset::clock_11:
		transComp->SetRotationEuler({ 0.0f,150.0f ,0.0f });
		break;
	default:
		break;
	}
}
