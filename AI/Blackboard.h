#pragma once
#include <unordered_map>
#include <variant>
#include <string>
#include <cstdint>
#include <shared_mutex>

using BBValue = std::variant<std::monostate, bool, int, float, std::string>;

struct BBEntry
{
	BBValue  value{};
	uint32_t version = 0;
	// 값 변경될 때만 version 증가
	// Decorator가 version 비교로 Dirty 판단
};

class Blackboard
{
public:
	template<typename T>
	bool TryGet(const std::string& key, T& out) const
	{
		std::shared_lock lock(m_Mutex);
		auto it = m_Data.find(key);
		if (it == m_Data.end())
			return false;
		if (!std::holds_alternative<T>(it->second.value))
			return false;
		out = std::get<T>(it->second.value);
		return true;
	}

	template<typename T>
	bool Set(const std::string& key, const T& value)
	{
		std::unique_lock lock(m_Mutex);
		auto& e = m_Data[key];

		// 정책: 값 동일하면 version 증가 안 함
		if (std::holds_alternative<T>(e.value) && std::get<T>(e.value) == value)
			return false;

		e.value = value;
		++e.version;
		return true;
	}


	uint32_t GetVersion(const std::string& key) const;

private:
	mutable std::shared_mutex m_Mutex;
	std::unordered_map<std::string, BBEntry> m_Data;
};

