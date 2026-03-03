#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include "CoreTypes.h"
#include "EventDispatcher.h"
#include "IEventListener.h"
#include "json.hpp"

class Object;

class Component
{
public:
	Component() = default;
	virtual ~Component() = default;
	virtual void Start() {}
	virtual void Update(float deltaTime) = 0;
	virtual void OnEvent(EventType type, const void* data) abstract;
	virtual const char* GetTypeName() const = 0;

	void HandleMessage(myCore::MessageID msg, void* data)
	{
		auto it = m_MessageHandlers.find(msg);
		if (it != m_MessageHandlers.end())
		{
			for (auto& handler : it->second)
			{
				handler(data);
			}
		}
	}

	// Property 순회로 변경
	virtual void Serialize(nlohmann::json& j) const;
	virtual void Deserialize(const nlohmann::json& j);

	using HandlerType = std::function<void(void*)>;
	void RegisterMessageHandler(myCore::MessageID msg, HandlerType handler)
	{
		m_MessageHandlers[msg].emplace_back(std::move(handler));
	}

	void SetOwner(Object* owner) { m_Owner = owner; }
	Object* GetOwner() const { return m_Owner; }

	void SetIsActive(bool active) { m_IsActive = active; }
	bool GetIsActive() { return m_IsActive; }
protected:
	bool    m_IsActive = true;
	Object* m_Owner = nullptr;
	std::unordered_map<myCore::MessageID, std::vector<HandlerType>> m_MessageHandlers;
	EventDispatcher& GetEventDispatcher() const;
};

using ComponentCreateFunc = std::function<std::unique_ptr<Component>()>;

class ComponentFactory 
{
public:
	static ComponentFactory& Instance()
	{
		static ComponentFactory instance;
		return instance;
	}

	void Register(const std::string& typeName, ComponentCreateFunc func)
	{
		factoryMap[typeName] = func;
	}

	std::unique_ptr<Component> Create(const std::string& typeName)
	{
		if (factoryMap.find(typeName) != factoryMap.end())
		{
			return factoryMap[typeName]();
		}
		return nullptr;
	}

private:
	std::unordered_map<std::string, ComponentCreateFunc> factoryMap;
};
