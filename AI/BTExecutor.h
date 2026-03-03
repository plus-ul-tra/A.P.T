#pragma once
#include <memory>
#include "BTStatus.h"

class Node;
class BTInstance;
class Blackboard;

// tick-start abort 정책을 가진 실행기
class BTExecutor
{
public:
	void SetRoot(std::shared_ptr<Node> root)
	{
		m_Root = root;
	}

	// 트리 id 부여 + 인스턴스 메모리 초기화
	void Build(BTInstance& inst);

	// 1프레임 실행
	// - dt는 BTInstance에 저장되어 Service에서 사용
	BTStatus Tick(BTInstance&, Blackboard&, float);

private:
	uint32_t AssignIdsDFS(const std::shared_ptr<Node>& node, uint32_t nextId);

private:
	std::shared_ptr<Node> m_Root;
	uint32_t m_NodeCount = 0;
};

