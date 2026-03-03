#pragma once

class BTInstance;
class Blackboard;

class Service
{
public:
	virtual ~Service() = default;
	virtual void TickService(BTInstance& inst, Blackboard& bb, float deltaTime) = 0;

	float interval = 0.2f;
};

