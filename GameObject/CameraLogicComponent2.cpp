
#include "CameraLogicComponent2.h"
#include "CameraComponent.h"
#include "Event.h"
#include "ReflectionMacro.h"
#include "TransformComponent.h"
#include "ServiceRegistry.h"
#include "Scene.h"
#include "InputManager.h"

#include <algorithm>

REGISTER_COMPONENT(CameraLogicComponent2)
REGISTER_PROPERTY(CameraLogicComponent2, MaxZoom)
REGISTER_PROPERTY(CameraLogicComponent2, MinZoom)
REGISTER_PROPERTY(CameraLogicComponent2, MinX)
REGISTER_PROPERTY(CameraLogicComponent2, MaxX)
REGISTER_PROPERTY(CameraLogicComponent2, MinZ)
REGISTER_PROPERTY(CameraLogicComponent2, MaxZ)
REGISTER_PROPERTY(CameraLogicComponent2, MoveSpeed)

CameraLogicComponent2::~CameraLogicComponent2()
{
	GetEventDispatcher().RemoveListener(EventType::MouseWheelUp, this);
	GetEventDispatcher().RemoveListener(EventType::MouseWheelDown, this);
}

void CameraLogicComponent2::Start()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	GetEventDispatcher().AddListener(EventType::MouseWheelUp, this);
	GetEventDispatcher().AddListener(EventType::MouseWheelDown, this);

	auto* trans = owner->GetComponent<TransformComponent>();
	auto* cam = owner->GetComponent<CameraComponent>();
	if (!trans || !cam)
		return;

	m_Transform = trans;
	m_Camera = cam;
}

void CameraLogicComponent2::Update(float deltaTime)
{
	if (!m_Transform || !m_Camera)
		return;

	CamZoom();
	MoveInBound(deltaTime);
}

void CameraLogicComponent2::OnEvent(EventType type, const void* data)
{
	(void)data;
	if (type == EventType::MouseWheelUp)
	{
		m_PendingZoomInput += 1.0f;
		return;
	}

	if (type == EventType::MouseWheelDown)
	{
		m_PendingZoomInput -= 1.0f;
		return;
	}
}

void CameraLogicComponent2::CamZoom()
{
	if (m_PendingZoomInput == 0.0f)
		return;

	const XMFLOAT3 currentEye = m_Camera->GetEye();
	const XMFLOAT3 currentLook = m_Camera->GetLook();

	XMVECTOR eyeVec = XMLoadFloat3(&currentEye);
	XMVECTOR lookVec = XMLoadFloat3(&currentLook);
	XMVECTOR forwardVec = XMVectorSubtract(lookVec, eyeVec);
	if (XMVectorGetX(XMVector3LengthSq(forwardVec)) < 0.000001f)
	{
		m_PendingZoomInput = 0.0f;
		return;
	}
	forwardVec = XMVector3Normalize(forwardVec);

	const float requestedZoomDelta = m_PendingZoomInput * m_ZoomSpeed;
	float clampedMin = (std::min)(m_MinZoom, m_MaxZoom);
	float clampedMax = (std::max)(m_MinZoom, m_MaxZoom);

	float desiredZoomOffset = m_CurrentZoomOffset + requestedZoomDelta;
	if (clampedMax > clampedMin)
	{
		desiredZoomOffset = std::clamp(desiredZoomOffset, clampedMin, clampedMax);
	}
	const float appliedZoomDelta = desiredZoomOffset - m_CurrentZoomOffset;
	m_CurrentZoomOffset = desiredZoomOffset;

	// EditorCamera와 동일하게 Eye/Look을 같은 dolly로 이동시키되,
	// Min/Max는 누적 offset 범위를 clamp해 적용한다.
	const XMVECTOR dolly = XMVectorScale(forwardVec, appliedZoomDelta);
	const XMVECTOR newEyeVec = XMVectorAdd(eyeVec, dolly);
	const XMVECTOR newLookVec = XMVectorAdd(lookVec, dolly);
	m_PendingZoomInput = 0.0f;

	XMFLOAT3 newEye{};
	XMFLOAT3 newLook{};
	XMStoreFloat3(&newEye, newEyeVec);
	XMStoreFloat3(&newLook, newLookVec);

	m_Camera->SetEyeLookUp(newEye, newLook, m_Camera->GetUp());
	m_Transform->SetPosition(newEye);
}

void CameraLogicComponent2::MoveInBound(float deltaTime)
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene || !scene->GetServices().Has<InputManager>())
		return;

	auto& input = scene->GetServices().Get<InputManager>();

	const float moveAmount = m_MoveSpeed * deltaTime;
	float deltaX = 0.0f;
	float deltaZ = 0.0f;

	if (input.IsKeyPressed('A'))
		deltaX -= moveAmount;
	if (input.IsKeyPressed('D'))
		deltaX += moveAmount;
	// 요청: W/S 동작 교체 (W: Z 증가, S: Z 감소)
	if (input.IsKeyPressed('W'))
		deltaZ += moveAmount;
	if (input.IsKeyPressed('S'))
		deltaZ -= moveAmount;

	if (deltaX == 0.0f && deltaZ == 0.0f)
		return;

	const XMFLOAT3 currentEye = m_Camera->GetEye();
	const XMFLOAT3 currentLook = m_Camera->GetLook();

	XMFLOAT3 newEye = currentEye;
	XMFLOAT3 newLook = currentLook;

	const float minX = (std::min)(m_MinX, m_MaxX);
	const float maxX = (std::max)(m_MinX, m_MaxX);
	const float minZ = (std::min)(m_MinZ, m_MaxZ);
	const float maxZ = (std::max)(m_MinZ, m_MaxZ);

	newEye.x = std::clamp(currentEye.x + deltaX, minX, maxX);
	newEye.z = std::clamp(currentEye.z + deltaZ, minZ, maxZ);

	const float appliedDeltaX = newEye.x - currentEye.x;
	const float appliedDeltaZ = newEye.z - currentEye.z;

	newLook.x += appliedDeltaX;
	newLook.z += appliedDeltaZ;

	m_Camera->SetEyeLookUp(newEye, newLook, m_Camera->GetUp());
	m_Transform->SetPosition(newEye);
}
