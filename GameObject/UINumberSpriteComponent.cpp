#include "UINumberSpriteComponent.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"
#include "UIObject.h"
#include "UIImageComponent.h"
#include "PlayerComponent.h"
#include "EnemyComponent.h"
#include "PlayerStatComponent.h"
#include "EnemyStatComponent.h"
#include "GameManager.h"
#include <algorithm>
#include <cmath>

REGISTER_UI_COMPONENT(UINumberSpriteComponent)
REGISTER_PROPERTY(UINumberSpriteComponent, Enabled)
REGISTER_PROPERTY(UINumberSpriteComponent, Value)
REGISTER_PROPERTY(UINumberSpriteComponent, LeadingZero)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitTextures)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitSpacing)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitOffsets)
REGISTER_PROPERTY(UINumberSpriteComponent, PerDigitAdvance)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitObjectNames)
REGISTER_PROPERTY(UINumberSpriteComponent, UseOwnerAsSingleTarget)
REGISTER_PROPERTY(UINumberSpriteComponent, FixedDigitCount)
REGISTER_PROPERTY(UINumberSpriteComponent, DigitTintColor)
REGISTER_PROPERTY(UINumberSpriteComponent, PositiveSignObjectName)
REGISTER_PROPERTY(UINumberSpriteComponent, NegativeSignObjectName)
REGISTER_PROPERTY(UINumberSpriteComponent, AutoValueSource)
REGISTER_PROPERTY(UINumberSpriteComponent, AutoValueActorId)
REGISTER_PROPERTY(UINumberSpriteComponent, UseAsCombatPopup)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupObjectNames)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupTrackActorId)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupRiseDistance)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupLifetime)
REGISTER_PROPERTY(UINumberSpriteComponent, PopupFadeOutTime)
REGISTER_PROPERTY(UINumberSpriteComponent, DamageTint)
REGISTER_PROPERTY(UINumberSpriteComponent, HealTint)
REGISTER_PROPERTY(UINumberSpriteComponent, DealTint)
REGISTER_PROPERTY(UINumberSpriteComponent, CriticalTint)
REGISTER_PROPERTY(UINumberSpriteComponent, GoldTint)
REGISTER_PROPERTY(UINumberSpriteComponent, MissTint)
REGISTER_PROPERTY(UINumberSpriteComponent, UseMissTexture)
REGISTER_PROPERTY(UINumberSpriteComponent, MissTexture)

UINumberSpriteComponent::~UINumberSpriteComponent()
{
	if ((m_ListenerRegistered || m_GoldListenerRegistered || m_StatListenerRegistered || m_EnemyHoverListenerRegistered) && m_Dispatcher && m_Dispatcher->IsAlive())
	{
		if (m_Dispatcher->FindListeners(EventType::CombatNumberPopup))
		{
			m_Dispatcher->RemoveListener(EventType::CombatNumberPopup, this);
		}
		if (m_GoldListenerRegistered && m_Dispatcher->FindListeners(EventType::GoldAcquired))
		{
			m_Dispatcher->RemoveListener(EventType::GoldAcquired, this);
		}
		if (m_Dispatcher->FindListeners(EventType::PlayerStatChanged))
		{
			m_Dispatcher->RemoveListener(EventType::PlayerStatChanged, this);
		}
		if (m_EnemyHoverListenerRegistered && m_Dispatcher->FindListeners(EventType::EnemyHovered))
		{
			m_Dispatcher->RemoveListener(EventType::EnemyHovered, this);
		}
		m_ListenerRegistered = false;
		m_GoldListenerRegistered = false;
		m_StatListenerRegistered = false;
		m_EnemyHoverListenerRegistered = false;
	}
}

void UINumberSpriteComponent::Start()
{
	m_RuntimeBindingsReady = false;
	m_ListenerRegistered = false;
	m_GoldListenerRegistered = false;
	m_StatListenerRegistered = false;
	m_EnemyHoverListenerRegistered = false;
	m_PopupPoolDirty = true;
	m_ValueDirty = true;
	m_DigitTargets.clear();
	m_BaseDigitBounds.clear();
	m_DisplayMissVisual = false;
}

void UINumberSpriteComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
	(void)deltaTime;

	if (!m_Enabled)
	{
		return;
	}

	if (!TryPrepareRuntimeBindings())
	{
		return;
	}

	if (m_DigitTargets.empty())
	{
		EnsureDigitTargetsResolved();
	}
	TryInitializePopupPool();

	TickPopups(deltaTime);
	TryUpdateValueFromAutoSource();

	if (!m_ValueDirty)
	{
		return;
	}

	ApplyValue();
	m_ValueDirty = false;
}

void UINumberSpriteComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);

	if (!m_Enabled)
	{
		return;
	}

	if (type == EventType::GoldAcquired && data)
	{
		const auto* payload = static_cast<const Events::GoldAcquiredEvent*>(data);
		if (m_UseAsCombatPopup && payload && payload->amount > 0)
		{
			ShowPopup(payload->amount, m_GoldTint, false);
		}

		if (m_AutoValueSource == 5 && payload)
		{
			SetValue(payload->total);
		}
		return;
	}

	if (type == EventType::PlayerStatChanged)
	{
		if (m_AutoValueSource != 0)
		{
			TryUpdateValueFromAutoSource();
			RefreshVisuals();
		}
		return;
	}

	if (type == EventType::EnemyHovered)
	{
		if (m_AutoValueSource == 3 || m_AutoValueSource == 4)
		{
			const auto* payload = static_cast<const Events::EnemyHoveredEvent*>(data);
			const int hoveredActorId = payload ? payload->actorId : 0;
			const bool hasHoveredEnemy = payload ? payload->hasEnemy : (hoveredActorId != 0);
			if (m_AutoValueActorId != hoveredActorId)
			{
				m_AutoValueActorId = hoveredActorId;
				m_ValueDirty = true;
			}

			if (!hasHoveredEnemy)
			{
				SetValue(0);
			}
			else
			{
				if (payload)
				{
					if (m_AutoValueSource == 3)
					{
						SetValue(payload->currentHp);
					}
					else
					{
						SetValue((std::max)(1, payload->maxHp));
					}
				}
				else
				{
					TryUpdateValueFromAutoSource();
				}
			}
			RefreshVisuals();
		}
		return;
	}

	if (type != EventType::CombatNumberPopup || !data)
	{
		return;
	}

	const auto* payload = static_cast<const Events::CombatNumberPopupEvent*>(data);
	if (!payload)
	{
		return;
	}

	if (m_AutoValueSource == 24 || m_AutoValueSource == 25)
	{
		auto resolvePlayerActorId = [this]() -> int
			{
				auto* scene = GetScene();
				if (!scene)
				{
					return 0;
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
						return player->GetActorId();
					}
				}
				return 0;
			};

		const int playerActorId = resolvePlayerActorId();
		const int deltaAbs = std::abs(payload->hpDelta);
		if (!payload->isMiss && deltaAbs > 0)
		{
			if (m_AutoValueSource == 24)
			{
				if (playerActorId != 0 && payload->instigatorActorId == playerActorId && payload->targetActorId != playerActorId)
				{
					SetValue(deltaAbs);
				}
			}
			else
			{
				if (playerActorId != 0 && payload->targetActorId == playerActorId)
				{
					SetValue(deltaAbs);
				}
			}
		}
	}

	if (!m_UseAsCombatPopup)
	{
		return;
	}

	if (m_PopupTrackActorId != 0 && payload->targetActorId != m_PopupTrackActorId)
	{
		return;
	}

	const int value = std::abs(payload->hpDelta);
	if (!payload->isMiss && value <= 0)
	{
		return;
	}

	DirectX::XMFLOAT4 tint = m_DamageTint;
	if (payload->isMiss)
	{
		tint = m_MissTint;
	}
	else if (payload->isCritical)
	{
		tint = m_CriticalTint;
	}
	else if (payload->hpDelta > 0)
	{
		tint = m_HealTint;
	}
	else if (payload->instigatorActorId == m_PopupTrackActorId)
	{
		tint = m_DealTint;
	}

	ShowPopup(payload->isMiss ? 0 : value, tint, payload->isMiss);
}

void UINumberSpriteComponent::SetEnabled(const bool& enabled)
{
	if (m_Enabled == enabled)
	{
		return;
	}

	m_Enabled = enabled;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetValue(const int& value)
{
	if (m_Value == value)
	{
		return;
	}

	m_Value = value;
	m_ValueDirty = true;
	RefreshVisuals();
}

void UINumberSpriteComponent::SetLeadingZero(const bool& leadingZero)
{
	if (m_LeadingZero == leadingZero)
	{
		return;
	}

	m_LeadingZero = leadingZero;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitTextureHandle(int digit, const TextureHandle& handle)
{
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return;
	}

	m_DigitTextures[static_cast<size_t>(digit)] = handle;
	m_ValueDirty = true;
}

const TextureHandle& UINumberSpriteComponent::GetDigitTextureHandle(int digit) const
{
	static TextureHandle invalid = TextureHandle::Invalid();
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return invalid;
	}
	return m_DigitTextures[static_cast<size_t>(digit)];
}

