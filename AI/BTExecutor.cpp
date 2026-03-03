#include "BTExecutor.h"
#include "Node.h"
#include "BTInstance.h"
#include "Blackboard.h"
#include "Selector.h"
#include "Sequance.h"
#include "Parallel.h"

static void CollectChildren(const std::shared_ptr<Node>& node, std::vector<std::shared_ptr<Node>>& out)
{
	if (!node) return;

	if (auto* selector = dynamic_cast<Selector*>(node.get()))
	{
		for (auto& child : selector->GetChildren()) out.push_back(child);
		return;
	}

	if (auto* sequance = dynamic_cast<Sequance*>(node.get()))
	{
		for (auto& child : sequance->GetChildren()) out.push_back(child);
		return;
	}

	if (auto* parallel = dynamic_cast<Parallel*>(node.get()))
	{
		for (auto& child : parallel->GetChildren()) out.push_back(child);
		return;
	}
}

uint32_t BTExecutor::AssignIdsDFS(const std::shared_ptr<Node>& node, uint32_t nextId)
{
	if (!node)
		return nextId;

	node->SetId(nextId++);

	std::vector<std::shared_ptr<Node>> children;
	CollectChildren(node, children);

	for (auto& child : children)
	{
		nextId = AssignIdsDFS(child, nextId);
	}

	return nextId;
}


void BTExecutor::Build(BTInstance& inst)
{
	m_NodeCount = AssignIdsDFS(m_Root, 0);
	if (m_NodeCount == 0) m_NodeCount = 1;
	inst.Init(m_NodeCount);
}

BTStatus BTExecutor::Tick(BTInstance& inst, Blackboard& bb, float dt)
{
	if (!m_Root)
		return BTStatus::Failure;

	inst.SetDeltaTime(dt);

	// tick-start abort 정책
	if (inst.HasAbortRequest())
	{
		// 현재 구현: root부터 abort 전파
		m_Root->OnAbort(inst, bb);
		inst.ClearAbortRequest();
	}

	const BTStatus status = m_Root->Tick(inst, bb);
	m_Root->TickServicesOnly(inst, bb);
	return status;
}