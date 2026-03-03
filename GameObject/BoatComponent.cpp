
#include "ReflectionMacro.h"
#include "Scene.h"
#include "GameObject.h"
#include "TransformComponent.h"
#include "BoatComponent.h"


REGISTER_COMPONENT(BoatComponent)
REGISTER_PROPERTY(BoatComponent, Height)
REGISTER_PROPERTY(BoatComponent, Speed)
REGISTER_PROPERTY(BoatComponent, Vertical)

void BoatComponent::Start()
{

}

void BoatComponent::Update(float deltaTime)
{
	SelfBob(deltaTime);
}

void BoatComponent::OnEvent(EventType type, const void* data)
{

}

void BoatComponent::SelfBob(float dTime)
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
	if(m_Vertical){ pos.y += sinf(m_BobTime * m_Speed) * m_Height; }
	else if(!m_Vertical) { pos.x += sinf(m_BobTime * m_Speed) * m_Height; }

	transform->SetPosition(pos);
}