void UINumberSpriteComponent::SetDigitTextures(const std::array<TextureHandle, 10>& textures)
{
	m_DigitTextures = textures;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitSpacing(const float& spacing)
{
	m_DigitSpacing = spacing;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitOffsets(std::vector<UIAnchor> offsets)
{
	m_DigitOffsets = std::move(offsets);
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitObjectNames(std::vector<std::string> names)
{
	m_DigitObjectNames = std::move(names);
	m_DigitTargets.clear();
	m_BaseDigitBounds.clear();
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetUseOwnerAsSingleTarget(const bool& useOwnerTarget)
{
	if (m_UseOwnerAsSingleTarget == useOwnerTarget)
	{
		return;
	}

	m_UseOwnerAsSingleTarget = useOwnerTarget;
	m_DigitTargets.clear();
	m_BaseDigitBounds.clear();
	m_ValueDirty = true;
}


void UINumberSpriteComponent::SetPerDigitAdvance(const std::array<float, 10>& offsets)
{
	m_PerDigitAdvance = offsets;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetFixedDigitCount(const int& count)
{
	m_FixedDigitCount = (std::max)(0, count);
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetDigitTintColor(const DirectX::XMFLOAT4& color)
{
	m_DigitTintColor = color;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetPositiveSignObjectName(const std::string& name)
{
	m_PositiveSignObjectName = name;
	m_ValueDirty = true;
	if (m_RuntimeBindingsReady)
	{
		RefreshVisuals();
	}
}

void UINumberSpriteComponent::SetNegativeSignObjectName(const std::string& name)
{
	m_NegativeSignObjectName = name;
	m_ValueDirty = true;
	if (m_RuntimeBindingsReady)
	{
		RefreshVisuals();
	}
}

void UINumberSpriteComponent::SetAutoValueSource(const int& source)
{
	const bool wasRuntimeBindingsReady = m_RuntimeBindingsReady;
	m_AutoValueSource = source;
	m_RuntimeBindingsReady = false;
	m_ValueDirty = true;

	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	m_Dispatcher = &GetEventDispatcher();
	if (m_Dispatcher && m_Dispatcher->IsAlive())
	{
		const bool shouldListenCombatPopup = (m_UseAsCombatPopup || m_AutoValueSource == 24 || m_AutoValueSource == 25);
		if (shouldListenCombatPopup && !m_ListenerRegistered)
		{
			m_Dispatcher->AddListener(EventType::CombatNumberPopup, this);
			m_ListenerRegistered = true;
		}
		else if (!shouldListenCombatPopup && m_ListenerRegistered)
		{
			if (m_Dispatcher->FindListeners(EventType::CombatNumberPopup))
			{
				m_Dispatcher->RemoveListener(EventType::CombatNumberPopup, this);
			}
			m_ListenerRegistered = false;
		}

		const bool shouldListenGold = (m_UseAsCombatPopup || m_AutoValueSource == 5);
		if (shouldListenGold && !m_GoldListenerRegistered)
		{
			m_Dispatcher->AddListener(EventType::GoldAcquired, this);
			m_GoldListenerRegistered = true;
		}
		else if (!shouldListenGold && m_GoldListenerRegistered)
		{
			if (m_Dispatcher->FindListeners(EventType::GoldAcquired))
			{
				m_Dispatcher->RemoveListener(EventType::GoldAcquired, this);
			}
			m_GoldListenerRegistered = false;
		}

		if (m_AutoValueSource != 0 && !m_StatListenerRegistered)
		{
			m_Dispatcher->AddListener(EventType::PlayerStatChanged, this);
			m_StatListenerRegistered = true;
		}
		else if (m_AutoValueSource == 0 && m_StatListenerRegistered)
		{
			if (m_Dispatcher->FindListeners(EventType::PlayerStatChanged))
			{
				m_Dispatcher->RemoveListener(EventType::PlayerStatChanged, this);
			}
			m_StatListenerRegistered = false;
		}

		const bool shouldListenEnemyHover = (m_AutoValueSource == 3 || m_AutoValueSource == 4);
		if (shouldListenEnemyHover && !m_EnemyHoverListenerRegistered)
		{
			m_Dispatcher->AddListener(EventType::EnemyHovered, this);
			m_EnemyHoverListenerRegistered = true;
		}
		else if (!shouldListenEnemyHover && m_EnemyHoverListenerRegistered)
		{
			if (m_Dispatcher->FindListeners(EventType::EnemyHovered))
			{
				m_Dispatcher->RemoveListener(EventType::EnemyHovered, this);
			}
			m_EnemyHoverListenerRegistered = false;
		}
	}


	if (m_AutoValueSource != 3 && m_AutoValueSource != 4)
	{
		SetVisible(true);
	}

	if (wasRuntimeBindingsReady)
	{
		RefreshVisuals();
	}
}

void UINumberSpriteComponent::SetAutoValueActorId(const int& actorId)
{
	m_AutoValueActorId = actorId;
	m_ValueDirty = true;
	if (m_RuntimeBindingsReady)
	{
		RefreshVisuals();
	}
}

void UINumberSpriteComponent::SetUseAsCombatPopup(const bool& useAsPopup)
{
	if (m_UseAsCombatPopup == useAsPopup)
	{
		return;
	}

	m_UseAsCombatPopup = useAsPopup;
	m_RuntimeBindingsReady = false;
	m_PopupPoolDirty = true;

	if (!m_UseAsCombatPopup && m_ListenerRegistered && m_AutoValueSource != 24 && m_AutoValueSource != 25 && m_Dispatcher && m_Dispatcher->IsAlive())
	{
		if (m_Dispatcher->FindListeners(EventType::CombatNumberPopup))
		{
			m_Dispatcher->RemoveListener(EventType::CombatNumberPopup, this);
		}
		m_ListenerRegistered = false;
	}

	if (!m_UseAsCombatPopup && m_GoldListenerRegistered && m_AutoValueSource != 5 && m_Dispatcher && m_Dispatcher->IsAlive())
	{
		if (m_Dispatcher->FindListeners(EventType::GoldAcquired))
		{
			m_Dispatcher->RemoveListener(EventType::GoldAcquired, this);
		}
		m_GoldListenerRegistered = false;
	}

	if (m_AutoValueSource != 3 && m_AutoValueSource != 4 && m_EnemyHoverListenerRegistered && m_Dispatcher && m_Dispatcher->IsAlive())
	{
		if (m_Dispatcher->FindListeners(EventType::EnemyHovered))
		{
			m_Dispatcher->RemoveListener(EventType::EnemyHovered, this);
		}
		m_EnemyHoverListenerRegistered = false;
	}
}

void UINumberSpriteComponent::SetPopupObjectNames(std::vector<std::string> names)
{
	m_PopupObjectNames = std::move(names);
	m_PopupPoolDirty = true;
}

void UINumberSpriteComponent::SetPopupTrackActorId(const int& actorId)
{
	m_PopupTrackActorId = actorId;
}

void UINumberSpriteComponent::SetPopupRiseDistance(const float& rise)
{
	m_PopupRiseDistance = rise;
}

void UINumberSpriteComponent::SetPopupLifetime(const float& time)
{
	m_PopupLifetime = (std::max)(0.05f, time);
}

void UINumberSpriteComponent::SetPopupFadeOutTime(const float& time)
{
	m_PopupFadeOutTime = (std::max)(0.01f, time);
}

void UINumberSpriteComponent::SetDamageTint(const DirectX::XMFLOAT4& color)
{
	m_DamageTint = color;
}

void UINumberSpriteComponent::SetHealTint(const DirectX::XMFLOAT4& color)
{
	m_HealTint = color;
}

void UINumberSpriteComponent::SetDealTint(const DirectX::XMFLOAT4& color)
{
	m_DealTint = color;
}

void UINumberSpriteComponent::SetCriticalTint(const DirectX::XMFLOAT4& color)
{
	m_CriticalTint = color;
}

void UINumberSpriteComponent::SetGoldTint(const DirectX::XMFLOAT4& color)
{
	m_GoldTint = color;
}

void UINumberSpriteComponent::SetMissTint(const DirectX::XMFLOAT4& color)
{
	m_MissTint = color;
}

void UINumberSpriteComponent::SetUseMissTexture(const bool& useMissTexture)
{
	m_UseMissTexture = useMissTexture;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::SetMissTexture(const TextureHandle& texture)
{
	m_MissTexture = texture;
	m_ValueDirty = true;
}

void UINumberSpriteComponent::RefreshVisuals()
{
	m_ValueDirty = true;

	if (!m_Enabled)
	{
		return;
	}

	if (!TryPrepareRuntimeBindings())
	{
		return;
	}

	EnsureDigitTargetsResolved();
	if (m_DigitTargets.empty())
	{
		return;
	}

	ApplyValue();
	m_ValueDirty = false;
}

UIObject* UINumberSpriteComponent::FindUIObject(const std::string& name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	auto* uiManager = GetUIManager();
	if (!uiManager)
	{
		return nullptr;
	}

	const auto* scene = GetScene();
	const std::string sceneName = scene ? scene->GetName() : std::string{};
	const std::string currentScene = uiManager->GetCurrentScene();

	if (!sceneName.empty())
	{
		auto uiObject = uiManager->FindUIObject(sceneName, name);
		if (uiObject)
		{
			return uiObject.get();
		}
	}

	if (!currentScene.empty() && currentScene != sceneName)
	{
		auto uiObject = uiManager->FindUIObject(currentScene, name);
		if (uiObject)
		{
			return uiObject.get();
		}
	}

	return nullptr;
}

bool UINumberSpriteComponent::TryUpdateValueFromAutoSource()
{
	if (m_AutoValueSource == 0)
	{
		return false;
	}

	auto* scene = GetScene();
	if (!scene)
	{
		return false;
	}

	auto resolvePlayer = [scene]() -> PlayerComponent*
		{
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
		};

	auto resolveEnemyStat = [scene, this]() -> EnemyStatComponent*
		{
			if (m_AutoValueActorId == 0)
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
				auto* enemy = object->GetComponent<EnemyComponent>();
				if (!enemy || enemy->GetActorId() != m_AutoValueActorId)
				{
					continue;
				}
				return object->GetComponent<EnemyStatComponent>();
			}
			return nullptr;
		};

	int nextValue = m_Value;
	bool valid = false;
	switch (m_AutoValueSource)
	{
	case 1: // Player current HP
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetCurrentHP();
				valid = true;
			}
		}
		break;
	}
	case 2: // Player max HP
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				int floor = 1;
				if (scene->GetServices().Has<GameManager>())
				{
					floor = scene->GetServices().Get<GameManager>().GetCurrentFloor();
				}
				nextValue = stat->GetMaxHealthForFloor((std::max)(1, floor));
				valid = true;
			}
		}
		break;
	}
	case 3: // Enemy current HP
	{
		if (auto* stat = resolveEnemyStat())
		{
			nextValue = stat->GetCurrentHP();
			valid = true;
		}
		break;
	}
	case 4: // Enemy max HP
	{
		if (auto* stat = resolveEnemyStat())
		{
			nextValue = stat->GetInitialHP();
			valid = true;
		}
		break;
	}
	case 5: // Player gold
	{
		if (auto* player = resolvePlayer())
		{
			nextValue = player->GetMoney();
			valid = true;
		}
		break;
	}
	case 6: // Player health stat
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetHealth();
				valid = true;
			}
		}
		break;
	}
	case 7: // Player strength stat
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetStrength();
				valid = true;
			}
		}
		break;
	}
	case 8: // Player agility stat
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetAgility();
				valid = true;
			}
		}
		break;
	}
	case 9: // Player sense stat
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetSense();
				valid = true;
			}
		}
		break;
	}
	case 10: // Player skill stat
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetSkill();
				valid = true;
			}
		}
		break;
	}
	case 11: // Player defense
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetDefense();
				valid = true;
			}
		}
		break;
	}
	case 12: // Player initiative bonus
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetCalculatedAgilityModifier();
				valid = true;
			}
		}
		break;
	}
	case 13: // Player equipment defense bonus
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetEquipmentDefenseBonus();
				valid = true;
			}
		}
		break;
	}
	case 14: // Player health modifier
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetCalculatedHealthModifier();
				valid = true;
			}
		}
		break;
	}
	case 15: // Player strength modifier
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetCalculatedStrengthModifier();
				valid = true;
			}
		}
		break;
	}
	case 16: // Player agility modifier
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetCalculatedAgilityModifier();
				valid = true;
			}
		}
		break;
	}
	case 17: // Player sense modifier
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetCalculatedSenseModifier();
				valid = true;
			}
		}
		break;
	}
	case 18: // Player skill modifier
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetCalculatedSkillModifier();
				valid = true;
			}
		}
		break;
	}
	case 19: // Player equipment health bonus
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetEquipmentHealthBonus();
				valid = true;
			}
		}
		break;
	}
	case 20: // Player equipment strength bonus
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetEquipmentStrengthBonus();
				valid = true;
			}
		}
		break;
	}
	case 21: // Player equipment agility bonus
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetEquipmentAgilityBonus();
				valid = true;
			}
		}
		break;
	}
	case 22: // Player equipment sense bonus
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetEquipmentSenseBonus();
				valid = true;
			}
		}
		break;
	}
	case 23: // Player equipment skill bonus
	{
		if (auto* player = resolvePlayer())
		{
			if (auto* stat = player->GetOwner()->GetComponent<PlayerStatComponent>())
			{
				nextValue = stat->GetEquipmentSkillBonus();
				valid = true;
			}
		}
		break;
	}
	default:
		break;
	}

	if (!valid)
	{
		return false;
	}

	SetValue(nextValue);
	return true;
}

