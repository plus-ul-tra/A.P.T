#pragma once
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <string>
#include "BTStatus.h"

class Decorator;
class Service;
class Node;

// Task 런타임 메모리
struct TaskMemory
{
	bool entered = false;
	float elapsed = 0.0f;
};

// Selector / Sequance 런타임 메모리
struct CompositeMemory
{
	uint32_t index = 0;
};

// Parallel 런타임 메모리
struct ParallelMemory
{
	bool mainDone			  = false;
	bool backgroundDone		  = false;

	BTStatus mainStatus		  = BTStatus::Running;
	BTStatus backgroundStatus = BTStatus::Running;
};

// Decorator 옵저버 상태 (키 버전 기반)
struct DecoratorObserveState
{
	bool initialized = false;
	bool dirty		 = true; // 최초 1회 평가 필요
	std::unordered_map<std::string, uint32_t> lastSeenVersion;
};

// Service 스케줄 상태
struct ServiceTickState
{
	float accumulator = 0.0f;
};

class BTInstance
{
public:
	void Init(uint32_t nodeCount)
	{
		m_TaskMemory.assign(nodeCount, TaskMemory{});
		m_CompositeMemory.assign(nodeCount, CompositeMemory{});
		m_ParallelMemory.assign(nodeCount, ParallelMemory{});
		m_DeltaTime = 0.0f;
	}

	void  SetDeltaTime(float dt) { m_DeltaTime = dt;   }
	float GetDeltaTime() const   { return m_DeltaTime; }

	TaskMemory& GetTaskMemory(uint32_t id)
	{
		return m_TaskMemory[id];
	}

	CompositeMemory& GetCompositeMemory(uint32_t id)
	{
		return m_CompositeMemory[id];
	}


	ParallelMemory& GetParallelMemory(uint32_t id)
	{
		return m_ParallelMemory[id];
	}

	// Decorator 관측 상태: decorator 포인터 키로 관리
	DecoratorObserveState& GetObserveState(const Decorator* d) { return m_DecObserveStates[d]; }

	// Service tick 상태: service 포인터 키로 관리
	ServiceTickState&      GetServiceState(const Service* s) { return m_ServiceStates[s]; }

	// Abort 요청 (tick-start에 처리)
	void RequestAbortSelf(uint32_t fromNodeId)
	{
		m_AbortRequested = true;
		m_AbortFromNodeId = fromNodeId;
	}

	bool     HasAbortRequest   () const { return m_AbortRequested; }
	uint32_t GetAbortFromNodeId() const { return m_AbortFromNodeId; }
	void     ClearAbortRequest ()
	{
		m_AbortRequested = false;
		m_AbortFromNodeId = UINT32_MAX;
	}

private:
	std::vector<TaskMemory>      m_TaskMemory;
	std::vector<CompositeMemory> m_CompositeMemory;
	std::vector<ParallelMemory>  m_ParallelMemory;
	std::unordered_map<const Decorator*, DecoratorObserveState> m_DecObserveStates;
	std::unordered_map<const Service*, ServiceTickState>        m_ServiceStates;

	bool m_AbortRequested = false;
	uint32_t m_AbortFromNodeId = UINT32_MAX;

	float m_DeltaTime = 0.0f;
};

