#include "FSMEventRegistry.h"

FSMEventRegistry& FSMEventRegistry::Instance()
{
	static FSMEventRegistry instance;

	return instance;
}

void FSMEventRegistry::RegisterEvent(const FSMEventDef& def)
{
	auto& events = m_EventsByCategory[def.category];
	auto& indexMap = m_EventIndexByCategory[def.category];
	auto it = indexMap.find(def.name);
	if (it != indexMap.end())
	{
		events[it->second] = def;
		return;
	}

	indexMap.emplace(def.name, events.size());
	events.push_back(def);
}

const FSMEventDef* FSMEventRegistry::FindEvent(const std::string& category, const std::string& name) const
{
	auto categoryIt = m_EventIndexByCategory.find(category);
	if (categoryIt == m_EventIndexByCategory.end())
	{
		return nullptr;
	}

	const auto& indexMap = categoryIt->second;
	auto it = indexMap.find(name);
	if (it == indexMap.end())
	{
		return nullptr;
	}

	const auto& events = m_EventsByCategory.at(category);
	return &events[it->second];
}

const std::vector<FSMEventDef>& FSMEventRegistry::GetEvents(const std::string& category) const
{
	static const std::vector<FSMEventDef> empty;
	auto it = m_EventsByCategory.find(category);
	if (it == m_EventsByCategory.end())
	{
		return empty;
	}

	return it->second;
}
