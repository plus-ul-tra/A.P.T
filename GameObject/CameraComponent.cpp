#include "CameraComponent.h"
#include "TransformComponent.h"
#include "Object.h"
#include "ReflectionMacro.h"
REGISTER_COMPONENT(CameraComponent);
REGISTER_PROPERTY(CameraComponent, Eye)
REGISTER_PROPERTY(CameraComponent, Look)
REGISTER_PROPERTY(CameraComponent, Up)
REGISTER_PROPERTY(CameraComponent, NearZ)
REGISTER_PROPERTY(CameraComponent, FarZ)
REGISTER_PROPERTY(CameraComponent, Viewport)
REGISTER_PROPERTY(CameraComponent, ProjectionMode)
REGISTER_PROPERTY(CameraComponent, Perspective)
REGISTER_PROPERTY(CameraComponent, Ortho)
REGISTER_PROPERTY(CameraComponent, OrthoOffCenter)


void CameraComponent::RebuildViewIfDirty()
{
	if (!m_ViewDirty) return;

	XMStoreFloat4x4(&m_View, LookAtLH(m_Eye, m_Look, m_Up));

	m_ViewDirty = false;
}

void CameraComponent::RebuildProjIfDirty()
{
	if (!m_ProjDirty) return;

	switch (m_Mode)
	{
	case ProjectionMode::Perspective:
		XMStoreFloat4x4(&m_Proj, PerspectiveLH(m_Perspective.Fov, m_Perspective.Aspect, m_NearZ, m_FarZ));
		break;
	case ProjectionMode::Orthographic:
		XMStoreFloat4x4(&m_Proj, OrthographicLH(m_Ortho.Width, m_Ortho.Height, m_NearZ, m_FarZ));
		break;
	case ProjectionMode::OrthoOffCenter:
		XMStoreFloat4x4(&m_Proj, OrthographicOffCenterLH(m_OrthoOffCenter.Left, m_OrthoOffCenter.Right, m_OrthoOffCenter.Bottom, m_OrthoOffCenter.Top, m_NearZ, m_FarZ));
		break;
	}
	
	m_ProjDirty = false;
}

CameraComponent::CameraComponent(float viewportW, float viewportH,
	float param1, float param2, float nearZ, float farZ, ProjectionMode mode) : m_Viewport({ viewportW, viewportH }), m_NearZ(nearZ), m_FarZ(farZ), m_Mode(mode)
{
	switch (mode)
	{
	case ProjectionMode::Perspective:
		SetPerspectiveProj(param1, param2, nearZ, farZ);
		break;
	case ProjectionMode::Orthographic:
		SetOrthoProj(param1, param2, nearZ, farZ);
		break;
	}
}


CameraComponent::CameraComponent(float viewportW, float viewportH,
	float left, float right, float bottom, float top, float nearZ, float farZ) : m_Viewport({ viewportW, viewportH }), m_OrthoOffCenter({ left, right, bottom, top }), m_NearZ(nearZ), m_FarZ(farZ)
{
	SetOrthoOffCenterProj(left, right, bottom, top, nearZ, farZ);
}

void CameraComponent::Update(float deltaTime)
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* transform = owner->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	const XMFLOAT4X4& world = transform->GetWorldMatrix();
	XMVECTOR scale = XMVectorZero();
	XMVECTOR rotation = XMVectorZero();
	XMVECTOR translation = XMVectorZero();

	if (!XMMatrixDecompose(&scale, &rotation, &translation, XMLoadFloat4x4(&world)))
	{
		return;
	}

	XMFLOAT3 eye{};
	XMStoreFloat3(&eye, translation);

	const XMVECTOR forwardVec = XMVector3Rotate(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
	const XMVECTOR upVec = XMVector3Rotate(XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f), rotation);

	XMFLOAT3 forward{};
	XMFLOAT3 up{};
	XMStoreFloat3(&forward, forwardVec);
	XMStoreFloat3(&up, upVec);

	const XMFLOAT3 look = Add(eye, forward);

	m_Eye = eye;
	m_Look = look;
	m_Up = up;
	m_ViewDirty = true;
}

void CameraComponent::OnEvent(EventType type, const void* data)
{
}


