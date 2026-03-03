#include "BoxColliderComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "TransformComponent.h"
#include "MeshComponent.h"
#include "AssetLoader.h"
#include <algorithm>
#include <cfloat>
#include <cmath>

REGISTER_COMPONENT(BoxColliderComponent)
REGISTER_PROPERTY(BoxColliderComponent, LocalMin)
REGISTER_PROPERTY(BoxColliderComponent, LocalMax)

void BoxColliderComponent::Start()
{
    TryInitializeFromMesh();
}

void BoxColliderComponent::Update(float deltaTime)
{
    if (!m_HasBounds)
    {
        TryInitializeFromMesh();
    }
}

void BoxColliderComponent::OnEvent(EventType type, const void* data)
{
}

void BoxColliderComponent::SetLocalBounds(const DirectX::XMFLOAT3& min, const DirectX::XMFLOAT3& max)
{
    m_LocalMin = min;
    m_LocalMax = max;
    m_HasBounds = true;
}

bool BoxColliderComponent::BuildWorldBounds(DirectX::XMFLOAT3& outMin, DirectX::XMFLOAT3& outMax) const
{
    if (!m_HasBounds)
        return false;
    
    auto* owner = GetOwner();
    if (!owner)
        return false;

    auto* transform = owner->GetComponent<TransformComponent>();
    if (!transform)
        return false;

    const auto world = DirectX::XMLoadFloat4x4(&transform->GetWorldMatrix());

    const DirectX::XMFLOAT3 localMin = m_LocalMin;
    const DirectX::XMFLOAT3 localMax = m_LocalMax;

	const DirectX::XMFLOAT3 corners[8] = {
		 { localMin.x, localMin.y, localMin.z },
		 { localMax.x, localMin.y, localMin.z },
		 { localMax.x, localMax.y, localMin.z },
		 { localMin.x, localMax.y, localMin.z },
		 { localMin.x, localMin.y, localMax.z },
		 { localMax.x, localMin.y, localMax.z },
		 { localMax.x, localMax.y, localMax.z },
		 { localMin.x, localMax.y, localMax.z }
	};

	DirectX::XMFLOAT3 worldCorner{};
	DirectX::XMFLOAT3 minOut{ FLT_MAX, FLT_MAX, FLT_MAX };
	DirectX::XMFLOAT3 maxOut{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (const auto& corner : corners)
	{
		const auto v = DirectX::XMLoadFloat3(&corner);
		const auto transformed = DirectX::XMVector3TransformCoord(v, world);
		DirectX::XMStoreFloat3(&worldCorner, transformed);

		minOut.x = min(minOut.x, worldCorner.x);
		minOut.y = min(minOut.y, worldCorner.y);
		minOut.z = min(minOut.z, worldCorner.z);

		maxOut.x = max(maxOut.x, worldCorner.x);
		maxOut.y = max(maxOut.y, worldCorner.y);
		maxOut.z = max(maxOut.z, worldCorner.z);
	}

	outMin = minOut;
	outMax = maxOut;
	return true;
}

bool BoxColliderComponent::IntersectsRay(const DirectX::XMFLOAT3& rayOrigin, const DirectX::XMFLOAT3& rayDir, float& outT) const
{
	if (!m_IsActive)
	{
		return false;
	}


	DirectX::XMFLOAT3 boundsMin{};
	DirectX::XMFLOAT3 boundsMax{};
	if (!BuildWorldBounds(boundsMin, boundsMax))
		return false;

	float tMin = 0.0f;
	float tMax = FLT_MAX;

	const float origin[3] = { rayOrigin.x, rayOrigin.y, rayOrigin.z };
	const float dir[3]    = { rayDir.x,    rayDir.y,    rayDir.z	};
	const float minB[3]   = { boundsMin.x, boundsMin.y, boundsMin.z };
	const float maxB[3]   = { boundsMax.x, boundsMax.y, boundsMax.z };

	for (int axis = 0; axis < 3; ++axis)
	{
		if (std::abs(dir[axis]) < 1e-6f)
		{
			if (origin[axis] < minB[axis] || origin[axis] > maxB[axis])
				return false;
			continue;
		}

		const float invD = 1.0f / dir[axis];
		float t0 = (minB[axis] - origin[axis]) * invD;
		float t1 = (maxB[axis] - origin[axis]) * invD;
		if (t0 > t1)
			std::swap(t0, t1);

		tMin = max(tMin, t0);
		tMax = min(tMax, t1);
		if (tMax < tMin)
			return false;
	}

	outT = tMin;
	return true;
}

void BoxColliderComponent::TryInitializeFromMesh()
{
	auto* owner = GetOwner();
	if (!owner)
		return;

	const auto meshComponents = owner->GetComponentsDerived<MeshComponent>();
	if (meshComponents.empty())
		return;

	auto* meshComp = meshComponents.front();
	if (!meshComp)
		return;

	const auto handle = meshComp->GetMeshHandle();
	if (!handle.IsValid())
		return;

	auto* loader = AssetLoader::GetActive();
	if (!loader)
		return;

	const auto* meshData = loader->GetMeshes().Get(handle);
	if (!meshData)
		return;

	m_LocalMin = meshData->boundsMin;
	m_LocalMax = meshData->boundsMax;
	m_HasBounds = true;
}
