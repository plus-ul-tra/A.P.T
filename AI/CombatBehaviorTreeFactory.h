#pragma once

#include <memory>

class Node;
class EventDispatcher;

namespace CombatBehaviorTreeFactory
{
	std::shared_ptr<Node> BuildDefaultTree(EventDispatcher* dispatcher);
}