#include "TransformComponent.h"
#include "Event.h"
#include <cassert>
#include "ReflectionMacro.h"
REGISTER_COMPONENT(TransformComponent)  //컴포넌트 등록
// 컴포넌트 property 등록 **이름 주의**
REGISTER_PROPERTY(TransformComponent, Position)
REGISTER_PROPERTY_READONLY(TransformComponent, WorldPos)
REGISTER_PROPERTY(TransformComponent, Rotation)
REGISTER_PROPERTY(TransformComponent, Scale)


TransformComponent::~TransformComponent()
{
	if (m_Parent)
	{
		m_Parent->RemoveChild(this);
		m_Parent = nullptr;
	}

	for (auto* child : m_Children)
	{
		if (child)
		{
			child->m_Parent = nullptr;
		}
	}
	m_Children.clear();
}

void TransformComponent::SetParent(TransformComponent* newParent)
{
	assert(newParent != this);
	assert(m_Parent == nullptr);

	m_Parent = newParent;
	m_Parent->AddChild(this);

	SetDirty();
}

void TransformComponent::SetParentKeepLocal(TransformComponent* newParent)
{
	assert(newParent != this);
	assert(m_Parent == nullptr);

	m_Parent = newParent;
	m_Parent->m_Children.push_back(this);

	SetDirty();
}

void TransformComponent::SetParentKeepWorld(TransformComponent* newParent)
{
	assert(newParent != this);

	// 1. 부모 변경 전에 월드 저장
	XMFLOAT4X4 worldBefore = GetWorldMatrix();

	// 2. 기존 부모 제거
	if (m_Parent)
		DetachFromParentKeepWorld();

	// 3. 부모 설정
	m_Parent = newParent;
	newParent->m_Children.push_back(this);

	// 4. Local = World * inverse(ParentWorld)
	XMFLOAT4X4 parentInvWorld = newParent->GetInverseWorldMatrix();
	XMFLOAT4X4 local = Mul(worldBefore, parentInvWorld);

	XMFLOAT4X4 localNoPivot = RemovePivot(local, GetPivotPoint());
	DecomposeMatrix(localNoPivot, m_Position, m_Rotation, m_Scale);

	SetDirty();
}

void TransformComponent::DetachFromParentKeepWorld()
{
	if (!m_Parent) return;

	XMFLOAT4X4 worldBefore = GetWorldMatrix();

	auto* parent = m_Parent;
	parent->m_Children.erase(
		std::remove(parent->m_Children.begin(), parent->m_Children.end(), this),
		parent->m_Children.end()
	);

	m_Parent = nullptr;

	XMFLOAT4X4 noPivot = RemovePivot(worldBefore, GetPivotPoint());
	DecomposeMatrix(noPivot, m_Position, m_Rotation, m_Scale);

	SetDirty();
}

void TransformComponent::DetachFromParentKeepLocal()
{
	if (m_Parent == nullptr) return;

	auto* parent = m_Parent;
	m_Parent = nullptr;

	parent->m_Children.erase(
		std::remove(parent->m_Children.begin(), parent->m_Children.end(), this),
		parent->m_Children.end()
	);

	SetDirty();
}

void TransformComponent::DetachFromParent()
{
	if (m_Parent == nullptr) return;

	m_Parent->RemoveChild(this);

	m_Parent = nullptr;

	SetDirty();
}

void TransformComponent::AddChild(TransformComponent* child)
{
	//XMFLOAT4X4 childLocalTM = child->GetLocalMatrix();
	//childLocalTM = Mul(childLocalTM, GetInverseWorldMatrix());
	XMFLOAT4X4 childWorldTM = child->GetWorldMatrix();
	XMFLOAT4X4 childLocalTM = Mul(childWorldTM, GetInverseWorldMatrix());
	XMFLOAT4X4 mNoPivot = RemovePivot(childLocalTM, child->GetPivotPoint());
	DecomposeMatrix(mNoPivot, child->m_Position, child->m_Rotation, child->m_Scale);

	m_Children.push_back(child);
}

void TransformComponent::RemoveChild(TransformComponent* child)
{
	//XMFLOAT4X4 childLocalTM = child->GetLocalMatrix();
	//XMFLOAT4X4 childLocalTM = child->GetWorldMatrix();childLocalTM = Mul(childLocalTM, m_WorldMatrix);
	XMFLOAT4X4 childLocalTM = child->GetWorldMatrix();
	XMFLOAT4X4 mNoPivot = RemovePivot(childLocalTM, child->GetPivotPoint());
	DecomposeMatrix(mNoPivot, child->m_Position, child->m_Rotation, child->m_Scale);

	m_Children.erase(
		std::remove(m_Children.begin(), m_Children.end(), child),
			m_Children.end()
	);
}

