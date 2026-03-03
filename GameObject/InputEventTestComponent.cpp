#include "InputEventTestComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "BoxColliderComponent.h"
#include "Event.h"
#include "RayHelper.h"
#include "UIObject.h"
#include "Scene.h"
#include "CameraObject.h"
#include <iostream>

REGISTER_COMPONENT(InputEventTestComponent)

InputEventTestComponent::InputEventTestComponent()
{
}

InputEventTestComponent::~InputEventTestComponent()
{
	GetEventDispatcher().RemoveListener(EventType::Pressed, this);
	GetEventDispatcher().RemoveListener(EventType::Released, this);
	GetEventDispatcher().RemoveListener(EventType::UIHovered, this);
	GetEventDispatcher().RemoveListener(EventType::UIDragged, this);
	GetEventDispatcher().RemoveListener(EventType::UIDoubleClicked, this);
	GetEventDispatcher().RemoveListener(EventType::Hovered, this);
	GetEventDispatcher().RemoveListener(EventType::Dragged, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClickHold, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClickUp, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftDoubleClick, this);
}

void InputEventTestComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::Pressed, this);
	GetEventDispatcher().AddListener(EventType::Released, this);
	GetEventDispatcher().AddListener(EventType::UIHovered, this);
	GetEventDispatcher().AddListener(EventType::UIDragged, this);
	GetEventDispatcher().AddListener(EventType::UIDoubleClicked, this);
	GetEventDispatcher().AddListener(EventType::Hovered, this);
	GetEventDispatcher().AddListener(EventType::Dragged, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClickHold, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClickUp, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftDoubleClick, this);
}

void InputEventTestComponent::Update(float deltaTime)
{
	(void)deltaTime;
}

void InputEventTestComponent::OnEvent(EventType type, const void* data)
{
	const auto* mouseData = static_cast<const Events::MouseState*>(data);

	switch (type)
	{
	case EventType::Pressed:
		LogEvent("UI Pressed", mouseData);
		if (mouseData)
			mouseData->handled = true;
		break;
	case EventType::Released:
		LogEvent("UI Released", mouseData);
		if (mouseData)
			mouseData->handled = true;
		break;
	case EventType::UIDragged:
		LogEvent("UI Dragged", mouseData);
		if (mouseData)
			mouseData->handled = true;
		break;
	case EventType::UIDoubleClicked:
		LogEvent("UI DoubleClicked", mouseData);
		if (mouseData)
			mouseData->handled = true;
		break;
	case EventType::Hovered:
		LogEvent("Game Hovered", mouseData);
		break;
	case EventType::UIHovered:
		LogEvent("UI Hovered", mouseData);
		if (mouseData)
			mouseData->handled = true;
		break;
	case EventType::Dragged:
		LogEvent("Game Dragged", mouseData);
		break;
	case EventType::MouseLeftClick:
		LogEvent("Game Click", mouseData);
		break;
	case EventType::MouseLeftClickHold:
		LogEvent("Game ClickHold", mouseData);
		break;
	case EventType::MouseLeftClickUp:
		LogEvent("Game ClickUp", mouseData);
		break;
	case EventType::MouseLeftDoubleClick:
		LogEvent("Game DoubleClick", mouseData);
		break;
	default:
		break;
	}
}

bool InputEventTestComponent::ShouldHandleEvent(EventType type, const void* data)
{
	auto* bound = GetOwner()->GetComponent<BoxColliderComponent>();
	if (!bound)
		return true;

	switch (type)
	{
	case EventType::MouseLeftClick:
	case EventType::MouseLeftClickHold:
	case EventType::MouseLeftClickUp:
	case EventType::MouseLeftDoubleClick:
	case EventType::MouseRightClick:
	case EventType::MouseRightClickHold:
	case EventType::MouseRightClickUp:
	case EventType::Dragged:
	case EventType::UIHovered:
	case EventType::Pressed:
	case EventType::Released:
	case EventType::Moved:
	case EventType::UIDragged:
	case EventType::UIDoubleClicked:
	case EventType::Hovered:
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		return mouseData ? IsWithinBounds(mouseData->pos) : false;
	}
	default:
		return true;
	}
}


void InputEventTestComponent::LogEvent(const char* label, const Events::MouseState* mouseData) const
{
	const auto* owner = GetOwner();
	const std::string ownerName = owner ? owner->GetName() : "Unknown";

	if (mouseData)
	{
		std::cout << "[InputEventTest] " << ownerName << " - " << label
			<< " (" << mouseData->pos.x << ", " << mouseData->pos.y << ")"
			<< std::endl;
	}
	else
	{
		std::cout << "[InputEventTest] " << ownerName << " - " << label << std::endl;
	}
}

bool InputEventTestComponent::IsWithinBounds(const POINT& pos) const
{
	if (auto* uiObject = dynamic_cast<UIObject*>(GetOwner()))
	{
		return uiObject->HitCheck(pos);
	}

	auto* owner = GetOwner();
	if (owner)
	{
		if (auto* collider = owner->GetComponent<BoxColliderComponent>())
		{
			auto* scene = owner->GetScene();
			if (scene)
			{
				auto cameraObject = scene->GetGameCamera();
				if (!cameraObject)
				{
					cameraObject = scene->GetEditorCamera();
				}

				if (cameraObject)
				{
					auto* camera = cameraObject->GetComponent<CameraComponent>();
					if (camera)
					{
						const auto view = camera->GetViewMatrix();
						const auto proj = camera->GetProjMatrix();
						const auto& viewport = camera->GetViewport();
						if (viewport.Width > 0.0f && viewport.Height > 0.0f)
						{
							const auto viewMat = DirectX::XMLoadFloat4x4(&view);
							const auto projMat = DirectX::XMLoadFloat4x4(&proj);
							const Ray ray = MakePickRayLH(
								static_cast<float>(pos.x),
								static_cast<float>(pos.y),
								0.0f,
								0.0f,
								viewport.Width,
								viewport.Height,
								viewMat,
								projMat);

							float hitT = 0.0f;
							if (collider->IntersectsRay(ray.m_Pos, ray.m_Dir, hitT))
							{
								return true;
							}
						}
					}
				}
			}
		}
	}

	return false;
}
