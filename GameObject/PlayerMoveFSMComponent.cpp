#include "PlayerMoveFSMComponent.h"
#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "GameObject.h"
#include "Object.h"
#include "Scene.h"
#include "TransformComponent.h"
#include "PlayerMovementComponent.h"
#include "PlayerFSMComponent.h"
#include "NodeComponent.h"
#include "BoxColliderComponent.h"
#include "Event.h"
#include "PlayerVisualPresetComponent.h"
#include "ServiceRegistry.h"
#include "SoundManager.h"

REGISTER_COMPONENT_DERIVED(PlayerMoveFSMComponent, FSMComponent)

static NodeComponent* FindClosestNodeHit(
	Scene* scene,
	const DirectX::XMFLOAT3& rayOrigin,
	const DirectX::XMFLOAT3& rayDir,
	float& outT)
{
	if (!scene) return nullptr;

	auto& gameObjects = scene->GetGameObjects();

	float closestT = FLT_MAX;
	NodeComponent* closestNode = nullptr;

	for (const auto& [name, object] : gameObjects)
	{
		if (!object) continue;

		auto* node = object->GetComponent<NodeComponent>();
		if (!node) continue;

		auto* col = object->GetComponent<BoxColliderComponent>();
		if (!col || !col->HasBounds()) continue;

		float t = 0.0f;
		if (!col->IntersectsRay(rayOrigin, rayDir, t)) continue;

		if (t >= 0.0f && t < closestT)
		{
			closestT = t;
			closestNode = node;
		}
	}

	if (!closestNode)
		return nullptr;

	outT = closestT;
	return closestNode;
}


PlayerMoveFSMComponent::PlayerMoveFSMComponent()
{
	// 이동 시작
	BindActionHandler("Move_Begin", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (player)
			{
				player->BeginMove();
			}
		});

	// 드래그 활성화: Selecting 진입 시 걸어두는 액션으로 사용
	BindActionHandler("Move_DragBegin", [this](const FSMAction&)
		{
			m_DraggingActive = true;
			m_CommitSucceeded = false;
			m_LastValid = false;
			ClearPendingTarget();
		});

	// 드래그 비활성화: Selecting 종료 시 걸어두는 액션으로 사용
	BindActionHandler("Move_DragEnd", [this](const FSMAction&)
		{
			m_LastValid = false;
		});

	// pending(q,r)로 Commit (Execute 진입 액션)
	BindActionHandler("Move_CommitPending", [this](const FSMAction&)
		{
			if (!m_HasPendingTarget)
				return;

			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
				return;

			if (auto* moveComp = owner->GetComponent<PlayerMovementComponent>())
			{
				moveComp->ApplyRotationForMove(m_PendingQ, m_PendingR);
			}

			const bool committed = player->CommitMove(m_PendingQ, m_PendingR);

			if (!committed)
			{
				auto* trans = owner->GetComponent<TransformComponent>();
				auto* moveComp = owner->GetComponent<PlayerMovementComponent>();
				if (trans && moveComp)
				{
					trans->SetPosition(moveComp->GetDragStartPos());
				}
				return;
			}

			if (committed)
			{
				auto* scene = owner->GetScene();
				if (scene)
				{
					auto& services = scene->GetServices();
					if (services.Has<SoundManager>())
					{
						services.Get<SoundManager>().SFX_Shot(L"Move_Player");
					}
				}


				m_CommitSucceeded = true;
				//if (auto* visualPreset = owner->GetComponent<PlayerVisualPresetComponent>())
				//{
				//	m_DebugVisualToggleFlip = !m_DebugVisualToggleFlip;
				//	visualPreset->ApplyByStateTag(m_DebugVisualToggleFlip ? "Melee" : "Throw");
				//}
				cout << "[Log] Player Moved" << endl;
				if (player->GetActorId() != 0)
				{
					const Events::ActorEvent actorEvent{ player->GetActorId() };
					GetEventDispatcher().Dispatch(EventType::PlayerMove, &actorEvent);
				}
				if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
				{
					playerFsm->DispatchEvent("Move_Complete");
				}
				DispatchEvent("Move_Complete");
				DispatchEvent("None");
			}
		});

	BindActionHandler("Move_ClearPending", [this](const FSMAction&)
		{
			ClearPendingTarget();
			m_LastValid = false;
		});

	// 취소 시 원복 (Cancel transition 또는 Idle enter에 연결)
	BindActionHandler("Move_Revert", [this](const FSMAction&)
		{
			auto* owner = GetOwner();
			if (!owner) return;

			auto* trans = owner->GetComponent<TransformComponent>();
			auto* moveComp = owner->GetComponent<PlayerMovementComponent>();
			if (!trans || !moveComp) return;

			trans->SetPosition(moveComp->GetDragStartPos());
		});

	BindActionHandler("Move_End", [this](const FSMAction&)
		{
			// 이동 시퀀스 종료 정리
			if (m_DraggingActive && !m_CommitSucceeded)
			{
				auto* owner = GetOwner();
				auto* trans = owner ? owner->GetComponent<TransformComponent>() : nullptr;
				auto* moveComp = owner ? owner->GetComponent<PlayerMovementComponent>() : nullptr;
				if (trans && moveComp)
				{
					trans->SetPosition(moveComp->GetDragStartPos());
				}
			}
			m_DraggingActive = false;
			m_LastValid = false;
			ClearPendingTarget();
		});

	BindActionHandler("Player_DispatchEvent", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			if (!owner)
			{
				return;
			}

			const std::string eventName = action.params.value("event", "");
			if (eventName.empty())
			{
				return;
			}

			if (auto* playerFsm = owner->GetComponent<PlayerFSMComponent>())
			{
				playerFsm->DispatchEvent(eventName);
			}
		});
}