void UINumberSpriteComponent::SetDisplayMissVisual(bool displayMiss)
{
	if (m_DisplayMissVisual == displayMiss)
	{
		return;
	}

	m_DisplayMissVisual = displayMiss;
	m_ValueDirty = true;
}


bool UINumberSpriteComponent::EnsureDigitTargetsResolved()
{
	std::vector<UIObject*> resolvedTargets;

	auto* owner = dynamic_cast<UIObject*>(GetOwner());
	auto* scene = GetScene();
	auto* uiManager = GetUIManager();
	if (!owner || !scene || !uiManager)
	{
		return !m_DigitTargets.empty();
	}

	const std::vector<std::string>* digitNameSource = nullptr;
	if (!m_DigitObjectNames.empty())
	{
		digitNameSource = &m_DigitObjectNames;
	}
	else if (!m_UseAsCombatPopup && !m_PopupObjectNames.empty())
	{
		// Backward compatibility/editor safety:
		// If users accidentally filled PopupObjectNames while not using popup mode,
		// treat those names as digit targets so Value changes still render.
		digitNameSource = &m_PopupObjectNames;
	}

	if (digitNameSource)
	{
		resolvedTargets.reserve(digitNameSource->size());
		for (const auto& name : *digitNameSource)
		{
			auto* target = FindUIObject(name);
			if (!target || !target->GetComponent<UIImageComponent>())
			{
				continue;
			}
			resolvedTargets.push_back(target);
		}

		if (m_DigitTargets != resolvedTargets)
		{
			m_DigitTargets = std::move(resolvedTargets);
			m_BaseDigitBounds.clear();
			m_ValueDirty = true;
		}

		return !m_DigitTargets.empty();
	}

	if (m_UseOwnerAsSingleTarget && owner->GetComponent<UIImageComponent>())
	{
		if (m_DigitTargets.size() != 1 || m_DigitTargets.front() != owner)
		{
			m_DigitTargets = { owner };
			m_BaseDigitBounds.clear();
			m_ValueDirty = true;
		}
		return true;
	}


	const std::string& ownerName = owner->GetName();
	if (ownerName.empty())
	{
		if (owner->GetComponent<UIImageComponent>())
		{
			if (m_DigitTargets.size() != 1 || m_DigitTargets.front() != owner)
			{
				m_DigitTargets = { owner };
				m_BaseDigitBounds.clear();
				m_ValueDirty = true;
			}
			return true;
		}
		return !m_DigitTargets.empty();
	}

	auto& allScenes = uiManager->GetUIObjects();
	auto sceneIt = allScenes.find(scene->GetName());
	if (sceneIt == allScenes.end())
	{
		return !m_DigitTargets.empty();
	}

	auto& sceneObjects = sceneIt->second;
	std::vector<std::pair<std::string, UIObject*>> candidates;
	candidates.reserve(sceneObjects.size());
	for (const auto& [name, object] : sceneObjects)
	{
		if (!object || object->GetParentName() != ownerName)
		{
			continue;
		}

		auto* target = object.get();
		if (!target->GetComponent<UIImageComponent>())
		{
			continue;
		}

		candidates.emplace_back(name, target);
	}

	std::sort(candidates.begin(), candidates.end(),
		[](const auto& lhs, const auto& rhs)
		{
			const int lhsZ = lhs.second ? lhs.second->GetZOrder() : 0;
			const int rhsZ = rhs.second ? rhs.second->GetZOrder() : 0;
			if (lhsZ != rhsZ)
			{
				return lhsZ < rhsZ;
			}

			return lhs.first < rhs.first;
		});

	resolvedTargets.reserve(candidates.size());
	for (const auto& [name, target] : candidates)
	{
		(void)name;
		resolvedTargets.push_back(target);
	}

	if (resolvedTargets.empty() && owner->GetComponent<UIImageComponent>())
	{
		resolvedTargets.push_back(owner);
	}

	if (m_DigitTargets != resolvedTargets)
	{
		m_DigitTargets = std::move(resolvedTargets);
		m_BaseDigitBounds.clear();
		m_ValueDirty = true;
	}

	return !m_DigitTargets.empty();
}

