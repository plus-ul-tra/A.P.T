
#include "NodeComponent.h"
#include "ReflectionMacro.h"
#include "MaterialComponent.h"
#include "GameObject.h"
#include "DoorComponent.h"
#include "Object.h"
#include "Scene.h"
#include "Event.h"
#include <algorithm>

REGISTER_COMPONENT(NodeComponent)
REGISTER_PROPERTY(NodeComponent, IsMoveable)
REGISTER_PROPERTY(NodeComponent, IsSight)
REGISTER_PROPERTY(NodeComponent, LinkedDoorName)
REGISTER_PROPERTY_READONLY(NodeComponent, StateInt)
REGISTER_PROPERTY_READONLY(NodeComponent, Q)
REGISTER_PROPERTY_READONLY(NodeComponent, R)


namespace
{
	bool AreMaterialOverridesEqual(const RenderData::MaterialData& lhs, const RenderData::MaterialData& rhs)
	{
		if (lhs.baseColor.x != rhs.baseColor.x || lhs.baseColor.y != rhs.baseColor.y
			|| lhs.baseColor.z != rhs.baseColor.z || lhs.baseColor.w != rhs.baseColor.w)
		{
			return false;
		}
		if (lhs.metallic != rhs.metallic || lhs.roughness != rhs.roughness
			|| lhs.saturation != rhs.saturation || lhs.lightness != rhs.lightness)
		{
			return false;
		}
		if (lhs.shaderAsset != rhs.shaderAsset
			|| lhs.vertexShader != rhs.vertexShader
			|| lhs.pixelShader != rhs.pixelShader)
		{
			return false;
		}
		return lhs.textures == rhs.textures;
	}
}

NodeComponent::NodeComponent() = default;

NodeComponent::~NodeComponent() {
	// Event Listener 쓰는 경우만
	/*GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);*/
}


void NodeComponent::Start()
{
	// 이벤트 추가
	/*GetEventDispatcher().AddListener(EventType::KeyDown, this);
	GetEventDispatcher().AddListener(EventType::KeyUp, this);*/
	auto* owner = GetOwner();
	if (owner)
	{
		m_Material = owner->GetComponent<MaterialComponent>();
		if (m_Material)
		{
			m_BaseMaterialOverrides = m_Material->GetOverrides();
			m_HasBaseMaterial = true;
		}
	}
	ResolveLinkedDoor(); // Door 연결
}


void NodeComponent::Update(float deltaTime) {

	if (!m_LinkedDoor && !m_LinkedDoorName.empty())
	{
		ResolveLinkedDoor();
	}

	switch (m_State)
	{
	case NodeState::Empty : m_StateInt = 0; break;
	case NodeState::HasPlayer: m_StateInt = 1; break;
	case NodeState::HasEnemy: m_StateInt = 2; break;
	}
}



void NodeComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}



void NodeComponent::ClearNeighbors()
{
	m_Neighbors.clear();
}



void NodeComponent::AddNeighbor(NodeComponent* node)
{
	if (!node) {
		return; 
	}
	m_Neighbors.push_back(node);
}

// 강조 표시
void NodeComponent::SetMoveRangeHighlight(float intensity, bool enabled)
{
	m_UsingMoveRangeHighlight = enabled;
	m_MoveHighlightIntensity = intensity;
	ApplyHighlight();
}

void NodeComponent::SetThrowRangeHighlight(float intensity, bool enabled)
{
	m_UsingThrowRangeHighlight = enabled;
	m_ThrowHighlightIntensity = intensity;
	ApplyHighlight();
}

void NodeComponent::SetSightHighlight(float intensity, bool enabled)
{
	m_UsingSightHighlight = enabled;
	m_SightHighlightIntensity = intensity;
	ApplyHighlight();
}

void NodeComponent::ClearHighlights()
{
	m_UsingMoveRangeHighlight = false;
	m_MoveHighlightIntensity = 0.0f;
	m_UsingThrowRangeHighlight = false;
	m_ThrowHighlightIntensity = 0.0f;
	m_UsingSightHighlight = false;
	m_SightHighlightIntensity = 0.0f;
	ApplyHighlight();
}

