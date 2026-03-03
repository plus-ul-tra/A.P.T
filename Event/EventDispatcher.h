#pragma once
#include "IEventListener.h"
#include "Event.h"
#include <unordered_map>
#include <vector>

class EventDispatcher
{
public:
	EventDispatcher() = default;
	~EventDispatcher() { m_IsAlive = false; }

	void AddListener(EventType type, IEventListener* listener);
	void RemoveListener(EventType type, IEventListener* listener);
	void Dispatch(EventType type, const void* data);
	bool IsAlive() const { return m_IsAlive; }
	std::vector<IEventListener*>* FindListeners(EventType type)
	{
		if (m_Listeners.empty())
			return nullptr;

		auto it = m_Listeners.find(type);
		if (it == m_Listeners.end())
			return nullptr;
		return &it->second;
	}

	const std::vector<IEventListener*>* FindListeners(EventType type) const
	{
		auto it = m_Listeners.find(type);
		if (it == m_Listeners.end())
			return nullptr;
		return &it->second;
	}

private :
	std::unordered_map<EventType, std::vector<IEventListener*>> m_Listeners;
	bool m_IsAlive = true;
};