void UINumberSpriteComponent::ApplyValue()
{
	if (m_DigitTargets.empty())
	{
		ApplySignObjectVisibility();
		return;
	}

	if (m_DisplayMissVisual)
	{
		for (size_t i = 0; i < m_DigitTargets.size(); ++i)
		{
			auto* target = m_DigitTargets[i];
			if (!target)
			{
				continue;
			}

			const bool visible = (i == 0);
			target->SetIsVisibleFromComponent(visible);
			if (!visible)
			{
				continue;
			}

			if (auto* image = target->GetComponent<UIImageComponent>())
			{
				if (m_UseMissTexture && m_MissTexture.IsValid())
				{
					image->SetTextureHandle(m_MissTexture);
				}
				image->SetTintColor(m_MissTint);
			}
		}
		return;
	}

	if (m_BaseDigitBounds.size() != m_DigitTargets.size())
	{
		m_BaseDigitBounds.resize(m_DigitTargets.size());
		for (size_t i = 0; i < m_DigitTargets.size(); ++i)
		{
			auto* target = m_DigitTargets[i];
			if (target && target->HasBounds())
			{
				m_BaseDigitBounds[i] = target->GetBounds();
			}
		}
	}

	const int absValue = std::abs(m_Value);
	std::string valueText = std::to_string(absValue);
	const int configuredDigits = (m_FixedDigitCount > 0) ? m_FixedDigitCount : static_cast<int>(m_DigitTargets.size());
	if (static_cast<int>(valueText.size()) < configuredDigits)
	{
		valueText = std::string(static_cast<size_t>(configuredDigits - valueText.size()), '0') + valueText;
	}


	const int digitCount = static_cast<int>(valueText.size());
	const int slotCount = static_cast<int>(m_DigitTargets.size());
	const int leadingSlots = max(0, slotCount - digitCount);
	float accumulatedOffset = 0.0f;

	for (int index = 0; index < slotCount; ++index)
	{
		auto* target = m_DigitTargets[static_cast<size_t>(index)];

		if (!target)
		{
			continue;
		}

		const bool isLeadingSlot = index < leadingSlots;
		if (isLeadingSlot && !m_LeadingZero)
		{
			target->SetIsVisibleFromComponent(false);
			continue;
		}

		const int valueIndex = isLeadingSlot ? 0 : index - leadingSlots;
		const char digitChar = valueText[static_cast<size_t>(valueIndex)];
		const int digitValue = std::clamp(static_cast<int>(digitChar - '0'), 0, 9);

		target->SetIsVisibleFromComponent(true);
		if (auto* image = target->GetComponent<UIImageComponent>())
		{
			const auto& handle = m_DigitTextures[static_cast<size_t>(digitValue)];
			if (handle.IsValid())
			{
				image->SetTextureHandle(handle);
			}
			image->SetTintColor(m_DigitTintColor);
		}

		if (target->HasBounds() && index < static_cast<int>(m_BaseDigitBounds.size()))
		{
			UIRect adjusted = m_BaseDigitBounds[static_cast<size_t>(index)];
			adjusted.x += accumulatedOffset;
			if (index < static_cast<int>(m_DigitOffsets.size()))
			{
				adjusted.x += m_DigitOffsets[static_cast<size_t>(index)].x;
				adjusted.y += m_DigitOffsets[static_cast<size_t>(index)].y;
			}
			target->SetBounds(adjusted);
		}

		accumulatedOffset += m_DigitSpacing + m_PerDigitAdvance[static_cast<size_t>(digitValue)];
	}
	ApplySignObjectVisibility();
}

