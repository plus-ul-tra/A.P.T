#include "Component.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include <unordered_map>


GameObject::GameObject(EventDispatcher& eventDispatcher) : Object(eventDispatcher)
{
	m_Transform = AddComponent<TransformComponent>();
}

void GameObject::Serialize(nlohmann::json& j) const
{
	j["name"] = m_Name;
	if (m_Transform && m_Transform->GetParent())
	{
		j["parent"] = m_Transform->GetParent()->GetOwner()->GetName();
	}
	j["components"] = nlohmann::json::array();
	for (const auto& component : m_Components)
	{
		for (const auto& comp : component.second)
		{
			//Reflection 으로 교체
			nlohmann::json compJson;
			compJson["type"] = comp->GetTypeName();
			comp->Serialize(compJson["data"]);
			j["components"].push_back(compJson);
		}
	}
}

void GameObject::Deserialize(const nlohmann::json& j)
{
	m_Name = j.at("name");
	std::unordered_map<std::string, size_t> desiredCounts;

	for (const auto& compJson : j.at("components"))
	{
		std::string typeName = compJson.at("type");
		++desiredCounts[typeName];
		// 기존 컴포넌트가 있으면 찾아서 갱신
		auto it = m_Components.find(typeName);
		if (it != m_Components.end() && !it->second.empty())
		{
			// 해당 타입의 모든 컴포넌트에 대해 Deserialize 시도
			for (auto& comp : it->second)
			{
				comp->Deserialize(compJson.at("data"));
			}
		}
		else
		{
			// 없으면 새로 생성 후 추가
			auto comp = ComponentFactory::Instance().Create(typeName);
			if (comp)
			{
				comp->Deserialize(compJson.at("data"));
				AddComponent(std::move(comp));
			}
		}
	}

	for (auto it = m_Components.begin(); it != m_Components.end();)
	{
		const auto desiredIt = desiredCounts.find(it->first);
		const size_t desiredCount = (desiredIt != desiredCounts.end()) ? desiredIt->second : 0;
		auto& components = it->second;

		while (components.size() > desiredCount)
		{
			components.pop_back();
		}

		if (components.empty())
		{
			it = m_Components.erase(it);
		}
		else
		{
			++it;
		}
	}
}