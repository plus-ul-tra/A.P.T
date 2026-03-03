#include "CollisionFSMComponent.h"
#include "FSMActionRegistry.h"
#include "FSMEventRegistry.h"
#include "Object.h"
#include "ReflectionMacro.h"

void RegisterCollisionFSMDefinitions()
{
	auto& actionRegistry = FSMActionRegistry::Instance();
	actionRegistry.RegisterAction({
		"SetComponentActive",
		"Collision",
		{
			{ "component", "string", "", true },
			{ "active", "bool", true, false }
		}
		});

	auto& eventRegistry = FSMEventRegistry::Instance();
	eventRegistry.RegisterEvent({ "Collision_Enter", "Collision" });
	eventRegistry.RegisterEvent({ "Collision_Stay", "Collision" });
	eventRegistry.RegisterEvent({ "Collision_Exit", "Collision" });
	eventRegistry.RegisterEvent({ "Collision_Trigger", "Collision" });
}


REGISTER_COMPONENT_DERIVED(CollisionFSMComponent, FSMComponent)

CollisionFSMComponent::CollisionFSMComponent()
{
	BindActionHandler("SetComponentActive", [this](const FSMAction& action)
		{
			if (!GetOwner())
				return;

			const std::string componentName = action.params.value("component", "");
			if (componentName.empty())
				return;

			const bool active = action.params.value("active", true);
			auto components = GetOwner()->GetComponentsByTypeName(componentName);
			for (auto* component : components)
			{
				if (component)
				{
					component->SetIsActive(active);
				}
			}
		});
}

CollisionFSMComponent::~CollisionFSMComponent()
{
	GetEventDispatcher().RemoveListener(EventType::CollisionEnter, this);
	GetEventDispatcher().RemoveListener(EventType::CollisionStay, this);
	GetEventDispatcher().RemoveListener(EventType::CollisionExit, this);
	GetEventDispatcher().RemoveListener(EventType::CollisionTrigger, this);
}

void CollisionFSMComponent::Start()
{
	FSMComponent::Start();
	GetEventDispatcher().AddListener(EventType::CollisionEnter, this);
	GetEventDispatcher().AddListener(EventType::CollisionStay, this);
	GetEventDispatcher().AddListener(EventType::CollisionExit, this);
	GetEventDispatcher().AddListener(EventType::CollisionTrigger, this);
}

std::optional<std::string> CollisionFSMComponent::TranslateEvent(EventType type, const void* data)
{
	switch (type)
	{
	case EventType::CollisionEnter:
		return std::string("Collision_Enter");
	case EventType::CollisionStay:
		return std::string("Collision_Stay");
	case EventType::CollisionExit:
		return std::string("Collision_Exit");
	case EventType::CollisionTrigger:
		return std::string("Collision_Trigger");
	default:
		return std::nullopt;
	}
}