void UINumberSpriteComponent::ApplySignObjectVisibility()
{
	if (!m_PositiveSignObjectName.empty())
	{
		if (auto* positive = FindUIObject(m_PositiveSignObjectName))
		{
			positive->SetIsVisibleFromComponent(m_Value >= 0);
		}
	}

	if (!m_NegativeSignObjectName.empty())
	{
		if (auto* negative = FindUIObject(m_NegativeSignObjectName))
		{
			negative->SetIsVisibleFromComponent(m_Value < 0);
		}
	}
}

bool UINumberSpriteComponent::TryPrepareRuntimeBindings()
{
	if (m_RuntimeBindingsReady)
	{
		return true;
	}

	auto* scene = GetScene();
	if (!scene)
	{
		return false;
	}

	auto* uiManager = GetUIManager();
	if (!uiManager)
	{
		return false;
	}

	if (m_UseAsCombatPopup && !ArePopupTargetsReady())
	{
		return false;
	}

	m_UIManager = uiManager;
	m_Dispatcher = &GetEventDispatcher();
	const bool shouldListenCombatPopup = (m_UseAsCombatPopup || m_AutoValueSource == 24 || m_AutoValueSource == 25);
	if (shouldListenCombatPopup && !m_ListenerRegistered)
	{
		m_Dispatcher->AddListener(EventType::CombatNumberPopup, this);
		m_ListenerRegistered = true;
	}
	else if (!shouldListenCombatPopup && m_ListenerRegistered)
	{
		if (m_Dispatcher->FindListeners(EventType::CombatNumberPopup))
		{
			m_Dispatcher->RemoveListener(EventType::CombatNumberPopup, this);
		}
		m_ListenerRegistered = false;
	}

	const bool shouldListenGold = (m_UseAsCombatPopup || m_AutoValueSource == 5);
	if (shouldListenGold && !m_GoldListenerRegistered)
	{
		m_Dispatcher->AddListener(EventType::GoldAcquired, this);
		m_GoldListenerRegistered = true;
	}
	else if (!shouldListenGold && m_GoldListenerRegistered)
	{
		if (m_Dispatcher->FindListeners(EventType::GoldAcquired))
		{
			m_Dispatcher->RemoveListener(EventType::GoldAcquired, this);
		}
		m_GoldListenerRegistered = false;
	}

	if (m_AutoValueSource != 0 && !m_StatListenerRegistered)
	{
		m_Dispatcher->AddListener(EventType::PlayerStatChanged, this);
		m_StatListenerRegistered = true;
	}
	else if (m_AutoValueSource == 0 && m_StatListenerRegistered)
	{
		if (m_Dispatcher->FindListeners(EventType::PlayerStatChanged))
		{
			m_Dispatcher->RemoveListener(EventType::PlayerStatChanged, this);
		}
		m_StatListenerRegistered = false;
	}


	const bool shouldListenEnemyHover = (m_AutoValueSource == 3 || m_AutoValueSource == 4);
	if (shouldListenEnemyHover && !m_EnemyHoverListenerRegistered)
	{
		m_Dispatcher->AddListener(EventType::EnemyHovered, this);
		m_EnemyHoverListenerRegistered = true;
	}
	else if (!shouldListenEnemyHover && m_EnemyHoverListenerRegistered)
	{
		if (m_Dispatcher->FindListeners(EventType::EnemyHovered))
		{
			m_Dispatcher->RemoveListener(EventType::EnemyHovered, this);
		}
		m_EnemyHoverListenerRegistered = false;
	}

	m_RuntimeBindingsReady = true;
	m_PopupPoolDirty = true;
	return true;
}

