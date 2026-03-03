#include "FloodSystemComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "GameManager.h"
#include "TransformComponent.h"
#include "PlayerComponent.h"
#include <algorithm>
#include <cmath>

REGISTER_COMPONENT(FloodSystemComponent)
REGISTER_PROPERTY(FloodSystemComponent, TurnTimeLimitSeconds)
REGISTER_PROPERTY(FloodSystemComponent, WaterRisePerSecond)
REGISTER_PROPERTY(FloodSystemComponent, RiseIntervalSeconds)
REGISTER_PROPERTY(FloodSystemComponent, RiseStepAmount)
REGISTER_PROPERTY(FloodSystemComponent, CorrectionMin)
REGISTER_PROPERTY(FloodSystemComponent, CorrectionMax)
REGISTER_PROPERTY(FloodSystemComponent, CorrectionDistanceMax)
REGISTER_PROPERTY(FloodSystemComponent, ShopCorrectionMultiplier)
REGISTER_PROPERTY_READONLY(FloodSystemComponent, WaterLevel)
REGISTER_PROPERTY_READONLY(FloodSystemComponent, TurnElapsed)
REGISTER_PROPERTY_READONLY(FloodSystemComponent, GameOver)

void FloodSystemComponent::Start()
{
	m_TurnElapsed = 0.0f;
	m_IsGameOver  = false;
}

void FloodSystemComponent::Update(float deltaTime)
{
	if (m_IsGameOver)
		return;

	if (!ShouldAdvance())
	{
		return;
	}

	m_TurnElapsed += deltaTime;
	float correction = ComputeCorrectionFactor();
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (gameManager && gameManager->IsShopInputAllowed())
	{
		correction *= m_ShopCorrectionMultiplier;
	}

	const float stepInterval = max(0.01f, m_RiseIntervalSeconds * correction);
	if (m_TurnElapsed >= stepInterval)
	{
		m_WaterLevel += m_RiseStepAmount * correction;
		m_TurnElapsed = 0.0f;
	}

	const float playerThreshold = GetPlayerFloodHeightThreshold();
	if (playerThreshold > 0.0f && m_WaterLevel >= playerThreshold)
	{
		MarkGameOver();
	}
}

void FloodSystemComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

const float FloodSystemComponent::GetTurnRemaining() const
{
	float correction = ComputeCorrectionFactor();
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (gameManager && gameManager->IsShopInputAllowed())
	{
		correction *= m_ShopCorrectionMultiplier;
	}

	const float stepInterval = max(0.01f, m_RiseIntervalSeconds * correction);
	const float remaining = stepInterval - m_TurnElapsed;
	return remaining > 0.0f ? remaining : 0.0f;
}

void FloodSystemComponent::MarkGameOver()
{
	if (m_IsGameOver)
	{
		return;
	}

	m_IsGameOver = true;
	GetEventDispatcher().Dispatch(EventType::GameOver, nullptr);
}

float FloodSystemComponent::ComputeCorrectionFactor() const
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return 1.0f;
	}

	Object* playerObject = nullptr;
	for (const auto& [name, object] : scene->GetGameObjects())
	{
		(void)name;
		if (object && object->GetComponent<PlayerComponent>())
		{
			playerObject = object.get();
			break;
		}
	}
	if (!playerObject)
	{
		return 1.0f;
	}

	auto* waterTransform = owner ? owner->GetComponent<TransformComponent>() : nullptr;
	auto* playerTransform = playerObject->GetComponent<TransformComponent>();
	if (!waterTransform || !playerTransform)
	{
		return 1.0f;
	}

	const auto waterPos = waterTransform->GetPosition();
	const auto playerPos = playerTransform->GetPosition();
	const float dx = waterPos.x - playerPos.x;
	const float dz = waterPos.z - playerPos.z;
	const float distance = std::sqrt(dx * dx + dz * dz);

	const float maxDistance = max(0.01f, m_CorrectionDistanceMax);
	const float normalized  = min(distance / maxDistance, 1.0f);
	const float minFactor   = min(m_CorrectionMin, m_CorrectionMax);
	const float maxFactor   = max(m_CorrectionMin, m_CorrectionMax);
	return minFactor + (maxFactor - minFactor) * normalized;
}

bool FloodSystemComponent::ShouldAdvance() const
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;

	return gameManager
		&& (gameManager->IsExplorationInputAllowed()
			|| gameManager->IsCombatInputAllowed()
			|| gameManager->IsShopInputAllowed());
}

float FloodSystemComponent::GetPlayerFloodHeightThreshold() const
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return -1.0f;
	}

	Object* playerObject = nullptr;
	for (const auto& [name, object] : scene->GetGameObjects())
	{
		(void)name;
		if (object && object->GetComponent<PlayerComponent>())
		{
			playerObject = object.get();
			break;
		}
	}

	auto* playerTransform = playerObject ? playerObject->GetComponent<TransformComponent>() : nullptr;
	if (!playerTransform)
	{
		return -1.0f;
	}

	constexpr float kPlayerHeight = 1.8f;
	constexpr float kHeightScale = 1.1547f;
	return playerTransform->GetPosition().y + (kPlayerHeight * kHeightScale);
}
