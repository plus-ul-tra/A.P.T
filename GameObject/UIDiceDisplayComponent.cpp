#include "UIDiceDisplayComponent.h"
#include "Event.h"
#include "EventDispatcher.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"
#include "UIObject.h"
#include "UIImageComponent.h"
#include <algorithm>
#include <iostream>

REGISTER_UI_COMPONENT(UIDiceDisplayComponent)
REGISTER_PROPERTY(UIDiceDisplayComponent, Enabled)
REGISTER_PROPERTY(UIDiceDisplayComponent, DiceType)
REGISTER_PROPERTY(UIDiceDisplayComponent, Value)
REGISTER_PROPERTY(UIDiceDisplayComponent, LeadingZero)
REGISTER_PROPERTY(UIDiceDisplayComponent, DiceContext)
REGISTER_PROPERTY(UIDiceDisplayComponent, AutoShow)
REGISTER_PROPERTY(UIDiceDisplayComponent, UseSidesForType)
REGISTER_PROPERTY(UIDiceDisplayComponent, TensDigitObjectName)
REGISTER_PROPERTY(UIDiceDisplayComponent, OnesDigitObjectName)
REGISTER_PROPERTY(UIDiceDisplayComponent, DigitTextures)
REGISTER_PROPERTY(UIDiceDisplayComponent, Layouts)
REGISTER_PROPERTY(UIDiceDisplayComponent, ShowTotals)
REGISTER_PROPERTY(UIDiceDisplayComponent, ShowIndividuals)
REGISTER_PROPERTY(UIDiceDisplayComponent, UseRollFaces)
REGISTER_PROPERTY(UIDiceDisplayComponent, RollIndex)

UIDiceDisplayComponent::~UIDiceDisplayComponent()
{
	if (m_ListenersRegistered && m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::DiceRolled))
	{
		m_Dispatcher->RemoveListener(EventType::DiceRolled, this);
	}
	if (m_ListenersRegistered && m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::PlayerDiceUIReset))
	{
		m_Dispatcher->RemoveListener(EventType::PlayerDiceUIReset, this);
	}
	if (m_ListenersRegistered && m_Dispatcher && m_Dispatcher->IsAlive() && m_Dispatcher->FindListeners(EventType::PlayerDiceStatResolved))
	{
		m_Dispatcher->RemoveListener(EventType::PlayerDiceStatResolved, this);
	}
}

void UIDiceDisplayComponent::Start()
{
	m_RuntimeBindingsReady = false;
	m_ListenersRegistered = false;
	m_LayoutDirty = true;
	m_ValueDirty = true;
	m_DisabledVisibilityPending = !m_Enabled;
}

void UIDiceDisplayComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
	(void)deltaTime;

	if (!m_Enabled)
	{
		if (m_DisabledVisibilityPending && TryPrepareRuntimeBindings())
		{
			ApplyDisabledVisibility();
		}
		return;
	}

	if (!TryPrepareRuntimeBindings())
	{
		return;
	}

	if (!m_LayoutDirty && !m_ValueDirty)
	{
		return;
	}

	auto* owner = dynamic_cast<UIObject*>(GetOwner());
	if (!owner)
	{
		return;
	}

	auto* uiManager = GetUIManager();
	if (!uiManager)
	{
		return;
	}

	auto* tens = FindUIObject(m_TensDigitObjectName);
	auto* ones = FindUIObject(m_OnesDigitObjectName);
	const UIDiceLayout* layout = FindLayout();
	if (layout && m_LayoutDirty)
	{
		ApplyLayout(*layout, *owner, tens, ones);
		m_LayoutDirty = false;
	}

	if (m_ValueDirty)
	{
		ApplyValue(tens, ones);
		m_ValueDirty = false;
	}
}

void UIDiceDisplayComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);

	if (!m_Enabled)
	{
		return;
	}

	if (!TryPrepareRuntimeBindings())
	{
		return;
	}

	if (type == EventType::PlayerDiceUIReset)
	{
		std::cout << "[UIDiceDisplay] reset context=" << m_DiceContext << " -> value=0" << std::endl;
		SetValue(0);
		return;
	}

	if (type == EventType::PlayerDiceStatResolved)
	{
		const auto* payload = static_cast<const Events::DiceStatResolvedEvent*>(data);
		if (!payload)
		{
			return;
		}

		ApplyDiceStatResolvedEvent(*payload);
		return;
	}


	if (type != EventType::DiceRolled)
	{
		return;
	}

	const auto* payload = static_cast<const Events::DiceRollEvent*>(data);
	if (!payload)
	{
		return;
	}

	if (!m_DiceContext.empty() && payload->context != m_DiceContext)
	{
		const auto hasNumericSuffix = [](const std::string& context) -> bool
			{
				return ResolveNumericSuffix(context) >= 0;
			};

		const bool slotHasSuffix = hasNumericSuffix(m_DiceContext);
		const std::string baseSlotContext = ResolveBaseContext(m_DiceContext);
		const std::string basePayloadContext = ResolveBaseContext(payload->context);

		if (payload->isTotal)
		{
			if (slotHasSuffix)
			{
				if (payload->context != m_DiceContext)
				{
					return;
				}
			}
			else if (basePayloadContext != baseSlotContext)
			{
				return;
			}
		}
		else if (slotHasSuffix)
		{
			return;
		}
		else if (basePayloadContext != baseSlotContext)
		{
			return;
		}
	}

	std::cout << "[UIDiceDisplay] apply context=" << payload->context
		<< " value=" << payload->value
		<< " diceType=" << m_DiceType << std::endl;

	ApplyDiceEvent(*payload);
}

bool UIDiceDisplayComponent::TryPrepareRuntimeBindings()
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

	m_UIManager = uiManager;
	m_Dispatcher = &GetEventDispatcher();
	if (!m_ListenersRegistered)
	{
		m_Dispatcher->AddListener(EventType::DiceRolled, this);
		m_Dispatcher->AddListener(EventType::PlayerDiceUIReset, this);
		m_Dispatcher->AddListener(EventType::PlayerDiceStatResolved, this);
		m_ListenersRegistered = true;
	}

	m_RuntimeBindingsReady = true;
	return true;
}

void UIDiceDisplayComponent::ApplyDisabledVisibility()
{
	auto* tens = FindUIObject(m_TensDigitObjectName);
	auto* ones = FindUIObject(m_OnesDigitObjectName);
	if (tens)
	{
		tens->SetIsVisibleFromComponent(false);
	}
	if (ones)
	{
		ones->SetIsVisibleFromComponent(false);
	}
	m_DisabledVisibilityPending = false;
}


void UIDiceDisplayComponent::SetEnabled(const bool& enabled)
{
	if (m_Enabled == enabled)
	{
		return;
	}

	m_Enabled = enabled;
	m_LayoutDirty = true;
	m_ValueDirty = true;

	m_DisabledVisibilityPending = !m_Enabled;
}

void UIDiceDisplayComponent::SetDiceType(const std::string& type)
{
	if (m_DiceType == type)
	{
		return;
	}

	m_DiceType = type;
	m_LayoutDirty = true;
}

void UIDiceDisplayComponent::SetValue(const int& value)
{
	const int clamped = std::clamp(value, 0, 24);
	if (m_Value == clamped)
	{
		return;
	}

	m_Value = clamped;
	m_ValueDirty = true;
}

void UIDiceDisplayComponent::SetValueFromRollFace(const int& face)
{
	SetValue(face);
}

void UIDiceDisplayComponent::SetValueFromRollFaces(const std::vector<int>& faces, int index)
{
	if (index < 0 || index >= static_cast<int>(faces.size()))
	{
		return;
	}

	SetValue(faces[static_cast<size_t>(index)]);
}

