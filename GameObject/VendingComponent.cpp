#include "VendingComponent.h"
#include "Object.h"
#include "Scene.h"
#include "GameObject.h"
#include "ReflectionMacro.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "CameraObject.h"
#include "ServiceRegistry.h"
#include "InputManager.h"
#include "RayHelper.h"
#include <cmath>
#include <iostream>

REGISTER_COMPONENT(VendingComponent)
REGISTER_PROPERTY(VendingComponent, Distance)

//거리 계산용
float DistanceSquared(const XMFLOAT3& a, const XMFLOAT3& b)
{
	const float dx = a.x - b.x;
	const float dz = a.z - b.z;

	return dx * dx  + dz * dz;
}



VendingComponent::VendingComponent()
{
}

VendingComponent::~VendingComponent()
{
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
}

void VendingComponent::Start()
{
	m_Player = FindPlayerComponent();
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
}

void VendingComponent::Update(float deltaTime)
{
}


void VendingComponent::OnEvent(EventType type, const void* data)
{

	if (type != EventType::MouseLeftClick)
	{
		return;
	}

	auto* mouseData = static_cast<const Events::MouseState*>(data);
	if (!mouseData || mouseData->handled)
	{
		return;
	}

	if (Clicked(mouseData))
	{
		mouseData->handled = true;
	}
}


// bool Click 되었는지 // 플레이어와 거리조건 까지 고려된것

bool VendingComponent::Clicked(const Events::MouseState* mouseData)
{
	auto* owner = GetOwner();
	if (!owner || !mouseData)
	{
		return false;
	}
	if (!m_Player)
	{
		m_Player = FindPlayerComponent();
	}
	if (!IsClickedThisVending(*mouseData))
	{
		return false;
	}

	auto* vendingTransform = owner->GetComponent<TransformComponent>();
	if (!m_Player || !vendingTransform)
	{
		return false;
	}

	auto* playerObject = m_Player->GetOwner();
	auto* playerTransform = playerObject ? playerObject->GetComponent<TransformComponent>() : nullptr;
	if (!playerTransform)
	{
		return false;
	}

	const XMFLOAT3 vendingPos = vendingTransform->GetWorldPos();
	const XMFLOAT3 playerPos = playerTransform->GetWorldPos();
	const float distanceSq = DistanceSquared(vendingPos, playerPos);
	const float minDistanceSq = m_Distance * m_Distance;

	if (distanceSq < minDistanceSq)
	{
		std::cout << "[Vending] Clicked while player is far enough. distance="
			<< std::sqrt(distanceSq) << " threshold=" << m_Distance << std::endl;
		return true; 
		//Event -> UI 띄우기 ( 돈 템 사면 )
	}
}

PlayerComponent* VendingComponent::FindPlayerComponent() const
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return nullptr;
	}

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		(void)name;
		if (!object)
		{
			continue;
		}

		if (auto* player = object->GetComponent<PlayerComponent>())
		{
			return player;
		}
	}

	return nullptr;
}

bool VendingComponent::IsClickedThisVending(const Events::MouseState& mouseData) const
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	auto* collider = owner->GetComponent<BoxColliderComponent>();
	auto* scene = owner->GetScene();
	if (!collider || !scene)
	{
		return false;
	}

	auto camera = scene->GetGameCamera();
	if (!camera)
	{
		return false;
	}

	auto& services = scene->GetServices();
	if (!services.Has<InputManager>())
	{
		return false;
	}

	auto& input = services.Get<InputManager>();
	Ray pickRay{};
	if (!input.IsPointInViewport(mouseData.pos))
	{
		return false;
	}

	if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), mouseData, pickRay))
	{
		return false;
	}

	float hitT = 0.0f;
	return collider->IntersectsRay(pickRay.m_Pos, pickRay.m_Dir, hitT);
}
