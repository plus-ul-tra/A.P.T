#include "Object.h"
#include "SkeletalMeshRenderer.h"
#include "SkeletalMeshComponent.h"
#include "Reflection.h"

Object::Object(EventDispatcher& eventDispatcher) : m_EventDispatcher(eventDispatcher)
{
}

bool Object::CanAddComponent(const std::type_info& type) const
{
	const bool hasMR  = HasComponent<MeshRenderer>();
	const bool hasSMR = HasComponent<SkeletalMeshRenderer>();

	const bool hasMC  = HasComponent<MeshComponent>();
	const bool hasSMC = HasComponent<SkeletalMeshComponent>();

	if ((type == typeid(MeshRenderer) && hasSMR) || (type == typeid(MeshComponent) && hasSMR)) return false;
	if ((type == typeid(SkeletalMeshRenderer) && hasMR) || (type == typeid(SkeletalMeshComponent) && hasMR)) return false;

	if ((type == typeid(MeshComponent) && hasSMC) || (type == typeid(MeshRenderer) && hasSMC)) return false;
	if ((type == typeid(SkeletalMeshComponent) && hasMC) || (type == typeid(SkeletalMeshRenderer) && hasMC)) return false;

	return true;
}

std::vector<std::string> Object::GetComponentTypeNames() const
{
	std::vector<std::string> names;
	names.reserve(m_Components.size());
	for (const auto& entry : m_Components)
	{
		names.push_back(entry.first);
	}
	return names;
}

Component* Object::GetComponentByTypeName(const std::string& typeName, int index) const
{
	auto it = m_Components.find(typeName);
	if (it == m_Components.end())
	{
		return nullptr;
	}

	const auto& vec = it->second;
	if (index < 0 || index >= static_cast<int>(vec.size()))
	{
		return nullptr;
	}

	return vec[index].get();
}

std::vector<Component*> Object::GetComponentsByTypeName(const std::string& typeName) const
{
	std::vector<Component*> result;
	auto it = m_Components.find(typeName);
	if (it == m_Components.end())
	{
		return result;
	}

	result.reserve(it->second.size());
	for (const auto& comp : it->second)
	{
		result.push_back(comp.get());
	}

	return result;
}

bool Object::RemoveComponentByTypeName(const std::string& typeName, int index)
{
	auto it = m_Components.find(typeName);
	if (it == m_Components.end())
	{
		return false;
	}

	auto& vec = it->second;
	if (index < 0 || index >= static_cast<int>(vec.size()))
	{
		return false;
	}

	vec.erase(vec.begin() + index);
	if (vec.empty())
	{
		m_Components.erase(it);
	}
	return true;
}

Component* Object::AddComponentByTypeName(const std::string& typeName)
{
	auto* typeInfo = ComponentRegistry::Instance().Find(typeName);
	if (!typeInfo || !typeInfo->factory)
	{
		return nullptr;
	}

	auto comp = typeInfo->factory();
	if (!comp)
	{
		return nullptr;
	}

	comp->SetOwner(this);
	Component* ptr = comp.get();
	m_Components[typeName].emplace_back(std::move(comp));
	return ptr;
}

void Object::Update(float deltaTime)
{
	for (auto it = m_Components.begin(); it != m_Components.end(); it++)
	{
		for (auto& comp : it->second)
		{
			if (comp->GetIsActive())
			{
				comp->Update(deltaTime);
			}
		}
	}
}

void Object::Start()
{
	for (auto& [name, comps] : m_Components)
	{
		for (auto& comp : comps)
		{
			comp->Start();
		}
	}
}

void Object::FixedUpdate()
{

}

void Object::SendMessages(const myCore::MessageID msg, void* data /*= nullptr*/)
{
	for (auto it = m_Components.begin(); it != m_Components.end(); it++)
	{
		for (auto& comp : it->second)
		{
			comp->HandleMessage(msg, data);
		}
	}
}

void Object::SendEvent(const std::string& evt)
{
	for (auto it = m_Components.begin(); it != m_Components.end(); it++)
	{
		for (auto& comp : it->second)
		{
			//comp->OnEvent(evt);
		}
	}
}