void UIDiceDisplayComponent::SetLeadingZero(const bool& leadingZero)
{
	if (m_LeadingZero == leadingZero)
	{
		return;
	}

	m_LeadingZero = leadingZero;
	m_ValueDirty = true;
}

void UIDiceDisplayComponent::SetDiceContext(const std::string& context)
{
	if (m_DiceContext == context)
	{
		return;
	}

	m_DiceContext = context;
}

void UIDiceDisplayComponent::SetAutoShow(const bool& autoShow)
{
	m_AutoShow = autoShow;
}

void UIDiceDisplayComponent::SetUseSidesForType(const bool& useSidesForType)
{
	m_UseSidesForType = useSidesForType;
}

void UIDiceDisplayComponent::SetTensDigitObjectName(const std::string& name)
{
	if (m_TensDigitObjectName == name)
	{
		return;
	}

	m_TensDigitObjectName = name;
	m_LayoutDirty = true;
	m_ValueDirty = true;
}

void UIDiceDisplayComponent::SetOnesDigitObjectName(const std::string& name)
{
	if (m_OnesDigitObjectName == name)
	{
		return;
	}

	m_OnesDigitObjectName = name;
	m_LayoutDirty = true;
	m_ValueDirty = true;
}

void UIDiceDisplayComponent::SetDigitTextureHandle(int digit, const TextureHandle& handle)
{
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return;
	}

	m_DigitTextures[static_cast<size_t>(digit)] = handle;
	m_ValueDirty = true;
}

const TextureHandle& UIDiceDisplayComponent::GetDigitTextureHandle(int digit) const
{
	static TextureHandle invalid = TextureHandle::Invalid();
	if (digit < 0 || digit >= static_cast<int>(m_DigitTextures.size()))
	{
		return invalid;
	}
	return m_DigitTextures[static_cast<size_t>(digit)];
}

void UIDiceDisplayComponent::RefreshVisuals()
{
	m_LayoutDirty = true;
	m_ValueDirty = true;
}

UIObject* UIDiceDisplayComponent::FindUIObject(const std::string& name) const
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

void UIDiceDisplayComponent::SetDigitTextures(const std::array<TextureHandle, 10>& textures)
{
	m_DigitTextures = textures;
	m_ValueDirty = true;
}

void UIDiceDisplayComponent::SetLayouts(std::vector<UIDiceLayout> layouts)
{
	m_Layouts = std::move(layouts);
	m_LayoutDirty = true;
}

void UIDiceDisplayComponent::SetShowTotals(const bool& show)
{
	m_ShowTotals = show;
}

void UIDiceDisplayComponent::SetShowIndividuals(const bool& show)
{
	m_ShowIndividuals = show;
}

void UIDiceDisplayComponent::SetUseRollFaces(const bool& useFaces)
{
	m_UseRollFaces = useFaces;
}

void UIDiceDisplayComponent::SetRollIndex(const int& index)
{
	m_RollIndex = max(0, index);
}

const UIDiceLayout* UIDiceDisplayComponent::FindLayout() const
{
	if (m_Layouts.empty())
	{
		return nullptr;
	}

	if (m_DiceType.empty())
	{
		return &m_Layouts.front();
	}

	auto it = std::find_if(m_Layouts.begin(), m_Layouts.end(),
		[this](const UIDiceLayout& layout)
		{
			return layout.type == m_DiceType;
		});

	if (it == m_Layouts.end())
	{
		return &m_Layouts.front();
	}

	return &(*it);
}

