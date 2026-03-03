#include "ReflectionMacro.h"
#include "Scene.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "BoatEndComponent.h"


REGISTER_COMPONENT(BoatEndComponent)
REGISTER_PROPERTY(BoatEndComponent, Height)
REGISTER_PROPERTY(BoatEndComponent, Speed)
REGISTER_PROPERTY(BoatEndComponent, Vertical)
REGISTER_PROPERTY(BoatEndComponent, ForSpeed)

void BoatEndComponent::Start()
{

}

void BoatEndComponent::Update(float deltaTime)
{
	SelfBob(deltaTime);
}

void BoatEndComponent::OnEvent(EventType type, const void* data)
{

}

void BoatEndComponent::SelfBob(float dTime)
{
	Object* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* transform = owner->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	m_BobTime += dTime;

	XMFLOAT3 pos = transform->GetPosition();
	if (m_Vertical) { pos.y += sinf(m_BobTime * m_Speed) * m_Height; }
	else if (!m_Vertical) { pos.x += sinf(m_BobTime * m_Speed) * m_Height; }
	pos.z += dTime * m_ForSpeed;
	transform->SetPosition(pos);
}