#include "Blackboard.h"

uint32_t Blackboard::GetVersion(const std::string& key) const
{
    std::shared_lock lock(m_Mutex);
    auto it = m_Data.find(key);

    return  (it == m_Data.end()) ? 0u : it->second.version;
}
