
#include "CameraLogicComponent.h"
#include "CameraComponent.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include <algorithm>
#include <limits>
// Game Main Camera Logic
REGISTER_COMPONENT(CameraLogicComponent)
REGISTER_PROPERTY(CameraLogicComponent, MaxZoom)
REGISTER_PROPERTY(CameraLogicComponent, MinZoom)
REGISTER_PROPERTY(CameraLogicComponent, XOffset)
REGISTER_PROPERTY(CameraLogicComponent, ZOffset)
REGISTER_PROPERTY(CameraLogicComponent, Threshold)

CameraLogicComponent::~CameraLogicComponent()
{
	GetEventDispatcher().RemoveListener(EventType::MouseWheelUp, this);
	GetEventDispatcher().RemoveListener(EventType::MouseWheelDown, this);
}

// Player 등록 필요. 
// Player의 GetWorldPos로 월드 기준 좌표를 Look으로 설정
void CameraLogicComponent::Start()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	GetEventDispatcher().AddListener(EventType::MouseWheelUp, this);
	GetEventDispatcher().AddListener(EventType::MouseWheelDown, this);

	auto*  trans = owner->GetComponent<TransformComponent>();
	auto* cam = owner->GetComponent<CameraComponent>();
	if (!trans || !cam) { return; }
	m_Transform = trans;
	m_Camera = cam;

	auto* scene = owner->GetScene();
	for (const auto& [name, gameObject] : scene->GetGameObjects())
	{
		(void)name;
		if (!gameObject)
			continue;

		if (gameObject->GetComponent<PlayerComponent>())
		{
			if (auto* playerTransform = gameObject->GetComponent<TransformComponent>())
			{
				m_PlayerTransform = playerTransform;
				m_PreviousPlayerPos = playerTransform->GetWorldPos();
				m_HasPreviousPlayerPos = true;
				break;
			}
		}
	}
}

void CameraLogicComponent::Update(float deltaTime)
{
	if (!m_Camera)
		return;
	if (!m_PlayerTransform)
		return;
	CamZoom();
	CamFollow(deltaTime);


}

void CameraLogicComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::MouseWheelUp)
	{
		m_PendingZoomInput += 1.0f;
	}
	else if (type == EventType::MouseWheelDown) {

		m_PendingZoomInput -= 1.0f;
	}
}

void CameraLogicComponent::CamZoom()
{
	if (m_PendingZoomInput == 0.0f)
	{
		return;
	}

	const XMFLOAT3 currentEye = m_Camera->GetEye();
	const XMFLOAT3 currentLook = m_Camera->GetLook();
	XMFLOAT3 zoomPivot = currentLook;
	if (m_PlayerTransform)
	{
		//targetLook = m_PlayerTransform->GetWorldPos();
		zoomPivot = m_PlayerTransform->GetWorldPos();
	}

	XMVECTOR eyeVec = XMLoadFloat3(&currentEye);
	XMVECTOR pivotVec = XMLoadFloat3(&zoomPivot);
	XMVECTOR toEye = XMVectorSubtract(eyeVec, pivotVec);
	const float currentDistance = XMVectorGetX(XMVector3Length(toEye));
	XMVECTOR dir = XMVectorSet(0.0f, 0.0f, -1.0f, 0.0f);
	if (currentDistance > 0.0001f)
	{
		dir = XMVectorScale(toEye, 1.0f / currentDistance);
	}

	float desiredDistance = currentDistance;
	desiredDistance = currentDistance - (m_PendingZoomInput * m_ZoomSpeed);
	const float minZoom = m_MinZoom;
	const float maxZoom = m_MaxZoom;
	if (maxZoom > 0.0f || minZoom > 0.0f)
	{
		const float clampedMin = (std::min)(minZoom, maxZoom > 0.0f ? maxZoom : minZoom);
		const float clampedMax = (std::max)(maxZoom, clampedMin);
		desiredDistance = std::clamp(desiredDistance, clampedMin, clampedMax);
	}
	m_PendingZoomInput = 0.0f;
	//XMVECTOR newEyeVec = XMVectorAdd(lookVec, XMVectorScale(dir, desiredDistance));
	XMVECTOR newEyeVec = XMVectorAdd(pivotVec, XMVectorScale(dir, desiredDistance));
	XMFLOAT3 newEye{};
	XMStoreFloat3(&newEye, newEyeVec);
	const XMVECTOR deltaEyeVec = XMVectorSubtract(newEyeVec, eyeVec);
	XMFLOAT3 newLook{};
	XMStoreFloat3(&newLook, XMVectorAdd(XMLoadFloat3(&currentLook), deltaEyeVec));
	m_Camera->SetEyeLookUp(newEye, newLook, m_Camera->GetUp());

}

