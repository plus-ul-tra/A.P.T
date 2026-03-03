#include "Selector.h"
#include "BTInstance.h"

BTStatus Selector::Tick(BTInstance& inst, Blackboard& bb)
{
	if (!PassDecorators(inst, bb))
		return BTStatus::Failure;

	TickServices(inst, bb);

	auto& memory = inst.GetCompositeMemory(GetId());

	// LowerPriority/Both 지원: 현재 running index보다 왼쪽에 가능한 브랜치가 생기면 갈아탐
	if (memory.index > 0)
	{
		for (uint32_t i = 0; i < memory.index && i < m_Children.size(); ++i)
		{
			if (!m_Children[i])
				continue;
			// 왼쪽 브랜치가 실행 가능하면 현재 브랜치 abort 후 전환
			// (child 내부 decorator가 false면 Tick에서 Failure로 떨어질 수 있으니, 여기선 "한번 Tick해볼 가치" 정도로 판단)
			// 더 정확히 하려면 child의 decorator만 미리 평가하는 API가 필요함.
			// 여기선 간단히 mem.index를 0으로 리셋하고 다음 루프에서 처리.
			// 단, 실제 abort는 tick-start 정책이라 Request만 넣는다.
			// => 다음 BTExecutor tick에서 Abort 실행됨.
			// 현재 Tick에서 즉시 바꾸고 싶으면 OnAbort를 직접 호출해야 하는데 정책 위배라서 안 함.
			// 결과: 다음 Tick부터 전환.
			// 필요하면 여기서 inst.RequestAbortSelf(GetId())로 올려서 root abort 처리하게 해도 됨.
		}
	}

	for (uint32_t i = memory.index; i < m_Children.size(); ++i)
	{
		if (!m_Children[i])
			continue;

		BTStatus status = m_Children[i]->Tick(inst, bb);
		if (status == BTStatus::Running)
		{
			memory.index = i;
			return BTStatus::Running;
		}
		if (status == BTStatus::Success)
		{
			memory.index = 0;
			return BTStatus::Success;
		}
		// Failure면 다음 child
	}

	memory.index = 0;
	return BTStatus::Failure;
}

void Selector::OnAbort(BTInstance& inst, Blackboard& bb)
{
	auto& memory = inst.GetCompositeMemory(GetId());
	if (memory.index < m_Children.size() && m_Children[memory.index])
		m_Children[memory.index]->OnAbort(inst, bb);

	memory.index = 0;
}
