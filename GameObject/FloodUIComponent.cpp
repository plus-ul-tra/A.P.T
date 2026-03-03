#include "FloodUIComponent.h"
#include "FloodSystemComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "UIProgressBarComponent.h"
#include "Scene.h"
#include "GameObject.h"
#include <algorithm>

REGISTER_UI_COMPONENT(FloodUIComponent)
REGISTER_PROPERTY_READONLY(FloodUIComponent, DisplayedWaterLevel)
REGISTER_PROPERTY_READONLY(FloodUIComponent, DisplayedTimeRemaining)
REGISTER_PROPERTY_READONLY(FloodUIComponent, DisplayedGameOver)

void FloodUIComponent::Start()
{
	m_FloodSystem = nullptr;
}

void FloodUIComponent::Update(float deltaTime)
{
	(void)deltaTime;
	if (!m_FloodSystem)
	{
		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		if (!scene)
			return;

		for (const auto& [name, object] : scene->GetGameObjects())
		{
			if (!object)
				continue;

			m_FloodSystem = object->GetComponent<FloodSystemComponent>();
			if (m_FloodSystem)
				break;
		}
		if (!m_FloodSystem)
			return;
	}

	m_DisplayedWaterLevel	 = m_FloodSystem->GetWaterLevel();
	m_DisplayedTimeRemaining = m_FloodSystem->GetTurnRemaining();
	m_DisplayedGameOver		 = m_FloodSystem->GetGameOver();

	if (auto* owner = GetOwner())
	{
		if (auto* progress = owner->GetComponent<UIProgressBarComponent>())
		{
			progress->SetPercent(std::clamp(m_DisplayedWaterLevel, 0.0f, 1.0f));
		}
	}
}

void FloodUIComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}
