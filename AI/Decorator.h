#pragma once
#include <vector>
#include <string>
#include <cstdint>

class BTInstance;
class Blackboard;

enum class AbortMode
{
	None, 
	Self,
	LowerPriority,
	Both
};

class Decorator
{
public:
	virtual ~Decorator() = default;

	void      SetAbortMode(AbortMode mode) { m_Abort = mode; }
	AbortMode GetAbortMode() const         { return m_Abort; }

	void ObserveKey(const std::string& key)	// Blackboard에서 관찰할 키
	{
		m_ObservedKeys.push_back(std::move(key));
	}

	const std::vector<std::string>& GetObservedKeys() const
	{
		return m_ObservedKeys;
	}

	virtual bool Evaluate(BTInstance&, Blackboard&) const = 0;


private:
	AbortMode m_Abort = AbortMode::None;
	std::vector<std::string> m_ObservedKeys;
};

