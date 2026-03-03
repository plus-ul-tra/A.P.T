#pragma once

#include <string>
#include <vector>
#include <unordered_map>

struct FSMEventDef
{
	std::string name;
	std::string category;
};

class FSMEventRegistry
{
public:
	static FSMEventRegistry& Instance();

	void RegisterEvent(const FSMEventDef& def);
	const FSMEventDef* FindEvent(const std::string& category, const std::string& name) const;
	const std::vector<FSMEventDef>& GetEvents(const std::string& category) const;

private:
	std::unordered_map<std::string, std::vector<FSMEventDef>> m_EventsByCategory;
	std::unordered_map<std::string, std::unordered_map<std::string, size_t>> m_EventIndexByCategory;
};

