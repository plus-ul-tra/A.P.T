#pragma once

#include "BTExecutor.h"
#include "BTInstance.h"
#include "Blackboard.h"

class AIController
{
public:
	explicit AIController(BTExecutor& executor);

	void Initialize();
	void Tick(float deltaTime);

	Blackboard&		  GetBlackboard()		{ return m_Blackboard; }
	const Blackboard& GetBlackboard() const { return m_Blackboard; }
	BTInstance&		  GetInstance  ()		{ return m_Instance;   }
	const BTInstance& GetInstance  () const { return m_Instance;   }

private:
	BTExecutor& m_Executor;
	BTInstance  m_Instance;
	Blackboard  m_Blackboard;
	bool		m_IsInitialized = false;
};

