#include "WaterRiseComponent.h"

#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "GameManager.h"
#include "TransformComponent.h"

#include <algorithm>

REGISTER_COMPONENT(WaterRiseComponent)
REGISTER_PROPERTY(WaterRiseComponent, MinY)
REGISTER_PROPERTY(WaterRiseComponent, MaxY)
REGISTER_PROPERTY(WaterRiseComponent, RiseDurationSeconds)
REGISTER_PROPERTY(WaterRiseComponent, ApplyMinYOnStart)
REGISTER_PROPERTY(WaterRiseComponent, Loop)
REGISTER_PROPERTY(WaterRiseComponent, Enabled)
REGISTER_PROPERTY_READONLY(WaterRiseComponent, ElapsedSeconds)
REGISTER_PROPERTY_READONLY(WaterRiseComponent, Finished)

void WaterRiseComponent::Start()
{
	m_ElapsedSeconds = 0.0f;
	m_Finished = false;

	if (m_ApplyMinYOnStart)
	{
		ApplyCurrentY();
	}
}

void WaterRiseComponent::Update(float deltaTime)
{
	if (!m_Enabled)
	{
		return;
	}

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	auto* gameManager = scene ? scene->GetGameManager() : nullptr;
	if (gameManager)
	{
		const Phase phase = gameManager->GetPhase();
		const bool pauseInCombat = (phase == Phase::TurnBasedCombat);
		const bool pauseInExplorationBlocked = (phase == Phase::ExplorationLoop
			&& !gameManager->IsExplorationInputAllowed());
		if (pauseInCombat || pauseInExplorationBlocked)
		{
			return;
		}
	}

	if (m_Finished && !m_Loop)
	{
		return;
	}

	const float safeDuration = max(0.001f, m_RiseDurationSeconds);
	m_ElapsedSeconds += deltaTime;

	if (m_ElapsedSeconds >= safeDuration)
	{
		if (m_Loop)
		{
			while (m_ElapsedSeconds >= safeDuration)
			{
				m_ElapsedSeconds -= safeDuration;
			}
			m_Finished = false;
		}
		else
		{
			m_ElapsedSeconds = safeDuration;
			m_Finished = true;
		}
	}

	ApplyCurrentY();
}

void WaterRiseComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void WaterRiseComponent::ResetRise()
{
	m_ElapsedSeconds = 0.0f;
	m_Finished = false;
	ApplyCurrentY();
}

void WaterRiseComponent::ApplyCurrentY()
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

	const float minY = min(m_MinY, m_MaxY);
	const float maxY = max(m_MinY, m_MaxY);
	const float safeDuration = max(0.001f, m_RiseDurationSeconds);
	const float t = std::clamp(m_ElapsedSeconds / safeDuration, 0.0f, 1.0f);
	const float currentY = minY + (maxY - minY) * t;

	XMFLOAT3 pos = transform->GetPosition();
	pos.y = currentY;
	transform->SetPosition(pos);
}