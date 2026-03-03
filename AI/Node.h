#pragma once
#include <vector>
#include <memory>
#include "BTStatus.h"

class BTInstance;
class Blackboard;
class Decorator;
class Service;
	
class Node
{
public:
	Node();
	virtual ~Node();

	void     SetId(uint32_t id) { m_Id = id;   }
	uint32_t GetId() const  { return m_Id; }

	// 실행 진입점
	virtual BTStatus Tick(BTInstance& inst, Blackboard& bb) = 0;

	// Abort는 tick-start에 호출될 수 있음
	virtual void     OnAbort(BTInstance& inst, Blackboard& bb);

	// 노드 서비스만 별도 실행 (후처리용)
	void TickServicesOnly(BTInstance& inst, Blackboard& bb);

	void AddDecorator(std::unique_ptr<Decorator> decorator);
	void AddService  (std::unique_ptr<Service> service);

protected:
	// Attachment
	std::vector<std::unique_ptr<Decorator>> m_Decorators;
	std::vector<std::unique_ptr<Service>>   m_Services;

	// Decorator gating + Observer 반영
	bool PassDecorators(BTInstance& inst, Blackboard& bb) const;

	// Services tick (active branch 기준으로 호출. 현재는 간단히 노드 Tick 시점에 호출)
	void TickServices(BTInstance& inst, Blackboard& bb);

private:
	uint32_t m_Id = 0;
};

