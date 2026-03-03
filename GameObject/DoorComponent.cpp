#include "DoorComponent.h"
#include "Object.h"
#include "Scene.h"
#include "GameObject.h"
#include "ReflectionMacro.h"
#include "AnimFSMComponent.h"
#include "AnimationComponent.h"
#include "NodeComponent.h"

REGISTER_COMPONENT(DoorComponent)
REGISTER_PROPERTY(DoorComponent, LinkedNodeName)

DoorComponent::DoorComponent()
{
}

DoorComponent::~DoorComponent()
{
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
}

void DoorComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	// Setting 필요 component
	Resolve();

}

void DoorComponent::Update(float deltaTime)
{
	if (!m_Node || !m_AnimFsm || !m_Animation) {
		Resolve();
	}
}

void DoorComponent::OnEvent(EventType type, const void* data)
{
	
}

// 문열림 처리
void DoorComponent::OpenDoor()
{
	if (m_IsOpen) { return; }

	if (m_Node) {
		m_Node->SetIsMoveable(true); // 노드열기
		m_Node->SetIsSight(true);    // 노드시야 열기
	}

	if (m_AnimFsm) {
		m_AnimFsm->DispatchEvent("Anim_Play");
	}
	else if (m_Animation) {
		m_Animation->Play();
	}

	m_IsOpen = true;
}

void DoorComponent::Resolve()
{
	auto* owner = GetOwner();
	if (!owner) { return; }

	if (!m_Node) {
		if (!m_LinkedNodeName.empty()) {
			auto* scene = owner->GetScene();

			if (scene) {

				const auto& objects = scene->GetGameObjects();
				const auto it = objects.find(m_LinkedNodeName);

				if (it != objects.end() && it->second) {
					m_Node = it->second->GetComponent<NodeComponent>();
				}
			}
		}

		if (!m_Node)
		{
			m_Node = owner->GetComponent<NodeComponent>();
		}
	}

	if (m_Node && !m_Node->GetLinkedDoor())
		{
		m_Node->SetLinkedDoor(this, owner->GetName());
		}

	if (!m_AnimFsm)
	{
		m_AnimFsm = owner->GetComponent<AnimFSMComponent>();
	}

	if (!m_Animation)
	{
		auto anims = owner->GetComponentsDerived<AnimationComponent>();
		m_Animation = anims.empty() ? nullptr : anims.front();
	}
}
