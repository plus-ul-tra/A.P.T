#include "CameraObject.h"
#include "TransformComponent.h"
CameraObject::CameraObject(EventDispatcher& eventDispatcher, float width, float height)
	: GameObject(eventDispatcher)
{
	//AddComponent<TransformComponent>();
	AddComponent<CameraComponent>();
	m_CameraComp = GetComponent<CameraComponent>();
	if (m_CameraComp)
	{
		m_CameraComp->SetViewport({width, height});
	}
}

XMFLOAT4X4 CameraObject::GetViewMatrix()
{
	return m_CameraComp->GetViewMatrix();
}

XMFLOAT4X4 CameraObject::GetProjMatrix()
{
	return m_CameraComp->GetProjMatrix();
}

Viewport CameraObject::GetViewportSize() const
{
	return m_CameraComp->GetViewport();
}