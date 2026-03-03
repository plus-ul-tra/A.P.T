#include "CombatBehaviorTreeFactory.h"
#include "Selector.h"
#include "Sequance.h"
#include "BlackboardConditionTask.h"
#include "CombatBehaviorTasks.h"
#include "CombatBehaviorServices.h"
#include "BlackboardKeys.h"
#include "Decorator.h"
#include "EventDispatcher.h"

namespace CombatBehaviorTreeFactory
{

	std::shared_ptr<Node> BuildDefaultTree(EventDispatcher* dispatcher)
	{
		auto root = std::make_shared<Selector>();

		if (dispatcher)
		{
			auto dispatchService = std::make_unique<AIRequestDispatchService>(dispatcher);
			dispatchService->interval = 0.0f;
			root->AddService(std::move(dispatchService));
		}

		auto buildSenseUpdateTask = [dispatcher]()
			{
				auto updateTask = std::make_shared<UpdateTargetLocationTask>();
				auto targetSense = std::make_unique<TargetSenseService>();
				targetSense->interval = 0.0f;
				updateTask->AddService(std::move(targetSense));

				auto combatSync = std::make_unique<CombatStateSyncService>();
				combatSync->interval = 0.0f;
				updateTask->AddService(std::move(combatSync));

				auto rangeUpdate = std::make_unique<RangeUpdateService>();
				rangeUpdate->interval = 0.0f;
				updateTask->AddService(std::move(rangeUpdate));

				auto estimateDamage = std::make_unique<EstimatePlayerDamageService>();
				estimateDamage->interval = 0.0f;
				updateTask->AddService(std::move(estimateDamage));

				auto repath = std::make_unique<RepathService>();
				repath->interval = 0.0f;
				updateTask->AddService(std::move(repath));
			
				return updateTask;
			};

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 죽었으면 행동 안 함(턴만 종료) - 시스템이 죽은 AI를 아예 스킵한다면 이 블록은 제거해도 됨
		{
			auto deadSeq = std::make_shared<Sequance>();
			deadSeq->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::IsAlive, false));
			deadSeq->AddChild(std::make_shared<EndTurnTask>());
			root->AddChild(deadSeq);
		}

		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 전투 여부 분기
		{
			auto combatSequence = std::make_shared<Sequance>();
			combatSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::IsAlive, true));
			combatSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::IsInCombat, true));
			combatSequence->AddChild(buildSenseUpdateTask());
			

			auto combatSelector = std::make_shared<Selector>();

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 도주
			{
				auto runOffSequence = std::make_shared<Sequance>();
				runOffSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::ShouldRunOff, true));
				runOffSequence->AddChild(std::make_shared<SelectRunOffTargetTask>());
				runOffSequence->AddChild(std::make_shared<RunOffMoveTask>());
				runOffSequence->AddChild(std::make_shared<EndTurnTask>());
				combatSelector->AddChild(runOffSequence);
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 원거리 공격: 사거리/시야/공격가능이면 공격
			{
				auto rangedAttackSequence = std::make_shared<Sequance>();
				rangedAttackSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::PreferRanged, true));
				rangedAttackSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::HasTarget, true));
				rangedAttackSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::InThrowRange, true));
				rangedAttackSequence->AddChild(std::make_shared<RangedAttackTask>());
				rangedAttackSequence->AddChild(std::make_shared<EndTurnTask>());
				combatSelector->AddChild(rangedAttackSequence);

				// 원거리 선호지만 사거리 안 맞으면 거리 유지/재포지션
				auto rangedMoveSequence = std::make_shared<Sequance>();
				rangedMoveSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::PreferRanged, true));
				rangedMoveSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::InThrowRange, false));
				//rangedMoveSequence->AddChild(std::make_shared<MaintainRangeTask>());
				rangedMoveSequence->AddChild(std::make_shared<ApproachRangedTargetTask>());
				rangedMoveSequence->AddChild(std::make_shared<EndTurnTask>());
				combatSelector->AddChild(rangedMoveSequence);
			}

			/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 근접 공격: 붙어있고 공격가능이면 공격
			{
				auto meleeAttackSequence = std::make_shared<Sequance>();
				meleeAttackSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::HasTarget, true));
				meleeAttackSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::InMeleeRange, true));
				meleeAttackSequence->AddChild(std::make_shared<MeleeAttackTask>());
				meleeAttackSequence->AddChild(std::make_shared<EndTurnTask>());
				combatSelector->AddChild(meleeAttackSequence);

				// 근접 사거리 아니면 접근
				auto approachSequence = std::make_shared<Sequance>();
				approachSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::InMeleeRange, false));
				approachSequence->AddChild(std::make_shared<ApproachTargetTask>());
				approachSequence->AddChild(std::make_shared<EndTurnTask>());
				combatSelector->AddChild(approachSequence);
			}

			////////////////////////////////////////////////////////////////////////////////////////////////////////////
			// 전투 Fallback: 위에서 아무것도 못 하면 최소 행동(정찰/대기/턴 종료)
			// - 여기서 "타겟 클리어" 같은 걸 하고 싶으면 ClearTargetTask 같은 걸 추가하면 됨
			{
				auto fallbackSeq = std::make_shared<Sequance>();
				fallbackSeq->AddChild(buildSenseUpdateTask());
				fallbackSeq->AddChild(std::make_shared<PatrolMoveTask>()); // 대기/재포지션 대용
				fallbackSeq->AddChild(std::make_shared<EndTurnTask>());
				combatSelector->AddChild(fallbackSeq);
			}

			combatSequence->AddChild(combatSelector);
			root->AddChild(combatSequence);
		}
				
		/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 비전투 탐색 (정찰/이동/대기 역할): InCombat == false (정찰/이동/대기)
		{
			auto nonCombatSequence = std::make_shared<Sequance>();
			nonCombatSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::IsAlive, true));
			nonCombatSequence->AddChild(std::make_shared<BlackboardConditionTask>(BlackboardKeys::IsInCombat, false));
			nonCombatSequence->AddChild(buildSenseUpdateTask());
			nonCombatSequence->AddChild(std::make_shared<PatrolMoveTask>());
			nonCombatSequence->AddChild(std::make_shared<EndTurnTask>());
			root->AddChild(nonCombatSequence);
		}

		////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// 최후 Fallback: 어떤 이유로도 못 들어가면 턴 종료(소프트락 방지)
		{
			auto ultimateFallback = std::make_shared<Sequance>();
			ultimateFallback->AddChild(std::make_shared<EndTurnTask>());
			root->AddChild(ultimateFallback);
		}

		return root;
	}
}
