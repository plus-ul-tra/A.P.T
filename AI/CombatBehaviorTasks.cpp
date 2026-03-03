#include "CombatBehaviorTasks.h"
#include "Blackboard.h"
#include "BlackboardKeys.h"
#include "BTInstance.h"
#include <iostream>

bool GetBool(Blackboard& bb, const char* key, bool defaultValue = false)
{
    bool value = defaultValue;
    bb.TryGet(key, value);
    return value;
}

BTStatus UpdateTargetLocationTask::OnTick(BTInstance& inst, Blackboard& bb)
{
    (void)inst;
	int targetQ = 0;
	int targetR = 0;
	/*if (!bb.TryGet(BlackboardKeys::TargetQ, targetQ)
		|| !bb.TryGet(BlackboardKeys::TargetR, targetR))*/
	bool hasTarget = bb.TryGet(BlackboardKeys::TargetQ, targetQ)
	&& bb.TryGet(BlackboardKeys::TargetR, targetR);
	if (!hasTarget)
	{
		hasTarget = bb.TryGet(BlackboardKeys::LastKnownTargetQ, targetQ)
			&& bb.TryGet(BlackboardKeys::LastKnownTargetR, targetR);
		if (hasTarget)
		{
			bb.Set(BlackboardKeys::TargetQ, targetQ);
			bb.Set(BlackboardKeys::TargetR, targetR);
		}
	}

	if (!hasTarget)
	{
		bool isInCombat = false;
		bb.TryGet(BlackboardKeys::IsInCombat, isInCombat);
		return isInCombat ? BTStatus::Success : BTStatus::Failure;
	}

	bb.Set(BlackboardKeys::LastKnownTargetQ, targetQ);
	bb.Set(BlackboardKeys::LastKnownTargetR, targetR);
    return BTStatus::Success;
}

BTStatus MoveToTargetTask::OnTick(BTInstance& inst, Blackboard& bb)
{
    (void)inst;
    if (!GetBool(bb, BlackboardKeys::HasTarget))
    {
        return BTStatus::Failure;
    }

    bb.Set(BlackboardKeys::MoveRequested, true);
    return BTStatus::Success;
}

BTStatus ApproachTargetTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	float distance = 0.0f;
	float meleeRange = 1.0f;
	bb.TryGet(BlackboardKeys::TargetDistance, distance);
	bb.TryGet(BlackboardKeys::MeleeRange, meleeRange);

	if (distance <= meleeRange)
	{
		return BTStatus::Success;
	}

	bb.Set(BlackboardKeys::MoveRequested, true);
	return BTStatus::Success;
}

BTStatus MeleeAttackTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	if (!GetBool(bb, BlackboardKeys::InMeleeRange))
	{
		return BTStatus::Failure;
	}

	bb.Set(BlackboardKeys::RequestMeleeAttack, true);
	return BTStatus::Success;
}

BTStatus RangedAttackTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	if (!GetBool(bb, BlackboardKeys::InThrowRange))
	{
		return BTStatus::Failure;
	}

	bb.Set(BlackboardKeys::RequestRangedAttack, true);
	return BTStatus::Success;
}

BTStatus SelectRunOffTargetTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	bool hasTarget = GetBool(bb, BlackboardKeys::HasTarget);
	bb.Set(BlackboardKeys::RunOffTargetFound, hasTarget);
	return hasTarget ? BTStatus::Success : BTStatus::Failure;
}

BTStatus RunOffMoveTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	if (!GetBool(bb, BlackboardKeys::RunOffTargetFound))
	{
		return BTStatus::Failure;
	}
	bb.Set(BlackboardKeys::RequestRunOffMove, true);
	return BTStatus::Success;
}

BTStatus MaintainRangeTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	if (!GetBool(bb, BlackboardKeys::MaintainRange))
	{
		return BTStatus::Failure;
	}
	bb.Set(BlackboardKeys::RequestMaintainRange, true);
	return BTStatus::Success;
}

BTStatus PatrolMoveTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	(void)inst;
	bb.Set(BlackboardKeys::MoveRequested, true);
	return BTStatus::Success;
}


void EndTurnTask::OnEnter(BTInstance& inst, Blackboard& bb)
{
	(void)bb;
	auto& memory = inst.GetTaskMemory(GetId());
	memory.elapsed = 0.0f;
}

BTStatus EndTurnTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	auto& memory = inst.GetTaskMemory(GetId());
	memory.elapsed += inst.GetDeltaTime();
	float delaySeconds = m_DelaySeconds;
	bb.TryGet(BlackboardKeys::EndTurnDelay, delaySeconds);
	if (memory.elapsed < delaySeconds)
	{
		return BTStatus::Running;
	}
	bb.Set(BlackboardKeys::EndTurnRequested, true);
	return BTStatus::Success;
}

BTStatus ApproachRangedTargetTask::OnTick(BTInstance& inst, Blackboard& bb)
{
	float distance = 0.0f;
	float throwRange = 1.0f;
	bb.TryGet(BlackboardKeys::TargetDistance, distance);
	bb.TryGet(BlackboardKeys::ThrowRange, throwRange);

	if (distance <= throwRange)
	{
		return BTStatus::Success;
	}

	bb.Set(BlackboardKeys::MoveRequested, true);
	return BTStatus::Success;
}
