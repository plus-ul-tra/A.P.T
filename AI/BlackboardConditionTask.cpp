#include "BlackboardConditionTask.h"
#include "Blackboard.h"

BlackboardConditionTask::BlackboardConditionTask(std::string key, bool expected)
    : m_Key(std::move(key)), m_Expected(expected)
{
}

BTStatus BlackboardConditionTask::OnTick(BTInstance& inst, Blackboard& bb)
{
    (void)inst;
    bool value = false;
    if (!bb.TryGet(m_Key, value))
    {
        return BTStatus::Failure;
    }

    return value == m_Expected ? BTStatus::Success : BTStatus::Failure;
}
