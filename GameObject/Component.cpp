#include "Component.h"
#include "GameObject.h"
#include "Reflection.h"

EventDispatcher& Component::GetEventDispatcher() const
{
    return m_Owner->GetEventDispatcher();
}

void Component::Serialize(nlohmann::json& j) const {
	auto* typeInfo = ComponentRegistry::Instance().Find(GetTypeName());
	if (!typeInfo) return;

	auto props = ComponentRegistry::Instance().CollectProperties(typeInfo);

	for (const auto& prop : props) {
		if (!prop->IsSerializable())
		{
			continue;
		}
		prop->Serialize(const_cast<Component*>(this), j);
	}
}

void Component::Deserialize(const nlohmann::json& j) {

	auto* typeInfo = ComponentRegistry::Instance().Find(GetTypeName());
	if (!typeInfo) return;

	auto props = ComponentRegistry::Instance().CollectProperties(typeInfo);

	for (const auto& prop : props) {
		if (!prop->IsSerializable())
		{
			continue;
		}
		prop->Deserialize(this, j);
	}
}