bool UINumberSpriteComponent::ArePopupTargetsReady() const
{
	if (m_PopupObjectNames.empty())
	{
		return true;
	}

	for (const auto& name : m_PopupObjectNames)
	{
		if (name.empty())
		{
			continue;
		}

		if (!FindUIObject(name))
		{
			return false;
		}
	}

	return true;
}

void UINumberSpriteComponent::TryInitializePopupPool()
{
	if (!m_PopupPoolDirty)
	{
		return;
	}

	auto* uiManager = GetUIManager();
	if (!uiManager)
	{
		return;
	}

	UpdatePopupPool();
	m_PopupPoolDirty = false;
}


void UINumberSpriteComponent::UpdatePopupPool()
{
	m_PopupStates.clear();
	m_PopupStates.reserve(m_PopupObjectNames.size());
	for (const auto& name : m_PopupObjectNames)
	{
		PopupState state{};
		state.objectName = name;
		if (auto* popup = FindUIObject(name))
		{
			popup->SetIsVisibleFromComponent(false);
			popup->SetOpacityFromComponent(1.0f);
			if (popup->HasBounds())
			{
				state.baseBounds = popup->GetBounds();
			}
		}
		m_PopupStates.push_back(state);
	}
}

void UINumberSpriteComponent::TickPopups(float deltaTime)
{
	for (auto& popup : m_PopupStates)
	{
		if (!popup.active)
		{
			continue;
		}

		auto* target = FindUIObject(popup.objectName);
		if (!target)
		{
			popup.active = false;
			continue;
		}

		popup.elapsed += deltaTime;
		const float t = min(1.0f, popup.elapsed / max(0.01f, m_PopupLifetime));
		UIRect moved = popup.baseBounds;
		moved.y -= m_PopupRiseDistance * t;
		target->SetBounds(moved);

		float opacity = 1.0f;
		const float fadeStart = max(0.0f, m_PopupLifetime - m_PopupFadeOutTime);
		if (popup.elapsed > fadeStart)
		{
			const float fadeT = (popup.elapsed - fadeStart) / max(0.01f, m_PopupFadeOutTime);
			opacity = max(0.0f, 1.0f - fadeT);
		}
		target->SetOpacityFromComponent(opacity);

		if (popup.elapsed >= m_PopupLifetime)
		{
			popup.active = false;
			target->SetIsVisibleFromComponent(false);
			target->SetOpacityFromComponent(1.0f);
			target->SetBounds(popup.baseBounds);
		}
	}
}

