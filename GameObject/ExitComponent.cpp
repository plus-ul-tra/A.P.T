
#include "ExitComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "NodeComponent.h"
#include "MaterialComponent.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include <cmath>

REGISTER_COMPONENT(ExitComponent)
REGISTER_PROPERTY(ExitComponent, TargetScene)

void ExitComponent::Start()
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	m_Node = owner->GetComponent<NodeComponent>();
	m_Material = owner->GetComponent<MaterialComponent>();
	if (m_Material)
	{
		m_BaseMaterialOverrides = m_Material->GetOverrides();
		m_HasBaseMaterial = true;
	}
}

void ExitComponent::Update(float deltaTime)
{
	UpdateHighlight(deltaTime);

	if (!m_SceneChangeRequested && m_Node && m_Node->GetState() == NodeState::HasPlayer)
	{
		RequestSceneChange();
	}
}

void ExitComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void ExitComponent::UpdateHighlight(float deltaTime)
{
	if (!m_Material)
	{
		return;
	}

	if (!m_HasBaseMaterial)
	{
		m_BaseMaterialOverrides = m_Material->GetOverrides();
		m_HasBaseMaterial = true;
	}

	m_HighlightTime += deltaTime;
	const float pulse = 0.5f + 0.5f * std::sinf(m_HighlightTime * 4.0f);
	RenderData::MaterialData overrides = m_BaseMaterialOverrides;
	const auto& baseColor = m_BaseMaterialOverrides.baseColor;
	overrides.baseColor.x = baseColor.x + (1.0f - baseColor.x) * pulse;
	overrides.baseColor.y = baseColor.y + (0.9f - baseColor.y) * pulse;
	overrides.baseColor.z = baseColor.z + (0.2f - baseColor.z) * pulse;
	overrides.baseColor.w = baseColor.w;
	m_Material->SetOverrides(overrides);
}

void ExitComponent::RequestSceneChange()
{
	auto* scene = GetOwner() ? GetOwner()->GetScene() : nullptr;
	if (!scene)
	{
		return;
	}

	auto& services = scene->GetServices();
	auto& gameManager = services.Get<GameManager>();
	gameManager.RequestSceneChange(m_TargetScene);
	m_SceneChangeRequested = true;
}
