#pragma once
#include "Composite.h"

class Selector : public Composite
{
public:
	BTStatus Tick(BTInstance& inst, Blackboard& bb) override;
	void     OnAbort(BTInstance& inst, Blackboard& bb) override;

	const std::vector<std::shared_ptr<Node>>& GetChildren() const { return m_Children; }
};

