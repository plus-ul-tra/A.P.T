#pragma once

#include "Service.h"

class TargetSenseService : public Service
{
public:
	void TickService(BTInstance& inst, Blackboard& bb, float deltaTime) override;
};

class CombatStateSyncService : public Service
{
public:
	void TickService(BTInstance& inst, Blackboard& bb, float deltaTime) override;
};

class RangeUpdateService : public Service
{
public:
	void TickService(BTInstance& inst, Blackboard& bb, float deltaTime) override;
};

class EstimatePlayerDamageService : public Service
{
public:
	void TickService(BTInstance& inst, Blackboard& bb, float deltaTime) override;
};

class RepathService : public Service
{
public:
	void TickService(BTInstance& inst, Blackboard& bb, float deltaTime) override;
};

class EventDispatcher;

class AIRequestDispatchService : public Service
{
public:
	explicit AIRequestDispatchService(EventDispatcher* dispatcher);

	void TickService(BTInstance& inst, Blackboard& bb, float deltaTime) override;

private:
	EventDispatcher* m_Dispatcher = nullptr;
};