void UIDiceDisplayComponent::ApplyLayout(const UIDiceLayout& layout, UIObject& owner, UIObject* tens, UIObject* ones)
{
	const auto resolveBounds = [&owner](const UIDiceDigitSlot& slot, const UIObject* target)
		{
			UIRect bounds = slot.bounds;
			if (target && target->HasBounds())
			{
				const auto& current = target->GetBounds();
				if (bounds.width == 0.0f && bounds.height == 0.0f)
				{
					bounds.width = current.width;
					bounds.height = current.height;
				}
			}
			if (slot.useParentOffset && owner.HasBounds())
			{
				const auto& parentBounds = owner.GetBounds();
				bounds.x += parentBounds.x;
				bounds.y += parentBounds.y;
			}
			return bounds;
		};

	if (auto* image = owner.GetComponent<UIImageComponent>())
	{
		if (layout.diceTexture.IsValid())
		{
			image->SetTextureHandle(layout.diceTexture);
		}
	}

	if (tens)
	{
		const UIRect bounds = resolveBounds(layout.tens, tens);
		tens->SetAnchorMin(layout.tens.anchor);
		tens->SetAnchorMax(layout.tens.anchor);
		tens->SetPivot(layout.tens.pivot);
		tens->SetBounds(bounds);
		tens->SetZOrderFromComponent(owner.GetZOrder() + 2);
	}

	if (ones)
	{
		const UIRect bounds = resolveBounds(layout.ones, ones);
		ones->SetAnchorMin(layout.ones.anchor);
		ones->SetAnchorMax(layout.ones.anchor);
		ones->SetPivot(layout.ones.pivot);
		ones->SetBounds(bounds);
		ones->SetZOrderFromComponent(owner.GetZOrder() + 2);
	}

	m_TensSlot = layout.tens;
	m_OnesSlot = layout.ones;
	m_TensSlot.bounds = resolveBounds(layout.tens, tens);
	m_OnesSlot.bounds = resolveBounds(layout.ones, ones);
	m_HasLayout = true;
}

void UIDiceDisplayComponent::ApplyValue(UIObject* tens, UIObject* ones)
{
	const int tensDigit = m_Value / 10;
	const int onesDigit = m_Value % 10;

	const auto applyDigitOffset = [this](UIObject* target, const UIDiceDigitSlot& slot, int digit)
		{
			if (!target || !m_HasLayout)
			{
				return;
			}

			UIRect adjusted = slot.bounds;
			if (digit >= 0 && digit < static_cast<int>(slot.digitOffsets.size()))
			{
				const auto& offset = slot.digitOffsets[static_cast<size_t>(digit)];
				adjusted.x += offset.x;
				adjusted.y += offset.y;
			}
			target->SetBounds(adjusted);
		};

	if (tens)
	{
		if (!m_LeadingZero && tensDigit == 0)
		{
			tens->SetIsVisibleFromComponent(false);
		}
		else
		{
			tens->SetIsVisibleFromComponent(true);
			applyDigitOffset(tens, m_TensSlot, tensDigit);
			if (auto* image = tens->GetComponent<UIImageComponent>())
			{
				const auto& handle = m_DigitTextures[static_cast<size_t>(tensDigit)];
				if (handle.IsValid())
				{
					image->SetTextureHandle(handle);
				}
			}
		}
	}

	if (ones)
	{
		ones->SetIsVisibleFromComponent(true);
		applyDigitOffset(ones, m_OnesSlot, onesDigit);
		if (auto* image = ones->GetComponent<UIImageComponent>())
		{
			const auto& handle = m_DigitTextures[static_cast<size_t>(onesDigit)];
			if (handle.IsValid())
			{
				image->SetTextureHandle(handle);
			}
		}
	}
}

