#pragma once

#include "Task.h"

class UpdateTargetLocationTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class MoveToTargetTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class ApproachTargetTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class ApproachRangedTargetTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class MeleeAttackTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class RangedAttackTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class SelectRunOffTargetTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class RunOffMoveTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class MaintainRangeTask : public Task
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class PatrolMoveTask : public Task 
{
protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;
};

class EndTurnTask : public Task
{
public:
	explicit EndTurnTask(float delaySeconds = 1) : m_DelaySeconds(delaySeconds) {}
protected:
	void OnEnter(BTInstance& inst, Blackboard& bb) override;
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;

private:
	float m_DelaySeconds = 1;
};