void CameraComponent::Deserialize(const nlohmann::json& j)
{
	Component::Deserialize(j);
	m_ViewDirty = true;
	m_ProjDirty = true;
}

XMFLOAT4X4 CameraComponent::GetViewMatrix()
{
	RebuildViewIfDirty(); // 필요할 때만 계산
	return m_View;
}

XMFLOAT4X4 CameraComponent::GetProjMatrix()
{
	RebuildProjIfDirty(); // 필요할 때만 계산
	return m_Proj;
}

void CameraComponent::SetProjectionMode(const ProjectionMode& mode)
{
	if (m_Mode == mode)
	{
		return;
	}

	m_Mode = mode;
	if (m_Mode == ProjectionMode::Perspective && m_Viewport.Height > 0.0f)
	{
		m_Perspective.Aspect = m_Viewport.Width / m_Viewport.Height;
	}
	m_ProjDirty = true;
}

void CameraComponent::SetEyeLookUp(const XMFLOAT3& eye, const XMFLOAT3& look, const XMFLOAT3& up)
{
	m_Eye  = eye;
	m_Look = look;
	m_Up   = up;
	m_ViewDirty = true;
	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* transform = owner->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	const XMVECTOR eyeVec = XMLoadFloat3(&eye);
	const XMVECTOR lookVec = XMLoadFloat3(&look);
	const XMVECTOR upVec = XMLoadFloat3(&up);
	XMVECTOR forwardVec = XMVectorSubtract(lookVec, eyeVec);
	const float forwardLength = XMVectorGetX(XMVector3Length(forwardVec));

	if (forwardLength < 0.0001f)
	{
		forwardVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
	}
	else
	{
		forwardVec = XMVector3Normalize(forwardVec);
	}

	XMVECTOR rightVec = XMVector3Cross(upVec, forwardVec);
	const float rightLength = XMVectorGetX(XMVector3Length(rightVec));

	if (rightLength < 0.0001f)
	{
		rightVec = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	}
	else
	{
		rightVec = XMVector3Normalize(rightVec);
	}

	const XMVECTOR orthoUpVec = XMVector3Normalize(XMVector3Cross(forwardVec, rightVec));

	const XMMATRIX rotationMatrix = XMMATRIX(
		rightVec,
		orthoUpVec,
		forwardVec,
		XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f)
	);

	const XMVECTOR rotationQuat = XMQuaternionNormalize(XMQuaternionRotationMatrix(rotationMatrix));
	XMFLOAT4 rotation{};
	XMStoreFloat4(&rotation, rotationQuat);

	transform->SetPosition(eye);
	transform->SetRotation(rotation);
}



void CameraComponent::SetPerspectiveProj(const float fov, const float aspect, const float nearZ, const float farZ)
{
	m_Mode = ProjectionMode::Perspective;
	m_Perspective.Fov = fov;
	m_Perspective.Aspect = aspect;
	m_NearZ = nearZ;
	m_FarZ = farZ;
	m_ProjDirty = true;
}

void CameraComponent::SetOrthoProj(const float width, const float height, const float nearZ, const float farZ)
{
	m_Mode = ProjectionMode::Orthographic;
	m_Ortho.Width = width;
	m_Ortho.Height = height;
	m_NearZ = nearZ;
	m_FarZ = farZ;
	m_ProjDirty = true;
}

void CameraComponent::SetOrthoOffCenterProj(const float left, const float right, const float bottom, const float top, const float nearZ, const float farZ)
{
	m_Mode = ProjectionMode::OrthoOffCenter;
	m_OrthoOffCenter.Left = left;
	m_OrthoOffCenter.Right = right;
	m_OrthoOffCenter.Bottom = bottom;
	m_OrthoOffCenter.Top = top;
	m_NearZ = nearZ;
	m_FarZ = farZ;
	m_ProjDirty = true;
}

void CameraComponent::SetViewport(const Viewport& viewport)
{
	m_Viewport = viewport;
	if (m_Mode == ProjectionMode::Perspective && viewport.Height > 0.0f)
	{
		m_Perspective.Aspect = viewport.Width / viewport.Height;
		m_ProjDirty = true;
	}
}
