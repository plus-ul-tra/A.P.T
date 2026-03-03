#include "EventDispatcher.h"
#include <algorithm>

bool IsMouseEvent(EventType type)
{
	switch (type)
	{
	case EventType::MouseLeftClick:
	case EventType::MouseLeftClickHold:
	case EventType::MouseLeftClickUp:
	case EventType::MouseLeftDoubleClick:
	case EventType::MouseRightClick:
	case EventType::MouseRightClickHold:
	case EventType::MouseRightClickUp:
	case EventType::Dragged:
	case EventType::Hovered:
	case EventType::MouseWheelUp:
	case EventType::MouseWheelDown:
	case EventType::Pressed:
	case EventType::Released:
	case EventType::Moved:
	case EventType::UIDragged:
	case EventType::UIHovered:
	case EventType::UIDoubleClicked:
		return true;
	default:
		return false;
	}
}

void EventDispatcher::AddListener(EventType type, IEventListener* listener)
{
	if (!m_IsAlive || !listener) return;

	auto& vec = m_Listeners[type]; // 여기선 생성 OK(등록이니까)
	if (std::find(vec.begin(), vec.end(), listener) != vec.end())
		return;

	vec.push_back(listener);
}

void EventDispatcher::RemoveListener(EventType type, IEventListener* listener)
{
	if (!m_IsAlive || !listener) return;

	auto it = m_Listeners.find(type);
	if (it == m_Listeners.end())
		return;

	auto& vec = it->second;

	// 모두 제거(중복 등록 방지까지 같이 처리)
	vec.erase(std::remove(vec.begin(), vec.end(), listener), vec.end());

	if (vec.empty())
		m_Listeners.erase(it);
}

void EventDispatcher::Dispatch(EventType type, const void* data)
{
	auto it = m_Listeners.find(type);
	if (it == m_Listeners.end())
		return;

	auto listeners = it->second; // 스냅샷 복사
	for (auto* listener : listeners)
	{
		if (listener)
		{
			if (!listener->ShouldHandleEvent(type, data))
				continue;
			listener->OnEvent(type, data);
		}

		if (data && IsMouseEvent(type))
		{
			auto mouseData = static_cast<const Events::MouseState*>(data);
			if (mouseData && mouseData->handled)
				break;
		}
	}
}
