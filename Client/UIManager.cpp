//Client Game
#include "pch.h"
#include "UIManager.h"
#include "Event.h"
#include "UIButtonComponent.h"
#include "UIImageComponent.h"
#include "HorizontalBox.h"
#include "MaterialComponent.h"
#include "UIProgressBarComponent.h"
#include "UITextComponent.h"
#include "UIFSMComponent.h"
#include "UISliderComponent.h"
#include "Border.h"
#include "ScaleBox.h"
#include "SizeBox.h"
#include <algorithm>
#include <unordered_set>

namespace
{
	void ApplySizeBoxOverrides(UIObject& uiObject)
	{
		auto* sizeBox = uiObject.GetComponent<SizeBox>();
		if (!sizeBox || !uiObject.HasBounds())
			return;

		UIRect bounds = uiObject.GetBounds();
		const UISize desired = sizeBox->GetDesiredSize(UISize{ bounds.width, bounds.height });
		if (desired.width != bounds.width || desired.height != bounds.height)
		{
			bounds.width = desired.width;
			bounds.height = desired.height;
			uiObject.SetBounds(bounds);
		}
	}

	void ApplyScaleBoxLayout(UIObject& uiObject, const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
	{
		auto* scaleBox = uiObject.GetComponent<ScaleBox>();
		if (!scaleBox || !uiObject.HasBounds())
			return;

		const std::string& parentName = uiObject.GetParentName();
		if (parentName.empty())
			return;

		auto itParent = uiMap.find(parentName);
		if (itParent == uiMap.end() || !itParent->second || !itParent->second->HasBounds())
			return;

		const UIRect parentBounds = itParent->second->GetBounds();
		UIRect bounds = uiObject.GetBounds();

		const UISize scaled = scaleBox->CalculateScaledSize(
			UISize{ parentBounds.width, parentBounds.height },
			UISize{ bounds.width, bounds.height });
		bounds.width = scaled.width;
		bounds.height = scaled.height;
		bounds.x = parentBounds.x + (parentBounds.width - scaled.width) * 0.5f;
		bounds.y = parentBounds.y + (parentBounds.height - scaled.height) * 0.5f;
		uiObject.SetBounds(bounds);
	}

	// 	void ApplyBorderLayout(UIObject& borderObject, const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
	// 	{
	// 		auto* border = borderObject.GetComponent<Border>();
	// 		if (!border || !borderObject.HasBounds())
	// 			return;
	// 
	// 		const std::string& parentName = borderObject.GetName();
	// 		const UIRect contentBounds = border->GetContentRect(borderObject.GetBounds());
	// 
	// 		for (const auto& [name, child] : uiMap)
	// 		{
	// 			if (!child || !child->HasBounds())
	// 			{
	// 				continue;
	// 			}
	// 
	// 			child->SetBounds(contentBounds);
	// 		}
	// 	} // 트리 구조도 아니고 시간 없어서 안쓸것같음

	void ApplyHorizontalBoxLayout(UIObject& uiObject, const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
	{
		auto* horizontalBox = uiObject.GetComponent<HorizontalBox>();
		if (!horizontalBox || !uiObject.HasBounds())
			return;

		auto& slots = horizontalBox->GetSlotsMutable();
		for (auto& slot : slots)
		{
			if (slot.child || slot.childName.empty())
			{
				continue;
			}

			auto itChild = uiMap.find(slot.childName);
			if (itChild != uiMap.end())
			{
				slot.child = itChild->second.get();
			}
		}

		const UIRect parentBounds = uiObject.GetBounds();
		const bool   parentVisible = uiObject.IsVisible();
		const int    parentZOrder = uiObject.GetZOrder();
		const UISize availableSize{ parentBounds.width, parentBounds.height };
		const auto arranged = horizontalBox->ArrangeChildren(parentBounds.x, parentBounds.y, availableSize);
		const size_t count = std::min(arranged.size(), slots.size());
		for (size_t i = 0; i < count; ++i)
		{
			if (slots[i].child)
			{
				slots[i].child->SetBounds(arranged[i]);
				slots[i].child->SetIsVisibleFromParent(parentVisible);
				slots[i].child->SetZOrder(parentZOrder + static_cast<int>(i) + 1);
			}
		}
	}

	UIRect ResolveWorldBounds(const std::string& name,
		const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
		std::unordered_map<std::string, UIRect>& cache,
		std::unordered_set<std::string>& visiting)
	{
		auto cached = cache.find(name);
		if (cached != cache.end())
		{
			return cached->second;
		}

		auto itObj = uiMap.find(name);
		if (itObj == uiMap.end() || !itObj->second)
		{
			return UIRect{};
		}

		if (!visiting.insert(name).second)
		{
			return itObj->second->GetBounds();
		}

		auto& uiObject = *itObj->second;
		UIRect local = uiObject.GetBounds();
		const std::string& parentName = uiObject.GetParentName();
		if (parentName.empty() || uiMap.find(parentName) == uiMap.end())
		{
			cache[name] = local;
			visiting.erase(name);
			return local;
		}

		UIRect parentBounds = ResolveWorldBounds(parentName, uiMap, cache, visiting);
		const UIAnchor anchorMin = uiObject.GetAnchorMin();
		const UIAnchor anchorMax = uiObject.GetAnchorMax();
		const UIAnchor pivot = uiObject.GetPivot();

		const float anchorLeft = parentBounds.x + parentBounds.width * anchorMin.x;
		const float anchorTop = parentBounds.y + parentBounds.height * anchorMin.y;
		const float anchorRight = parentBounds.x + parentBounds.width * anchorMax.x;
		const float anchorBottom = parentBounds.y + parentBounds.height * anchorMax.y;

		const bool stretchX = anchorMin.x != anchorMax.x;
		const bool stretchY = anchorMin.y != anchorMax.y;
		const float baseWidth = stretchX ? (anchorRight - anchorLeft) : 0.0f;
		const float baseHeight = stretchY ? (anchorBottom - anchorTop) : 0.0f;

		const float width = stretchX ? (baseWidth + local.width) : local.width;
		const float height = stretchY ? (baseHeight + local.height) : local.height;

		UIRect world;
		world.width = width;
		world.height = height;
		world.x = anchorLeft + local.x - width * pivot.x;
		world.y = anchorTop + local.y - height * pivot.y;

		cache[name] = world;
		visiting.erase(name);
		return world;
	}

	bool ResolveInheritedVisibility(const std::string& name,
								    const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
								    std::unordered_map<std::string, bool>& cache,
								    std::unordered_set<std::string>& visiting)
	{
		auto cached = cache.find(name);
		if (cached != cache.end())
		{
			return cached->second;
		}

		auto itObj = uiMap.find(name);
		if (itObj == uiMap.end() || !itObj->second)
		{
			return true;
		}

		if (!visiting.insert(name).second)
		{
			const bool fallback = itObj->second->IsLocallyVisible();
			cache[name] = fallback;
			return fallback;
		}

		const UIObject& uiObject = *itObj->second;
		bool visible = uiObject.IsLocallyVisible();
		const std::string& parentName = uiObject.GetParentName();
		if (!parentName.empty())
		{
			auto itParent = uiMap.find(parentName);
			if (itParent != uiMap.end() && itParent->second)
			{
				visible = visible && ResolveInheritedVisibility(parentName, uiMap, cache, visiting);
			}
		}

		cache[name] = visible;
		visiting.erase(name);
		return visible;
	}

	void ApplyAnchorLayout(const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
	{
		std::unordered_map<std::string, UIRect> cache;
		std::unordered_set<std::string> visiting;
		cache.reserve(uiMap.size());

		for (const auto& [name, uiObject] : uiMap)
		{
			if (!uiObject)
			{
				continue;
			}
			const UIRect world = ResolveWorldBounds(name, uiMap, cache, visiting);
			uiObject->SetBounds(world);
		}
	}

	void ApplyResolutionScale(const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
		const UISize& viewportSize,
		const UISize& referenceResolution,
		float& lastScale,
		UISize& lastOffset,
		bool& hasScaleState)
	{
		constexpr float kMinScale = 0.001f;

		if (referenceResolution.width <= 0.0f || referenceResolution.height <= 0.0f)
		{
			return;
		}

		if (viewportSize.width <= 0.0f || viewportSize.height <= 0.0f)
		{
			return;
		}

		const float scaleX = viewportSize.width / referenceResolution.width;
		const float scaleY = viewportSize.height / referenceResolution.height;
		const float uniformScale = std::min(scaleX, scaleY);
		if (uniformScale < kMinScale)
		{
			return;
		}

		const float offsetX = (viewportSize.width - referenceResolution.width * uniformScale) * 0.5f;
		const float offsetY = (viewportSize.height - referenceResolution.height * uniformScale) * 0.5f;

		const float previousScale = hasScaleState ? lastScale : 1.0f;
		const float previousOffsetX = hasScaleState ? lastOffset.width : 0.0f;
		const float previousOffsetY = hasScaleState ? lastOffset.height : 0.0f;
		const bool canUnscale = previousScale > 0.0f;

		for (const auto& [name, uiObject] : uiMap)
		{
			if (!uiObject || !uiObject->HasBounds())
			{
				continue;
			}

			UIRect bounds = uiObject->GetBounds();
			if (canUnscale)
			{
				bounds.x = (bounds.x - previousOffsetX) / previousScale;
				bounds.y = (bounds.y - previousOffsetY) / previousScale;
				bounds.width /= previousScale;
				bounds.height /= previousScale;
			}
			bounds.x = bounds.x * uniformScale + offsetX;
			bounds.y = bounds.y * uniformScale + offsetY;
			bounds.width *= uniformScale;
			bounds.height *= uniformScale;
			uiObject->SetBounds(bounds);
		}

		lastScale = uniformScale;
		lastOffset = UISize{ offsetX, offsetY };
		hasScaleState = true;
	}

	void ApplyLayoutOverrides(const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
		const UISize& viewportSize,
		const UISize& referenceResolution,
		float& lastScale,
		UISize& lastOffset,
		bool& hasScaleState,
		const bool useAnchorLayout,
		const bool useResolutionScale)
	{
// 		constexpr float kMinScale = 0.001f;
// 		if (useResolutionScale && hasScaleState && lastScale >= kMinScale)
// 		{
// 			for (const auto& [name, uiObject] : uiMap)
// 			{
// 				if (!uiObject || !uiObject->HasBounds())
// 				{
// 					continue;
// 				}
// 
// 				UIRect bounds = uiObject->GetBounds();
// 				bounds.x = (bounds.x - lastOffset.width) / lastScale;
// 				bounds.y = (bounds.y - lastOffset.height) / lastScale;
// 				bounds.width /= lastScale;
// 				bounds.height /= lastScale;
// 				uiObject->SetBounds(bounds);
// 			}
// 		}

		for (const auto& [name, uiObject] : uiMap)
		{
			if (uiObject)
			{
				ApplySizeBoxOverrides(*uiObject);
			}
		}

		for (const auto& [name, uiObject] : uiMap)
		{
			if (uiObject)
			{
				ApplyScaleBoxLayout(*uiObject, uiMap);
			}
		}

		if (useAnchorLayout)
		{
			ApplyAnchorLayout(uiMap);
		}

		for (const auto& [name, uiObject] : uiMap)
		{
			if (uiObject)
			{
				ApplyHorizontalBoxLayout(*uiObject, uiMap);
			}
		}

		std::unordered_map<std::string, bool> visibilityCache;
		std::unordered_set<std::string> visibilityVisiting;
		for (const auto& [name, uiObject] : uiMap)
		{
			if (!uiObject)
			{
				continue;
			}

			bool parentVisible = true;
			const std::string& parentName = uiObject->GetParentName();
			if (!parentName.empty())
			{
				auto itParent = uiMap.find(parentName);
				if (itParent != uiMap.end() && itParent->second)
				{
					parentVisible = ResolveInheritedVisibility(parentName, uiMap, visibilityCache, visibilityVisiting);
				}
			}
			uiObject->SetIsVisibleFromParent(parentVisible);
		}

// 		if (useResolutionScale)
// 		{
// 			ApplyResolutionScale(uiMap, viewportSize, referenceResolution, lastScale, lastOffset, hasScaleState);
// 		}
	}
}


UIManager::~UIManager()
{
	Reset();
}


void UIManager::SetEventDispatcher(EventDispatcher* eventDispatcher)
{
	if (m_EventDispatcher != nullptr && m_EventDispatcher->IsAlive() && m_EventDispatcher->FindListeners(EventType::Pressed))
	{
		m_EventDispatcher->RemoveListener(EventType::Pressed, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->IsAlive() && m_EventDispatcher->FindListeners(EventType::UIHovered))
	{
		m_EventDispatcher->RemoveListener(EventType::UIHovered, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->IsAlive() && m_EventDispatcher->FindListeners(EventType::UIDragged))
	{
		m_EventDispatcher->RemoveListener(EventType::UIDragged, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->IsAlive() && m_EventDispatcher->FindListeners(EventType::UIDoubleClicked))
	{
		m_EventDispatcher->RemoveListener(EventType::UIDoubleClicked, this);
	}
	if (m_EventDispatcher != nullptr && m_EventDispatcher->IsAlive() && m_EventDispatcher->FindListeners(EventType::Released))
	{
		m_EventDispatcher->RemoveListener(EventType::Released, this);
	}

	m_EventDispatcher = eventDispatcher;

	if (!m_EventDispatcher)
		return;

	m_EventDispatcher->AddListener(EventType::Pressed, this);
	m_EventDispatcher->AddListener(EventType::UIHovered, this);
	m_EventDispatcher->AddListener(EventType::UIDragged, this);
	m_EventDispatcher->AddListener(EventType::UIDoubleClicked, this);
	m_EventDispatcher->AddListener(EventType::Released, this);
}


void UIManager::Start()
{

}


bool UIManager::IsFullScreenUIActive() const
{
	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return false;

	const auto& uiMap = it->second;
	for (const auto& pair : uiMap)
	{
		const auto& uiObject = pair.second;
		if (uiObject->IsVisible() && uiObject->IsFullScreen())
			return true;
	}
	return false;
}

void UIManager::Update(float deltaTime)
{
	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return;

	for (auto& pair : it->second)
	{
		pair.second->Update(deltaTime);
	}

	ApplyLayoutOverrides(it->second, m_ViewportSize, m_ReferenceResolution, m_LastResolutionScale, m_LastResolutionOffset, m_HasResolutionScaleState, m_UseAnchorLayout, m_UseResolutionScale);
}

std::shared_ptr<UIObject> UIManager::FindUIObject(const std::string& sceneName, const std::string& objectName)
{
	auto it = m_UIObjects.find(sceneName);
	if (it == m_UIObjects.end())
	{
		return nullptr;
	}

	auto itObj = it->second.find(objectName);
	if (itObj == it->second.end())
	{
		return nullptr;
	}

	return itObj->second;
}


void UIManager::OnEvent(EventType type, const void* data)
{
	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return;

	auto& uiMap = it->second;
	ApplyLayoutOverrides(uiMap, m_ViewportSize, m_ReferenceResolution, m_LastResolutionScale, m_LastResolutionOffset, m_HasResolutionScaleState, m_UseAnchorLayout, m_UseResolutionScale);
	UpdateSortedUI(uiMap);

	const auto* rawMouseData = static_cast<const Events::MouseState*>(data);
	Events::MouseState scaledMouseData = rawMouseData ? *rawMouseData : Events::MouseState{};

	if (rawMouseData && m_UseResolutionScale && m_HasResolutionScaleState && m_LastResolutionScale > 0.0f)
	{
		scaledMouseData.pos.x = static_cast<LONG>(scaledMouseData.pos.x * m_LastResolutionScale + m_LastResolutionOffset.width);
		scaledMouseData.pos.y = static_cast<LONG>(scaledMouseData.pos.y * m_LastResolutionScale + m_LastResolutionOffset.height);
	}

	const auto* mouseData = rawMouseData ? &scaledMouseData : nullptr;
	auto sendToHitUIs = [&](EventType eventType, UIObject* skipUi = nullptr, bool requireHit = true) {
		bool handledAny = false;

		for (auto* ui : m_SortedUI)
		{
			if (!ui || ui == skipUi || !ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!(ui->hasButton || ui->hasSlider || ui->hasUIFSM))
				continue;
			if (requireHit && (!mouseData || !ui->HitCheck(mouseData->pos)))
				continue;

			if (SendEventToUI(ui, eventType, mouseData))
			{
				handledAny = true;
			}
		}

		if (handledAny && rawMouseData)
			rawMouseData->handled = true;

		return handledAny;
	};

	if (type == EventType::Pressed)
	{
		m_ActiveUI = nullptr;
		bool handledAny = false;

		for (auto* ui : m_SortedUI)
		{
			if (!ui || !ui->IsVisible())
				continue;
			if (m_FullScreenUIActive && ui->GetZOrder() < m_FullScreenZ)
				continue;
			if (!(ui->hasButton || ui->hasSlider || ui->hasUIFSM))
				continue;
			if (!mouseData || !ui->HitCheck(mouseData->pos))
				continue;

			if (SendEventToUI(ui, type, mouseData))
			{
				if (!m_ActiveUI && (ui->hasButton || ui->hasSlider))
					m_ActiveUI = ui;
				if (rawMouseData)
					rawMouseData->handled = true;
				handledAny = true;
			}
		}

		if (handledAny && rawMouseData)
			rawMouseData->handled = true;
	}
	else if (type == EventType::UIDragged || type == EventType::Released)
	{
		bool handled = false;
		if (m_ActiveUI)
		{
			handled = SendEventToUI(m_ActiveUI, type == EventType::UIDragged ? EventType::UIDragged : type, mouseData);
		}

		const bool handledByHit = sendToHitUIs(type == EventType::UIDragged ? EventType::UIDragged : type, m_ActiveUI);
		handled = handled || handledByHit;
		if (handled && rawMouseData)
		{
			rawMouseData->handled = true;
		}
		if (type == EventType::Released)
		{
			m_ActiveUI = nullptr;
		}
	}
	else if (type == EventType::UIDoubleClicked)
	{
		sendToHitUIs(EventType::UIDoubleClicked);
	}
	else if (type == EventType::UIHovered)
	{
		sendToHitUIs(EventType::UIHovered, nullptr, false);
	}
}

void UIManager::UpdateSortedUI(const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
{
	m_SortedUI.clear();
	m_SortedUI.reserve(uiMap.size());

	m_FullScreenUIActive = false;
	m_FullScreenZ = -1;

	for (auto& pair : uiMap)
	{
		UIObject* ui = pair.second.get();
		m_SortedUI.push_back(ui);

		if (ui->IsVisible() && ui->IsFullScreen())
		{
			m_FullScreenUIActive = true;
			if (ui->GetZOrder() > m_FullScreenZ)
				m_FullScreenZ = ui->GetZOrder();
		}
	}

	std::sort(m_SortedUI.begin(), m_SortedUI.end(), [](UIObject* a, UIObject* b) {
		return a->GetZOrder() > b->GetZOrder();
		});
}

bool UIManager::SendEventToUI(UIObject* ui, EventType type, const void* data)
{
	bool handled = false;

	if (ui->hasButton)
	{
		auto buttons = ui->GetComponents<UIButtonComponent>();
		for (auto* button : buttons)
		{
			if (!button)
				continue;

			const bool isEnabled = button->GetIsEnabled();

			if (type == EventType::Pressed)
			{
				if (isEnabled)
				{
					button->HandlePressed();
					handled = true;
				}
			}
			else if (type == EventType::Released)
			{
				if (isEnabled)
				{
					button->HandleReleased();
					handled = true;
				}
			}
			else if (type == EventType::UIHovered)
			{
				const auto mouseData = static_cast<const Events::MouseState*>(data);
				const bool isHovered = ui->HitCheck(mouseData->pos);
				if (isEnabled)
				{
					button->HandleHover(isHovered);
					handled = true;
				}
			}
		}
		if (ui->hasSlider)
		{
			auto sliders = ui->GetComponents<UISliderComponent>();
			for (auto* slider : sliders)
			{
				if (!slider)
					continue;

				if (type == EventType::UIDragged)
				{
					const auto mouseData = static_cast<const Events::MouseState*>(data);
					const auto bounds = ui->GetBounds();
					float normalizedValue = 0.0f;
					const UIFillDirection direction = slider->GetFillDirection();
					const bool isVertical = direction == UIFillDirection::TopToBottom
						|| direction == UIFillDirection::BottomToTop;
					if (isVertical)
					{
						if (bounds.height > 0.0f)
						{
							normalizedValue = (mouseData->pos.y - bounds.y) / bounds.height;
						}
						if (direction == UIFillDirection::BottomToTop)
						{
							normalizedValue = 1.0f - normalizedValue;
						}
					}
					else
					{
						if (bounds.width > 0.0f)
						{
							normalizedValue = (mouseData->pos.x - bounds.x) / bounds.width;
						}
						if (direction == UIFillDirection::RightToLeft)
						{
							normalizedValue = 1.0f - normalizedValue;
						}
					}
					normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
					slider->HandleDrag(normalizedValue);
					handled = true;
				}
				else if (type == EventType::Released)
				{
					slider->HandleReleased();
					handled = true;
				}
			}
		}
		if (ui->hasUIFSM)
		{
			auto* fsm = ui->GetComponent<UIFSMComponent>();
			if (!fsm)
			{
				return handled;
			}
			if (!fsm->ShouldHandleEvent(type, data))
			{
				return handled;
			}

			if (type == EventType::UIHovered)
			{
				const auto mouseData = static_cast<const Events::MouseState*>(data);
				if (!ui->HitCheck(mouseData->pos))
				{
					return handled;
				}
			}
			fsm->OnEvent(type, data);
			handled = true;
		}
	}

	return handled;
}

void UIManager::RefreshUIListForCurrentScene()
{
	auto& uiObjects = GetUIObjects();
	auto it = uiObjects.find(m_CurrentSceneName);
	if (it != uiObjects.end())
	{
		UpdateSortedUI(it->second);
		return;
	}

	m_SortedUI.clear();
	m_FullScreenUIActive = false;
	m_FullScreenZ = -1;
	m_ActiveUI = nullptr;
	m_LastHoveredUI = nullptr;
}

void UIManager::BuildUIFrameData(RenderData::FrameData& frameData)
{
	frameData.uiElements.clear();
	frameData.uiTexts.clear();

	auto it = m_UIObjects.find(m_CurrentSceneName);
	if (it == m_UIObjects.end())
		return;

	ApplyLayoutOverrides(it->second, m_ViewportSize, m_ReferenceResolution, m_LastResolutionScale, m_LastResolutionOffset, m_HasResolutionScaleState, m_UseAnchorLayout, m_UseResolutionScale);

	for (const auto& [name, uiObject] : it->second)
	{
		if (!uiObject || !uiObject->IsVisible() || !uiObject->HasBounds())
		{
			continue;
		}

		const auto& bounds = uiObject->GetBounds();
		const int baseZOrder = uiObject->GetZOrder();
		const auto* imageComponent = uiObject->GetComponent<UIImageComponent>();
		auto* progressComponent = uiObject->GetComponent<UIProgressBarComponent>();
		auto* sliderComponent = uiObject->GetComponent<UISliderComponent>();
		auto* buttonComponent = uiObject->GetComponent<UIButtonComponent>();
		auto* textComponent = uiObject->GetComponent<UITextComponent>();
		const bool hasVisualElement = imageComponent || progressComponent || sliderComponent || buttonComponent;
		if (!hasVisualElement && !textComponent)
		{
			continue;
		}

		auto uiComp = uiObject->GetComponent<UIComponent>();
		auto resolveOpacity = [](const std::shared_ptr<UIObject> uiObject)
			{
				return uiObject ? uiObject->GetOpacity() : 1.0f;
			};

		auto applyOverrides = [&](RenderData::UIElement& element,
			const TextureHandle& texture,
			const ShaderAssetHandle& shaderAsset,
			const VertexShaderHandle& vertexShader,
			const PixelShaderHandle& pixelShader
			)
			{
				const bool hasOverrides = texture.IsValid() || shaderAsset.IsValid()
					|| vertexShader.IsValid() || pixelShader.IsValid();
				if (!hasOverrides)
				{
					return;
				}

				element.useMaterialOverrides = true;
				element.materialOverrides.textureHandle = texture;
				element.materialOverrides.shaderAsset = shaderAsset;
				element.materialOverrides.vertexShader = vertexShader;
				element.materialOverrides.pixelShader = pixelShader;
			};

		auto applyImageOverrides = [&](RenderData::UIElement& element, const UIImageComponent* image)
			{
				if (!image)
					return;

				applyOverrides(element,
					image->GetTextureHandle(),
					image->GetShaderAssetHandle(),
					image->GetVertexShaderHandle(),
					image->GetPixelShaderHandle());
			};

		auto appendElement = [&](const UIRect& rect, int zOrder, const UIImageComponent* image, float opacity, float progress = 1.0f, float progressDirection = 0.0f, const TextureHandle& maskTexture = TextureHandle::Invalid())
			{
				RenderData::UIElement element{};
				element.position = { rect.x, rect.y };
				element.size = { rect.width, rect.height };
				element.rotation = uiObject->GetRotationDegrees();
				element.zOrder = zOrder;
				element.color = image ? image->GetTintColor() : DirectX::XMFLOAT4{ 1.0f, 1.0f, 1.0f, 1.0f };
				element.opacity = opacity;
				element.progress = progress;
				element.progressDirection = progressDirection;
				element.maskTextureHandle = maskTexture;
				applyImageOverrides(element, image);
				frameData.uiElements.push_back(element);
			};

		auto buildFillRect = [](const UIRect& rect, float ratio, UIFillDirection direction)
			{
				UIRect fill = rect;
				switch (direction)
				{
				case UIFillDirection::LeftToRight:
					fill.width = rect.width * ratio;
					break;
				case UIFillDirection::RightToLeft:
					fill.width = rect.width * ratio;
					fill.x = rect.x + rect.width - fill.width;
					break;
				case UIFillDirection::TopToBottom:
					fill.height = rect.height * ratio;
					break;
				case UIFillDirection::BottomToTop:
					fill.height = rect.height * ratio;
					fill.y = rect.y + rect.height - fill.height;
					break;
				default:
					break;
				}
				return fill;
			};

		const float opacity = resolveOpacity(uiObject);

		if (auto* progress = progressComponent)
		{
			appendElement(bounds, baseZOrder, nullptr, 1.0f);
			auto& backgroundElement = frameData.uiElements.back();
			applyOverrides(backgroundElement,
				progress->GetBackgroundTextureHandle(),
				progress->GetBackgroundShaderAssetHandle(),
				progress->GetBackgroundVertexShaderHandle(),
				progress->GetBackgroundPixelShaderHandle());

			const float percent = std::clamp(progress->GetPercent(), 0.0f, 1.0f);
			if (percent > 0.0f)
			{
				UIRect fillRect = bounds;
				float progressValue = percent;
				const bool useMaskTexture = progress->GetFillMaskTextureHandle().IsValid();

				if (progress->GetFillMode() == UIProgressFillMode::Rect && !useMaskTexture)
				{
					fillRect = buildFillRect(bounds, percent, progress->GetFillDirection());
				}

				const UIFillDirection fillDirection = progress->GetFillDirection();
				const bool isReverseFill = fillDirection == UIFillDirection::RightToLeft
					|| fillDirection == UIFillDirection::BottomToTop;

				const float progressDirection = static_cast<float>(fillDirection);
				appendElement(fillRect, baseZOrder + 1, nullptr, opacity, progressValue, progressDirection, progress->GetFillMaskTextureHandle());

				auto& fillElement = frameData.uiElements.back();
				applyOverrides(fillElement,
					progress->GetFillTextureHandle(),
					progress->GetFillShaderAssetHandle(),
					progress->GetFillVertexShaderHandle(),
					progress->GetFillPixelShaderHandle());
			}
		}
		else if (auto* slider = sliderComponent)
		{
			appendElement(bounds, baseZOrder, nullptr, opacity);
			auto& backgroundElement = frameData.uiElements.back();
			applyOverrides(backgroundElement,
				slider->GetBackgroundTextureHandle(),
				slider->GetBackgroundShaderAssetHandle(),
				slider->GetBackgroundVertexShaderHandle(),
				slider->GetBackgroundPixelShaderHandle());

			const float normalized = std::clamp(slider->GetNormalizedValue(), 0.0f, 1.0f);
			if (normalized > 0.0f)
			{
				UIRect fillRect = buildFillRect(bounds, normalized, slider->GetFillDirection());
				appendElement(fillRect, baseZOrder + 1, nullptr, opacity);
				auto& fillElement = frameData.uiElements.back();
				applyOverrides(fillElement,
					slider->GetFillTextureHandle(),
					slider->GetFillShaderAssetHandle(),
					slider->GetFillVertexShaderHandle(),
					slider->GetFillPixelShaderHandle());
			}

			const UIFillDirection fillDirection = slider->GetFillDirection();
			const bool isVertical = fillDirection == UIFillDirection::TopToBottom
				|| fillDirection == UIFillDirection::BottomToTop;

			float handleSize = std::min(bounds.width, bounds.height);
			if (slider->HasHandleSizeOverride())
			{
				handleSize = slider->GetHandleSizeOverride();
			}

			if (handleSize > 0.0f)
			{
				UIRect handleRect = bounds;
				handleRect.width = handleSize;
				handleRect.height = handleSize;
				if (isVertical)
				{
					const float ratio = fillDirection == UIFillDirection::BottomToTop ? 1.0f - normalized : normalized;
					handleRect.x = bounds.x + (bounds.width - handleSize) * 0.5f;
					handleRect.y = bounds.y + bounds.height * ratio - handleSize * 0.5f;
					handleRect.y = std::clamp(handleRect.y, bounds.y, bounds.y + bounds.height - handleSize);
				}
				else
				{
					const float ratio = fillDirection == UIFillDirection::RightToLeft ? 1.0f - normalized : normalized;
					handleRect.x = bounds.x + bounds.width * ratio - handleSize * 0.5f;
					handleRect.x = std::clamp(handleRect.x, bounds.x, bounds.x + bounds.width - handleSize);
					handleRect.y = bounds.y + (bounds.height - handleSize) * 0.5f;
				}
				appendElement(handleRect, baseZOrder + 2, nullptr, opacity);
				auto& handleElement = frameData.uiElements.back();
				applyOverrides(handleElement,
					slider->GetHandleTextureHandle(),
					slider->GetHandleShaderAssetHandle(),
					slider->GetHandleVertexShaderHandle(),
					slider->GetHandlePixelShaderHandle());
			}
		}
		else if (hasVisualElement)
		{
			appendElement(bounds, baseZOrder, imageComponent, opacity);
			if (auto* button = buttonComponent)
			{
				auto& element = frameData.uiElements.back();

				if (button->HasStyleOverrides())
				{
					applyOverrides(element,
						button->GetCurrentTextureHandle(),
						button->GetShaderAssetHandle(),
						button->GetVertexShaderHandle(),
						button->GetPixelShaderHandle());
				}
				if (button->HasColorOverrides())
				{
					element.color = button->GetCurrentTintColor();
				}
			}
		}

		if (auto* textComp = textComponent)
		{
			RenderData::UITextElement text{};
			text.position = { bounds.x, bounds.y };
			text.color = textComp->GetTextColor();
			text.color.w = opacity;
			text.fontSize = textComp->GetFontSize();
			text.text = textComp->GetText();
			frameData.uiTexts.push_back(std::move(text));
		}
	}

	std::sort(frameData.uiElements.begin(), frameData.uiElements.end(), [](const RenderData::UIElement& a, const RenderData::UIElement& b)
		{
			return a.zOrder < b.zOrder;
		});
}

void UIManager::SerializeSceneUI(const std::string& sceneName, nlohmann::json& out) const
{
	out = nlohmann::json::array();
	auto it = m_UIObjects.find(sceneName);
	if (it == m_UIObjects.end())
	{
		return;
	}

	for (const auto& [name, uiObject] : it->second)
	{
		if (!uiObject)
		{
			continue;
		}
		nlohmann::json entry;
		uiObject->Serialize(entry);
		out.push_back(entry);
	}
}

void UIManager::DeserializeSceneUI(const std::string& sceneName, const nlohmann::json& data)
{
	if (!m_EventDispatcher)
	{
		return;
	}

	auto& uiMap = m_UIObjects[sceneName];
	uiMap.clear();

	nlohmann::json objects = nlohmann::json::array();
	if (data.is_array())
	{
		objects = data;
	}
	else if (data.is_object())
	{
		if (data.contains("objects") && data["objects"].is_array())
		{
			objects = data["objects"];
		}
	}

	for (const auto& entry : objects)
	{
		auto uiObject = std::make_shared<UIObject>(*m_EventDispatcher);
		uiObject->Deserialize(entry);
		uiObject->UpdateInteractableFlags();
		uiMap[uiObject->GetName()] = uiObject;
	}

	UpdateSortedUI(uiMap);
}



//void UIManager::Render(std::vector<UIRenderInfo>& uiRenderInfo, std::vector<UITextInfo>& uiTextInfo)
//{
//	auto it = m_UIObjects.find(m_CurrentSceneName);
//	if (it == m_UIObjects.end())
//		return;
//
//	for (auto& pair : it->second)
//	{
//		if (!pair.second->IsVisible())
//			continue;
//
//		pair.second->Render(uiRenderInfo);
//		pair.second->Render(uiTextInfo);
//	}
//}

void UIManager::Reset()
{
	SetEventDispatcher(nullptr);
	m_UIObjects.clear();
	m_ActiveUI = nullptr;
}

void UIManager::ClearSceneUI(const std::string& sceneName)
{
	auto itScene = m_UIObjects.find(sceneName);
	if (itScene == m_UIObjects.end())
	{
		return;
	}

	auto& uiMap = itScene->second;
	if (m_ActiveUI)
	{
		for (const auto& [name, uiObject] : uiMap)
		{
			if (uiObject && uiObject.get() == m_ActiveUI)
			{
				m_ActiveUI = nullptr;
				break;
			}
		}
	}

	if (m_LastHoveredUI)
	{
		for (const auto& [name, uiObject] : uiMap)
		{
			if (uiObject && uiObject.get() == m_LastHoveredUI)
			{
				m_LastHoveredUI = nullptr;
				break;
			}
		}
	}

	m_UIObjects.erase(itScene);
}