void PlayerMoveFSMComponent::Start()
{
	FSMComponent::Start();
}

void PlayerMoveFSMComponent::Update(float deltaTime)
{
	// base FSM 업데이트(상태/전이 처리 등)
	FSMComponent::Update(deltaTime);

	// Selecting 중일 때만 프리뷰/판정 수행
	if (m_DraggingActive)
		UpdateDragPreview();
}

void PlayerMoveFSMComponent::UpdateDragPreview()
{
	auto* owner = GetOwner();
	if (!owner) return;

	auto* scene = owner->GetScene();
	if (!scene) return;

	auto* player = owner->GetComponent<PlayerComponent>();
	if (!player) return;

	// 턴 아니면 프리뷰 안 함(취소는 외부 이벤트로 들어오는게 정상)
	if (player->GetCurrentTurn() != Turn::PlayerTurn)
		return;

	auto* moveComp = owner->GetComponent<PlayerMovementComponent>();
	auto* trans = owner->GetComponent<TransformComponent>();
	if (!moveComp || !trans) return;

	if (!moveComp->HasDragRay())
		return;

	float hitT = 0.0f;
	NodeComponent* node = FindClosestNodeHit(scene, moveComp->GetDragRayOrigin(), moveComp->GetDragRayDir(), hitT);

	bool valid = false;

	if (node && node->GetIsMoveable() && node->IsInMoveRange())
	{
		const auto st = node->GetState();
		if (st == NodeState::Empty || st == NodeState::HasPlayer)
		{
			auto* nodeOwner = node->GetOwner();
			auto* nodeTr = nodeOwner ? nodeOwner->GetComponent<TransformComponent>() : nullptr;
			if (nodeTr)
			{
				auto p = nodeTr->GetPosition();
				auto off = moveComp->GetDragOffset();

				trans->SetPosition({ p.x + off.x, p.y + off.y, p.z + off.z });

				SetPendingTarget(node->GetQ(), node->GetR());
				valid = true;
			}
		}
	}

	if (!valid)
		ClearPendingTarget();

	// valid/invalid 변화가 있을 때만 이벤트 발행
	if (valid != m_LastValid)
	{
		m_LastValid = valid;
		DispatchEvent(valid ? "Move_PointValid" : "Move_PointInvalid");
	}
}
