#include "PlayerPushFSMComponent.h"
#include "PlayerComponent.h"
#include "PlayerStatComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "scene.h"
#include "ServiceRegistry.h"
#include "DiceSystem.h"
#include "Event.h"

REGISTER_COMPONENT_DERIVED(PlayerPushFSMComponent, FSMComponent)

namespace
{
	constexpr int PushCost = 1;
	constexpr int PushRollThreshold = 5; //성공 값
}

PlayerPushFSMComponent::PlayerPushFSMComponent()
{
	BindActionHandler("Push_ConsumeActResource", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			if (!player)
			{
				DispatchEvent("Push_Revoke");
				return;
			}

			const bool consumed = player->ConsumeActResource(PushCost);
			if (!consumed)
			{
				DispatchEvent("Push_Revoke");
			}
		});

	BindActionHandler("Push_CheckPossible", [this](const FSMAction& action)
		{
			// 노드 간 조건 달성 되었을 때
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool canPush = player ? player->ConsumePushPossible() : false;
			DispatchEvent(canPush ? "Push_Possible" : "Push_Revoke");
		});

	BindActionHandler("Push_SearchTarget", [this](const FSMAction& action)
		{
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			const bool hasTarget = player ? player->ConsumePushTargetFound() : false;
			DispatchEvent(hasTarget ? "Push_TargetFound" : "Push_TargetNone");
		});

	BindActionHandler("Push_SelectTarget", [this](const FSMAction&)
		{
			DispatchEvent("Push_TargetSelected");
		});

	BindActionHandler("Push_Resolve", [this](const FSMAction& action)
		{
			// 주사위
			auto* owner = GetOwner();
			auto* player = owner ? owner->GetComponent<PlayerComponent>() : nullptr;
			auto* playerStat = owner ? owner->GetComponent<PlayerStatComponent>() : nullptr;

			if (player && playerStat)
			{
				auto* scene = owner ? owner->GetScene() : nullptr;
				if (scene)
				{
					auto& services = scene->GetServices();
					if (services.Has<DiceSystem>())
					{
						auto& diceSystem = services.Get<DiceSystem>();
						int bonus = playerStat->GetCalculatedStrengthModifier();
						const DiceConfig rollConfig{ 1, 20, 0 };

						const Events::DiceRollEvent difficultyEvent{ PushRollThreshold, 1, 20, 0, "PushDifficulty" };
						GetEventDispatcher().Dispatch(EventType::DiceRolled, &difficultyEvent);

						const auto roll = diceSystem.Roll(rollConfig, RandomDomain::World);
						const Events::DiceRollEvent rollEvent{ roll.total, rollConfig.count, rollConfig.sides, rollConfig.bonus, "PushRoll", false, roll.faces };
						GetEventDispatcher().Dispatch(EventType::DiceRolled, &rollEvent);

						const Events::DiceRollEvent totalEvent{ roll.total, rollConfig.count, rollConfig.sides, rollConfig.bonus, "PushRollTotal", true };
						GetEventDispatcher().Dispatch(EventType::DiceRolled, &totalEvent);

						player->SetPushSuccess(roll.total >= PushRollThreshold); // 성공 시 SetPushSucces
					}
				}
			}
			const bool success = player ? player->ConsumePushSuccess() : false;
			DispatchEvent(success ? "Push_Success" : "Push_Fail");
		});
}


void PlayerPushFSMComponent::Start()
{
	FSMComponent::Start();
}

void PlayerPushFSMComponent::Update(float deltaTime)
{
	FSMComponent::Update(deltaTime);

}
