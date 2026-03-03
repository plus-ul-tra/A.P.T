#pragma once

#include "Task.h"
#include <string>

class BlackboardConditionTask : public Task
{
public:
	BlackboardConditionTask(std::string key, bool expected);

protected:
	BTStatus OnTick(BTInstance& inst, Blackboard& bb) override;

private:
	std::string m_Key;
	bool		m_Expected = false;
};

