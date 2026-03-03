#pragma once
#include "Node.h"
#include "BTStatus.h"

class BTInstance;
class Blackboard;

class Task : public Node
{
public:
	BTStatus Tick(BTInstance& inst, Blackboard& bb) final;
	void     OnAbort(BTInstance& inst, Blackboard& bb) final;

protected:
	virtual void     OnEnter(BTInstance& inst, Blackboard& bb){ (void)inst; (void)bb; }
	virtual BTStatus OnTick(BTInstance& inst, Blackboard& bb) = 0;
	virtual void     OnExit(BTInstance& inst, Blackboard& bb, BTStatus status){ (void)inst; (void)bb; (void)status; }
	virtual void     OnAbortTask(BTInstance& inst, Blackboard& bb){ (void)inst; (void)bb; }
};

