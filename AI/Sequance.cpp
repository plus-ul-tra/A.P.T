#include "Sequance.h"
#include "BTInstance.h"

BTStatus Sequance::Tick(BTInstance& inst, Blackboard& bb)
{
	if (!PassDecorators(inst, bb))
		return BTStatus::Failure;

	TickServices(inst, bb);

	auto& memory = inst.GetCompositeMemory(GetId());

	for (uint32_t i = memory.index; i < m_Children.size(); ++i)
	{
		if (!m_Children[i])
			return BTStatus::Failure;

		BTStatus status = m_Children[i]->Tick(inst, bb);
		if (status == BTStatus::Running)
		{
			memory.index = i;
			return BTStatus::Running;
		}
		if (status == BTStatus::Failure)
		{
			memory.index = 0;
			return BTStatus::Failure;
		}
		// Success면 다음 child
	}

	memory.index = 0;
	return BTStatus::Success;
}

void Sequance::OnAbort(BTInstance& inst, Blackboard& bb)
{
	auto& memory = inst.GetCompositeMemory(GetId());
	if (memory.index < m_Children.size() && m_Children[memory.index])
		m_Children[memory.index]->OnAbort(inst, bb);

	memory.index = 0;
}
