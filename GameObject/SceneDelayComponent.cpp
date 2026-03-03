#include "SceneDelayComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "GameManager.h"

#include <algorithm>

REGISTER_COMPONENT(SceneDelayComponent)
REGISTER_PROPERTY(SceneDelayComponent, TargetScene)
REGISTER_PROPERTY(SceneDelayComponent, DelaySeconds)
REGISTER_PROPERTY(SceneDelayComponent, Enabled)
REGISTER_PROPERTY_READONLY(SceneDelayComponent, Requested)
REGISTER_PROPERTY_READONLY(SceneDelayComponent, ElapsedSeconds)

void SceneDelayComponent::Start()
{
	ResetTimer();
}

void SceneDelayComponent::Update(float deltaTime)
{
	if (!m_Enabled || m_Requested)
	{
		return;
	}

	m_ElapsedSeconds += deltaTime;
	if (m_ElapsedSeconds >= (std::max)(0.0f, m_DelaySeconds))
	{
		RequestSceneChange();
	}
}

void SceneDelayComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void SceneDelayComponent::ResetTimer()
{
	m_ElapsedSeconds = 0.0f;
	m_Requested = false;
}

void SceneDelayComponent::RequestSceneChange()
{
	auto* scene = GetOwner() ? GetOwner()->GetScene() : nullptr;
	if (!scene)
	{
		return;
	}

	auto& services = scene->GetServices();
	auto& gameManager = services.Get<GameManager>();
	gameManager.RequestSceneChange(m_TargetScene);
	m_Requested = true;
}