void NodeComponent::SetLinkedDoor(DoorComponent* door, const std::string& doorName)
{
	if (!door)
	{
		return;
	}

	m_LinkedDoor = door;
	if (!doorName.empty())
	{
		m_LinkedDoorName = doorName;
	}
}

void NodeComponent::ApplyHighlight()
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

	if (!m_UsingMoveRangeHighlight && !m_UsingThrowRangeHighlight && !m_UsingSightHighlight) 
	{
		m_Material->SetOverrides(m_BaseMaterialOverrides);
	/*	m_LastAppliedOverrides = m_BaseMaterialOverrides;
		m_HasLastAppliedOverrides = true;*/
		return;
	}
	/*const auto& currentOverrides = m_Material->GetOverrides();
	if (m_HasLastAppliedOverrides && !AreMaterialOverridesEqual(currentOverrides, m_LastAppliedOverrides))
	{
		m_BaseMaterialOverrides = currentOverrides;
		m_HasBaseMaterial = true;
	}*/
	const float moveIntensity = m_UsingMoveRangeHighlight ? std::clamp(m_MoveHighlightIntensity, 0.0f, 1.0f) : 0.0f;
	const float throwIntensity = m_UsingThrowRangeHighlight ? std::clamp(m_ThrowHighlightIntensity, 0.0f, 1.0f) : 0.0f;
	const float sightIntensity = m_UsingSightHighlight ? std::clamp(m_SightHighlightIntensity, 0.0f, 1.0f) : 0.0f;
	const float moveTotal = moveIntensity + throwIntensity;
	const float total = moveTotal + sightIntensity;
	const float combinedIntensity = std::clamp(total, 0.0f, 1.0f);
	RenderData::MaterialData overrides = m_BaseMaterialOverrides;
	const auto& baseColor = m_BaseMaterialOverrides.baseColor;
	const DirectX::XMFLOAT4 moveColor{ 0.2f, 1.0f, 0.2f, baseColor.w };
	const DirectX::XMFLOAT4 sightColor{ 1.0f, 0.2f, 0.2f, baseColor.w };
	const DirectX::XMFLOAT4 throwColor{ 0.2f, 0.2f, 1.0f, baseColor.w };

	DirectX::XMFLOAT4 blendedColor = moveColor;
	if (total > 0.0f)
	{
		const float moveWeight = moveTotal / total;
		const float throwWeight = throwIntensity / total;
		const float sightWeight = sightIntensity / total;
		blendedColor.x =
			moveColor.x * moveWeight +
			throwColor.x * throwWeight +
			sightColor.x * sightWeight;

		blendedColor.y =
			moveColor.y * moveWeight +
			throwColor.y * throwWeight +
			sightColor.y * sightWeight;

		blendedColor.z =
			moveColor.z * moveWeight +
			throwColor.z * throwWeight +
			sightColor.z * sightWeight;

		blendedColor.w = baseColor.w;
	}

	overrides.baseColor.x = baseColor.x + (blendedColor.x - baseColor.x) * combinedIntensity;
	overrides.baseColor.y = baseColor.y + (blendedColor.y - baseColor.y) * combinedIntensity;
	overrides.baseColor.z = baseColor.z + (blendedColor.z - baseColor.z) * combinedIntensity;
	overrides.baseColor.w = baseColor.w;

	m_Material->SetOverrides(overrides);
	/*m_LastAppliedOverrides = overrides;
	m_HasLastAppliedOverrides = true;*/
	//m_UsingMoveRangeHighlight = true;
}

void NodeComponent::ResolveLinkedDoor()
{
	//std::cout << "ResolveLinkedDoor" << std::endl;
	if (m_LinkedDoorName.empty() || m_LinkedDoor)
	{
		return;
	}

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;

	if (!scene)
	{
		return;
	}

	const auto& objects = scene->GetGameObjects();
	const auto it = objects.find(m_LinkedDoorName);
	if (it == objects.end() || !it->second)
	{
		return;
	}

	m_LinkedDoor = it->second->GetComponent<DoorComponent>();
	if (m_LinkedDoor) { std::cout << "Door Linked" << std::endl; }
}