void CameraLogicComponent::CamFollow(float deltaTime)
{
	const XMFLOAT3 playerPos = m_PlayerTransform->GetWorldPos();

	if (!m_HasPreviousPlayerPos)
	{
		m_PreviousPlayerPos = playerPos;
		m_HasPreviousPlayerPos = true;
	}

	const float playerMoveDeltaX = playerPos.x - m_PreviousPlayerPos.x;
	const float playerMoveDeltaZ = playerPos.z - m_PreviousPlayerPos.z;
	const bool isMovingNegativeX = (playerMoveDeltaX < 0.0f);
	const bool isMovingNegativeZ = (playerMoveDeltaZ < 0.0f);

	CamFollowX(deltaTime, isMovingNegativeX);
	CamFollowZ(deltaTime, isMovingNegativeZ);

	m_PreviousPlayerPos = playerPos;
}

void CameraLogicComponent::CamFollowX(float deltaTime, bool isMovingNegativeX)
{
	const XMFLOAT3 currentEye = m_Camera->GetEye();
	const XMFLOAT3 currentLook = m_Camera->GetLook();
	const XMFLOAT3 playerPos = m_PlayerTransform->GetWorldPos();

	
	const float playerDeltaFromLookX = playerPos.x - currentLook.x;
	const float directionalOffsetX = (playerDeltaFromLookX >= 0.0f) ? m_XOffset : -m_XOffset;
	const float targetLookX = playerPos.x + directionalOffsetX;
	const float deltaX = targetLookX - currentLook.x;

	if (!isMovingNegativeX && std::abs(deltaX) < m_FollowThreshold)
	{
		return;
	}

	const float maxStep = 2.0f * deltaTime;
	const float stepX = std::clamp(deltaX, -maxStep, maxStep);

	XMFLOAT3 newEye = currentEye;
	XMFLOAT3 newLook = currentLook;
	newEye.x += stepX;
	newLook.x += stepX;

	m_Camera->SetEyeLookUp(newEye, newLook, m_Camera->GetUp());
}

void CameraLogicComponent::CamFollowZ(float deltaTime, bool isMovingNegativeZ)
{
	const XMFLOAT3 currentEye = m_Camera->GetEye();
	const XMFLOAT3 currentLook = m_Camera->GetLook();
	const XMFLOAT3 playerPos = m_PlayerTransform->GetWorldPos();

	const float playerDeltaFromLookZ = playerPos.z - currentLook.z;
	const float directionalOffsetZ = (playerDeltaFromLookZ >= 0.0f) ? m_ZOffset : -m_ZOffset;
	const float targetLookZ = playerPos.z + directionalOffsetZ;
	const float deltaZ = targetLookZ - currentLook.z;

	if (!isMovingNegativeZ && std::abs(deltaZ) < m_FollowThreshold)
	{
		return;
	}

	const float maxStep = 2.0f * deltaTime;
	const float stepZ = std::clamp(deltaZ, -maxStep, maxStep);

	XMFLOAT3 newEye = currentEye;
	XMFLOAT3 newLook = currentLook;
	newEye.z += stepZ;
	newLook.z += stepZ;

	m_Camera->SetEyeLookUp(newEye, newLook, m_Camera->GetUp());
}