void TransformComponent::SetRotationEuler(const XMFLOAT3& rot)
{	// 쿼터니언 -> 오일러 변환
	// 변환 순서
	// degree -> radian -> Quater

	XMFLOAT3 rotRad{ XMConvertToRadians(rot.x),XMConvertToRadians(rot.y),XMConvertToRadians(rot.z) };
		
	XMFLOAT4 result{};
	XMStoreFloat4(&result, XMQuaternionRotationRollPitchYaw(rotRad.x, rotRad.y, rotRad.z));
	m_Rotation = result;
	SetDirty();
}

const XMFLOAT3& TransformComponent::GetWorldPos() 
{
	// 부모 밑에 있을 때 World Position을 외부에서 사용해야 할때, 부모 없으면 오류방지를 위해 부모기준 계산된 pos return
	
	if (m_Parent)
	{
		XMMATRIX local = CreateTRS(m_Position, m_Rotation, m_Scale);
		XMMATRIX parentWorld = XMLoadFloat4x4(&m_Parent->GetWorldMatrix());
		XMMATRIX world = XMMatrixMultiply(local, parentWorld);

		XMFLOAT4X4 w;
		XMStoreFloat4x4(&w, world);
		m_WorldPos = XMFLOAT3{ w._41, w._42, w._43 };
		return m_WorldPos;
	}
	// 부모 없으면 local == world
	return m_Position;

}

void TransformComponent::Translate(const XMFLOAT3& delta)
{
	AddInPlace(m_Position, delta);

	SetDirty();
}

void TransformComponent::Translate(const float x, const float y, const float z)
{
	XMFLOAT3 delta = { x, y, z };
	AddInPlace(m_Position, delta);

	SetDirty();
}

void TransformComponent::Rotate(float pitch, float yaw, float roll)
{
	XMFLOAT4 quat = QuatFromEular(pitch, yaw, roll);
	MulInPlace(m_Rotation, quat);
}

void TransformComponent::Rotate(const XMFLOAT4& rot)
{
	MulInPlace(m_Rotation, rot);
}

XMFLOAT3 TransformComponent::GetForward() const
{
	XMFLOAT3 forward;
	XMVECTOR vForward = XMVector3Rotate(
		XMVectorSet(0.f, 0.f, 1.f, 0.f),
		XMLoadFloat4(&m_Rotation)
	);
	XMStoreFloat3(&forward, vForward);

	return forward;
}

const XMFLOAT4X4& TransformComponent::GetWorldMatrix() 
{
	if (m_IsDirty) UpdateMatrices();
	return m_WorldMatrix;
}

const XMFLOAT4X4& TransformComponent::GetLocalMatrix()
{
	if (m_IsDirty) UpdateMatrices();
	return m_LocalMatrix;
}

XMFLOAT4X4 TransformComponent::GetInverseWorldMatrix()
{
	XMFLOAT4X4 inv = Inverse(m_WorldMatrix);
	return inv;
}

void TransformComponent::SetPivotPreset(TransformPivotPreset preset, const XMFLOAT3& size)
{
	switch (preset)
	{
	case TransformPivotPreset::BottomCenter:
		m_Pivot = { 0, -1, 0 };
		break;
	case TransformPivotPreset::Center:
		m_Pivot = { 0, 0, 0 };
		break;
	case TransformPivotPreset::BoundingPivot:
		break;
	}
}

void TransformComponent::Update(float deltaTime)
{

}

void TransformComponent::OnEvent(EventType type, const void* data)
{

}


void TransformComponent::Deserialize(const nlohmann::json& j)
{
	Component::Deserialize(j);
	SetDirty();
	UpdateMatrices();
}

void TransformComponent::UpdateMatrices()
{
	XMStoreFloat4x4(&m_LocalMatrix, CreateTRS(m_Position, m_Rotation, m_Scale));

	if (m_Parent)
	{
		//m_WorldMatrix = Mul( m_Parent->m_WorldMatrix, m_LocalMatrix); //☆
		m_WorldMatrix = Mul(m_LocalMatrix, m_Parent->m_WorldMatrix);
	}
	else
	{
		m_WorldMatrix = m_LocalMatrix;
	}

	m_IsDirty = false;
}
