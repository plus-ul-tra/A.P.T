#pragma once
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <stdexcept>
#include <utility>

class ServiceRegistry
{
	struct HolderBase { virtual ~HolderBase() = default; };

	template<typename T>
	struct Holder final : HolderBase
	{
		template<typename... Args>
		explicit Holder(Args&&... args) : value(std::forward<Args>(args)...) {}

		T value;
	};

public:
	template<typename T, typename... Args>
	T& Register(Args&&... args)
	{
		auto key = std::type_index(typeid(T));
		if(m_Services.find(key) != m_Services.end())
			throw std::runtime_error("Register: already registered");

		auto holder = std::make_unique<Holder<T>>(std::forward<Args>(args)...);
		T& value = holder->value;
		m_Services.emplace(key, std::move(holder));
		return value;
	}

	template<typename T>
	T& Get() const
	{
		auto it = m_Services.find(std::type_index(typeid(T)));
		if(it == m_Services.end()) throw std::runtime_error("Get: service not registered");
		return static_cast<Holder<T>*>(it->second.get())->value;
	}

	template<typename T>
	bool Has() const
	{
		return m_Services.find(std::type_index(typeid(T))) != m_Services.end();
	}

	void Reset()
	{
		m_Services.clear();
	}

private:
	std::unordered_map<std::type_index, std::unique_ptr<HolderBase>> m_Services;
};