void UIDiceDisplayComponent::ApplyDiceEvent(const Events::DiceRollEvent& payload)
{
	if (m_UseSidesForType && payload.diceSides > 0 && m_DiceType.empty())
	{
		SetDiceType("D" + std::to_string(payload.diceSides));
	}

	if (payload.isTotal)
	{
		if (!m_ShowTotals)
		{
			return;
		}

		if (m_UseRollFaces)
		{
			if (!payload.faces.empty() && m_RollIndex >= 0)
			{
				SetValueFromRollFaces(payload.faces, m_RollIndex);
			}
			else
			{
				// 롤 페이스 기반 슬롯은 PlayerDiceStatResolvedEvent에서 최종 face history를 반영한다.
				return;
			}
		}
		else
		{
			SetValue(payload.value);
		}
	}
	else
	{
		if (!m_ShowIndividuals)
		{
			return;
		}

		if (m_UseRollFaces && !payload.faces.empty())
		{
			if (m_RollIndex >= 0 && m_RollIndex < static_cast<int>(payload.faces.size()))
			{
				SetValueFromRollFaces(payload.faces, m_RollIndex);
			}
			else
			{
				SetValue(payload.value);
			}
		}
		else
		{
			SetValue(payload.value);
		}
	}

	if (m_AutoShow)
	{
		if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
		{
			owner->SetIsVisibleFromComponent(true);
		}
	}
}

int UIDiceDisplayComponent::ResolveNumericSuffix(const std::string& context)
{
	const auto pos = context.find_last_of('_');
	if (pos == std::string::npos)
	{
		return -1;
	}

	const auto suffix = context.substr(pos + 1);
	if (suffix.empty() || !std::all_of(suffix.begin(), suffix.end(), ::isdigit))
	{
		return -1;
	}

	return std::stoi(suffix);
}

std::string UIDiceDisplayComponent::ResolveBaseContext(const std::string& context)
{
	const auto suffix = ResolveNumericSuffix(context);
	if (suffix < 0)
	{
		return context;
	}

	const auto pos = context.find_last_of('_');
	if (pos == std::string::npos)
	{
		return context;
	}

	return context.substr(0, pos);
}

