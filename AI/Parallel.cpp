#include "Parallel.h"
#include "BTInstance.h"
#include "Blackboard.h"

static void AbortChild(const std::shared_ptr<Node>& node, BTInstance& inst, Blackboard& bb)
{
	if (node)
		node->OnAbort(inst, bb);
}

BTStatus Parallel::Tick(BTInstance& inst, Blackboard& bb)
{
	if (!PassDecorators(inst, bb))
		return BTStatus::Failure;

	TickServices(inst, bb);

	if (m_Children.size() < 2 || !m_Children[0] || !m_Children[1])
		return BTStatus::Failure;

	auto& memory = inst.GetParallelMemory(GetId());
	Node& mainNode = *m_Children[0];
	Node& bgNode = *m_Children[1];

	// main
	if (!memory.mainDone)
	{
		BTStatus st = mainNode.Tick(inst, bb);
		memory.mainStatus = st;
		if (st != BTStatus::Running)
		{
			memory.mainDone = true;

			if (m_FinishMode == ParallelFinishMode::Immediate)
			{
				AbortChild(m_Children[1], inst, bb);
				memory.backgroundDone = true;
				memory.backgroundStatus = BTStatus::Failure; // Abort로 종료 처리 (정책 선택)
			}
		}
	}

	// background
	if (!memory.backgroundDone)
	{
		BTStatus st = bgNode.Tick(inst, bb);
		memory.backgroundStatus = st;
		if (st != BTStatus::Running)
			memory.backgroundDone = true;
	}

	BTStatus out = Resolve(memory);

	// Parallel이 종료되면 메모리 리셋 (다음 재진입 대비)
	if (out != BTStatus::Running)
		memory = ParallelMemory{};

	return out;
}

BTStatus Parallel::Resolve(const ParallelMemory& memory) const
{
	switch (m_ResultPolicy)
	{
	case ParallelResultPolicy::MainOnly:
		return memory.mainDone ? memory.mainStatus : BTStatus::Running;

	case ParallelResultPolicy::BackgroundOnly:
		return memory.backgroundDone ? memory.backgroundStatus : BTStatus::Running;

	case ParallelResultPolicy::BothMustSucceed:
		if (!memory.mainDone || !memory.backgroundDone) return BTStatus::Running;
		return (memory.mainStatus == BTStatus::Success && memory.backgroundStatus == BTStatus::Success)
			? BTStatus::Success : BTStatus::Failure;

	case ParallelResultPolicy::AnySucceed:
		if ((memory.mainDone && memory.mainStatus == BTStatus::Success) ||
			(memory.backgroundDone && memory.backgroundStatus == BTStatus::Success))
			return BTStatus::Success;
		if (memory.mainDone && memory.backgroundDone)
			return BTStatus::Failure;
		return BTStatus::Running;

	default:
		return BTStatus::Failure;
	}
}


void Parallel::OnAbort(BTInstance& inst, Blackboard& bb)
{
	// 정책 : Abort는 active path 하위에 전파되어야 함
	// 여기서는 Parallel 자체 Abort면 자식 둘 다 Abort 처리
	if (m_Children.size() >= 1) AbortChild(m_Children[0], inst, bb);
	if (m_Children.size() >= 2) AbortChild(m_Children[1], inst, bb);

	// 메모리 리셋
	auto& memory = inst.GetParallelMemory(GetId());
	memory = ParallelMemory{};
}

