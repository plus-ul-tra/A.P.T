#include "FSMActionRegistry.h"

FSMActionRegistry& FSMActionRegistry::Instance()
{
	static FSMActionRegistry instance;

	return instance;
}

void FSMActionRegistry::RegisterAction(const FSMActionDef& def)
{
	auto& actions = m_ActionsByCategory[def.category];
	auto& indexMap = m_ActionIndexByCategory[def.category];
	auto categoryIt = indexMap.find(def.id);
	if (categoryIt != indexMap.end())
	{
		actions[categoryIt->second] = def;
		return;
	}

	indexMap.emplace(def.id, actions.size());
	actions.push_back(def);
}

const FSMActionDef* FSMActionRegistry::FindAction(const std::string& category, const std::string& id) const
{
	auto categoryIt = m_ActionIndexByCategory.find(category);
	if (categoryIt == m_ActionIndexByCategory.end())
	{
		return nullptr;
	}

	const auto& indexMap = categoryIt->second;
	auto it = indexMap.find(id);
	if (it == indexMap.end())
	{
		return nullptr;
	}

	const auto& actions = m_ActionsByCategory.at(category);
	return &actions[it->second];
}

const std::vector<FSMActionDef>& FSMActionRegistry::GetActions(const std::string& category) const
{
	static const std::vector<FSMActionDef> empty;
	auto it = m_ActionsByCategory.find(category);
	if (it == m_ActionsByCategory.end())
	{
		return empty;
	}

	return it->second;
}