void UIDiceDisplayComponent::ApplyDiceStatResolvedEvent(const Events::DiceStatResolvedEvent& payload)
{
	if (m_DiceContext.empty())
	{
		return;
	}

	const std::string baseSlotContext = ResolveBaseContext(m_DiceContext);
	if (baseSlotContext != "InitiativeStatRoll")
	{
		return;
	}

	if (payload.facesHistory.empty())
	{
		return;
	}

	const int suffixIndex = ResolveNumericSuffix(m_DiceContext);
	const int faceIndex = max(0, m_RollIndex);

	const auto parseSidesFromDiceType = [](const std::string& type) -> int
		{
			if (type.size() < 2 || (type[0] != 'D' && type[0] != 'd'))
			{
				return -1;
			}
			const std::string number = type.substr(1);
			if (number.empty() || !std::all_of(number.begin(), number.end(), ::isdigit))
			{
				return -1;
			}
			return std::stoi(number);
		};

	const auto expectedCountForSides = [](const int sides) -> int
		{
			switch (sides)
			{
			case 12: return 2;
			case 8: return 3;
			case 6: return 4;
			case 4: return 6;
			default: return -1;
			}
		};

	int historyIndex = -1;

	// 1) 컨텍스트 suffix(_1/_2/_3)가 유효하면 우선 해당 history를 사용
	if (suffixIndex > 0)
	{
		const int requested = suffixIndex - 1;
		if (requested >= 0 && requested < static_cast<int>(payload.facesHistory.size()))
		{
			historyIndex = requested;
		}
	}

	// 2) suffix가 범위를 벗어나면 DiceType 기반으로 history를 매칭
	if (historyIndex < 0)
	{
		const int parsedSides = parseSidesFromDiceType(m_DiceType);
		const int expectedCount = expectedCountForSides(parsedSides);
		if (expectedCount > 0)
		{
			for (int i = 0; i < static_cast<int>(payload.facesHistory.size()); ++i)
			{
				if (static_cast<int>(payload.facesHistory[static_cast<size_t>(i)].size()) == expectedCount)
				{
					historyIndex = i;
					break;
				}
			}
		}
	}

	// 3) 그래도 못 찾으면 최신 history를 기본값으로 사용
	if (historyIndex < 0)
	{
		historyIndex = static_cast<int>(payload.facesHistory.size()) - 1;
	}

	if (historyIndex < 0 || historyIndex >= static_cast<int>(payload.facesHistory.size()))
	{
		std::cout << "[UIDiceDisplay] skip stat history context=" << m_DiceContext
			<< " historyIndex=" << historyIndex
			<< " historyCount=" << payload.facesHistory.size() << std::endl;
		return;
	}

	const auto* chosenFaces = &payload.facesHistory[static_cast<size_t>(historyIndex)];
	int chosenTotal = (historyIndex >= 0 && historyIndex < static_cast<int>(payload.totalsHistory.size()))
		? payload.totalsHistory[static_cast<size_t>(historyIndex)]
		: 0;

	const int slotSides = parseSidesFromDiceType(m_DiceType);
	const int expectedFacesCount = expectedCountForSides(slotSides > 0 ? slotSides : payload.diceSides);
	const int expectedSides = (slotSides > 0) ? slotSides : payload.diceSides;

	const auto isFacesCompatible = [&](const std::vector<int>& faces) -> bool
		{
			if (expectedFacesCount > 0 && static_cast<int>(faces.size()) != expectedFacesCount)
			{
				return false;
			}
			if (expectedSides > 0)
			{
				for (const int face : faces)
				{
					if (face < 1 || face > expectedSides)
					{
						return false;
					}
				}
			}
			return true;
		};

	if (!isFacesCompatible(*chosenFaces))
	{
		for (int i = 0; i < static_cast<int>(payload.facesHistory.size()); ++i)
		{
			if (!isFacesCompatible(payload.facesHistory[static_cast<size_t>(i)]))
			{
				continue;
			}
			chosenFaces = &payload.facesHistory[static_cast<size_t>(i)];
			chosenTotal = (i >= 0 && i < static_cast<int>(payload.totalsHistory.size()))
				? payload.totalsHistory[static_cast<size_t>(i)]
				: 0;
			historyIndex = i;
			std::cout << "[UIDiceDisplay] remap compatible history context=" << m_DiceContext
				<< " historyIndex=" << historyIndex
				<< " expectedCount=" << expectedFacesCount
				<< " expectedSides=" << expectedSides << std::endl;
			break;
		}
	}

	if (!isFacesCompatible(*chosenFaces) && !payload.faces.empty())
	{
		std::cout << "[UIDiceDisplay] fallback selected faces context=" << m_DiceContext
			<< " historyIndex=" << historyIndex
			<< " selectedCount=" << payload.faces.size()
			<< " selectedSides=" << payload.diceSides << std::endl;
		chosenFaces = &payload.faces;
		chosenTotal = payload.total;
	}

	const auto& rollFaces = *chosenFaces;
	const int rollTotal = chosenTotal;

	std::cout << "[UIDiceDisplay] apply stat history context=" << m_DiceContext
		<< " historyIndex=" << historyIndex
		<< " faceIndex=" << faceIndex
		<< " rollIndex=" << m_RollIndex
		<< " useRollFaces=" << m_UseRollFaces
		<< " facesCount=" << rollFaces.size()
		<< " rollTotal=" << rollTotal << std::endl;


	if (m_UseRollFaces)
	{
		if (faceIndex >= 0 && faceIndex < static_cast<int>(rollFaces.size()))
		{
			SetValue(rollFaces[static_cast<size_t>(faceIndex)]);
		}
		else
		{
			std::cout << "[UIDiceDisplay] skip face apply context=" << m_DiceContext
				<< " faceIndex=" << faceIndex
				<< " facesCount=" << rollFaces.size() << std::endl;
			SetValue(0);
		}
	}
	else
	{
		SetValue(rollTotal);
	}

	if (m_AutoShow)
	{
		if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
		{
			owner->SetIsVisibleFromComponent(true);
		}
	}
}
