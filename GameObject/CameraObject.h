#pragma once
#include "GameObject.h"
#include "CameraComponent.h"

class CameraObject : public GameObject
{
public:

	CameraObject(EventDispatcher& eventDispatcher, float width, float height);
	virtual ~CameraObject() = default;

	XMFLOAT4X4 GetViewMatrix();
	XMFLOAT4X4 GetProjMatrix();
	Viewport   GetViewportSize() const;
	const XMFLOAT3& GetEye () const { return m_CameraComp->GetEye();  }
	const XMFLOAT3& GetLook() const { return m_CameraComp->GetLook(); }
	const XMFLOAT3& GetUp  () const { return m_CameraComp->GetUp();   }
	float GetNearZ() const { return m_CameraComp->GetNearZ(); }
	float GetFarZ() const { return m_CameraComp->GetFarZ(); }
private:
	CameraComponent* m_CameraComp;
};