void UINumberSpriteComponent::ShowPopup(int value, const DirectX::XMFLOAT4& tint, bool isMiss)
{
	TryInitializePopupPool();

	auto pick = std::find_if(m_PopupStates.begin(), m_PopupStates.end(), [](const PopupState& state) { return !state.active; });
	if (pick == m_PopupStates.end() && !m_PopupStates.empty())
	{
		pick = std::min_element(m_PopupStates.begin(), m_PopupStates.end(), [](const PopupState& a, const PopupState& b)
			{
				return a.elapsed > b.elapsed;
			});
	}

	if (pick == m_PopupStates.end())
	{
		return;
	}

	auto* target = FindUIObject(pick->objectName);
	if (!target)
	{
		return;
	}

	if (target->HasBounds())
	{
		pick->baseBounds = target->GetBounds();
	}

	if (auto* popupNumber = target->GetComponent<UINumberSpriteComponent>())
	{
		popupNumber->SetDisplayMissVisual(isMiss);
		popupNumber->SetDigitTintColor(tint);
		popupNumber->SetValue(value);
		popupNumber->RefreshVisuals();
	}

	target->SetBounds(pick->baseBounds);
	target->SetOpacityFromComponent(1.0f);
	target->SetIsVisibleFromComponent(true);
	pick->elapsed = 0.0f;
	pick->active = true;
}