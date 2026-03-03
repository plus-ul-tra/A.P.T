#include "Task.h"
#include "BTInstance.h"

BTStatus Task::Tick(BTInstance& inst, Blackboard& bb)
{
	if (!PassDecorators(inst, bb))
		return BTStatus::Failure;

	TickServices(inst, bb);

	auto& memory = inst.GetTaskMemory(GetId());
	if (!memory.entered)
	{
		memory.entered = true;
		OnEnter(inst, bb);
	}

	BTStatus status = OnTick(inst, bb);

	if (status == BTStatus::Success || status == BTStatus::Failure)
	{
		OnExit(inst, bb, status);
		memory.entered = false;
	}

	return status;
}

void Task::OnAbort(BTInstance& inst, Blackboard& bb)
{
	auto& mem = inst.GetTaskMemory(GetId());

	if (mem.entered)
	{
		OnAbortTask(inst, bb);
		mem.entered = false;
	}
}