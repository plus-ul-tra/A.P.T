#include "Node.h"
#include "BTInstance.h"
#include "Blackboard.h"
#include "Decorator.h"
#include "Service.h"
#include <utility>

void Node::AddDecorator(std::unique_ptr<Decorator> decorator)
{
	m_Decorators.emplace_back(std::move(decorator));
}

void Node::AddService(std::unique_ptr<Service> service)
{
	m_Services.emplace_back(std::move(service));

}

bool Node::PassDecorators(BTInstance& inst, Blackboard& bb) const
{
	for (const auto& decorator : m_Decorators)
	{
		auto& observeState = inst.GetObserveState(decorator.get());

		// observedKeys 초기화 : lastSeenVersion 세팅
		if (!observeState.initialized)
		{
			for (const auto& key : decorator->GetObservedKeys())
			{
				observeState.lastSeenVersion[key] = bb.GetVersion(key);
			}
			observeState.initialized = true;
			observeState.dirty = true;
		}
		else
		{
			// 키 버전이 변하면 Dirty
			for (const auto& key : decorator->GetObservedKeys())
			{
				uint32_t cur = bb.GetVersion(key);
				auto it = observeState.lastSeenVersion.find(key);
				uint32_t last = (it == observeState.lastSeenVersion.end()) ? 0u : it->second;
				if (cur != last)
				{
					observeState.dirty = true;
					observeState.lastSeenVersion[key] = cur;
				}
			}
		}

		// dirty면 재평가
		if (observeState.dirty)
		{
			observeState.dirty = false;
			bool ok = decorator->Evaluate(inst, bb);

			if (!ok)
			{
				// abort는 여기서 즉시 실행하지 않고 요청만 (tick-start 처리 정책)
				AbortMode mode = decorator->GetAbortMode();

				if (mode == AbortMode::Self || mode == AbortMode::Both)
				{
					inst.RequestAbortSelf(GetId());
				}

				return false;
			}
		}
		else
		{
			// dirty가 아니면 Evaluate 스킵. 안전하게 다시 평가하고 싶으면 여기서 Evaluate 해도 됨.
		    // 현재 구현은 "키 변화 없으면 조건 변화 없음" 전제.
		    // observedKeys 없는 데코레이터면 항상 dirty로 두는 게 맞음.
			if (decorator->GetObservedKeys().empty())
			{
				bool ok = decorator->Evaluate(inst, bb);
				if (!ok)
				{
					AbortMode mode = decorator->GetAbortMode();
					if (mode == AbortMode::Self || mode == AbortMode::Both)
						inst.RequestAbortSelf(GetId());
					return false;
				}
			}
		}
	}

	return true;
}

void Node::TickServices(BTInstance& inst, Blackboard& bb)
{
	float dt = inst.GetDeltaTime();

	for (const auto& service : m_Services)
	{
		auto& serviceState = inst.GetServiceState(service.get());
		serviceState.accumulator += dt;
		if (serviceState.accumulator >= service->interval)
		{
			serviceState.accumulator = 0.0f;
			service->TickService(inst, bb, dt);
		}
	}
}

void Node::TickServicesOnly(BTInstance& inst, Blackboard& bb)
{
	TickServices(inst, bb);
}

Node::Node() = default;
Node::~Node() = default;

void Node::OnAbort(BTInstance& inst, Blackboard& bb)
{
	// 기본은 attachment만 리셋할 게 없음. 파생(composite/task)이 실제 abort 처리
	(void)inst;
	(void)bb;
}
