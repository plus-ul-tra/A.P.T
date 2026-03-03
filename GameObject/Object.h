#pragma once
#include "EventDispatcher.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <vector>
#include "CoreTypes.h"
#include "Component.h"
#include "RenderData.h"
#include "InitiativeUIComponent.h"

class Scene;

class Object
{
	friend class Scene;

public:
	Object(EventDispatcher& eventDispatcher);
	virtual ~Object() = default;

	template<typename T, typename... Args>
	T* AddComponent(Args&&... args)
	{
		static_assert(std::is_constructible_v<T, Args...>, "Invalid constructor arguments for T.");

		if (!CanAddComponent(typeid(T)))
			return nullptr;

		auto comp = std::make_unique<T>(std::forward<Args>(args)...);
		comp->SetOwner(this);  // 필요한 경우

		T* ptr = comp.get();

		m_Components[T::StaticTypeName].emplace_back(std::move(comp));
		
		return ptr;
	}

	template<typename T>
	T* GetComponent(int index = 0) const
	{
		auto it = m_Components.find(T::StaticTypeName);
		if (it == m_Components.end())
			return nullptr;

		const auto& vec = it->second;
		if (index < 0 || index >= static_cast<int>(vec.size()))
			return nullptr;

		return dynamic_cast<T*>(vec[index].get());
	}

	template<typename T>
	std::vector<T*> GetComponents() const
	{
		std::vector<T*> result;

		auto it = m_Components.find(T::StaticTypeName);
		if (it == m_Components.end())
			return result; // 빈 벡터 반환

		for (const auto& comp : it->second)
		{
			if (auto ptr = dynamic_cast<T*>(comp.get()))
				result.push_back(ptr);
		}

		return result;
	}

	template<typename T>
	std::vector<T*> GetComponentsDerived() const
	{
		std::vector<T*> result;

		for (const auto& [typeName, components] : m_Components)
		{
			for (const auto& comp : components)
			{
				if (auto ptr = dynamic_cast<T*>(comp.get()))
				{
					result.push_back(ptr);
				}
			}
		}

		return result;
	}


	template<typename T>
	void RemoveComponent(T* Component)
	{
		auto it = m_Components.find(T::StaticTypeName);
		if (it == m_Components.end())
			return;
		auto& vec = it->second;

		// 3. vector가 비면 map에서 key 제거
		if (vec.empty())
			m_Components.erase(it);
	}

	bool CanAddComponent(const std::type_info& type) const;
	
	template<typename T>
	bool HasComponent() const
	{
		return GetComponent<T>() != nullptr;
	}

	std::vector<std::string> GetComponentTypeNames() const;

	Component* GetComponentByTypeName(const std::string& typeName, int index = 0) const;
	std::vector<Component*> GetComponentsByTypeName(const std::string& typeName) const;
	bool RemoveComponentByTypeName(const std::string& typeName, int index = 0); // 삭제

	Component* AddComponentByTypeName(const std::string& typeName);

	virtual void Start();
	virtual void Update(float deltaTime);

	virtual void FixedUpdate();

	void SetName(std::string name)
	{
		m_Name = name;
	}

	std::string GetName() const
	{
		return m_Name;
	}

	virtual void Serialize(nlohmann::json& j) const {}
	virtual void Deserialize(const nlohmann::json& j) {}

	void SetScene(Scene* scene) { m_Scene = scene; }
	Scene* GetScene() const		{ return m_Scene;  }

	void SendMessages(const myCore::MessageID msg, void* data = nullptr);

	void SendEvent(const std::string& evt);

	EventDispatcher& GetEventDispatcher() const { return m_EventDispatcher; }

	void SetLayer(RenderData::RenderLayer layer) { m_Layer = layer; }
	RenderData::RenderLayer GetLayer() const { return m_Layer; }

protected:
	std::string m_Name;
	std::unordered_map<std::string, std::vector<std::unique_ptr<Component>>> m_Components;
	EventDispatcher& m_EventDispatcher;
	Scene*		m_Scene = nullptr;
	RenderData::RenderLayer m_Layer = RenderData::RenderLayer::None;
};

