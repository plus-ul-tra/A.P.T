#include "Reflection.h"



ComponentRegistry& ComponentRegistry::Instance()
{
    static ComponentRegistry instance;
    return instance;
}

void ComponentRegistry::Register(ComponentTypeInfo* info)
{
    m_Types[info->name] = info;
}

vector<string> ComponentRegistry::GetTypeNames() const
{
    vector<string> names;
    names.reserve(m_Types.size());
    for (const auto& [name, typeInfo] : m_Types)
    {
        names.push_back(name);
    }
    sort(names.begin(), names.end());
    return names;
}
