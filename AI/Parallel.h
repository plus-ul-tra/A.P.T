#pragma once
#include <memory>
#include <vector>
#include "Node.h"

enum class ParallelFinishMode
{
	Immediate,
	Delayed
};

// 결과 정책
enum class ParallelResultPolicy
{	
	MainOnly,			// 메인 결과가 곧 Parallel 결과
	BackgroundOnly,		// 백그라운드 결과가 곧 Parallel 결과
	BothMustSucceed,	// 둘다 Success여야 Success
	AnySucceed			// 하나라도 Success면 Success
};

class Parallel : public Node
{
public:
	void AddChild(std::shared_ptr<Node> child)
	{
		m_Children.emplace_back(std::move(child));
	}

	// 규칙 : child[0] = main, child[1] = background로 사용
	void SetFinishMode(ParallelFinishMode mode) { m_FinishMode = mode; }
	void SetResultPolicy(ParallelResultPolicy policy) { m_ResultPolicy = policy; }

	BTStatus Tick(BTInstance& inst, Blackboard& bb) override;
	void     OnAbort(BTInstance& inst, Blackboard& bb) override;

	const std::vector<std::shared_ptr<Node>>& GetChildren() const { return m_Children; }

private:
	BTStatus Resolve(const struct ParallelMemory& memory) const;

private:
	std::vector<std::shared_ptr<Node>> m_Children;

	ParallelFinishMode   m_FinishMode   = ParallelFinishMode::Immediate;
	ParallelResultPolicy m_ResultPolicy = ParallelResultPolicy::MainOnly;
};

