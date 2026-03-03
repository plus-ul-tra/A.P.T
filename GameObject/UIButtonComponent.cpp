#include "UIButtonComponent.h"
#include "ReflectionMacro.h"
#include "UIFSMComponent.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"


REGISTER_UI_COMPONENT(UIButtonComponent)
REGISTER_PROPERTY(UIButtonComponent, IsEnabled)
REGISTER_PROPERTY_READONLY(UIButtonComponent, IsPressed)
REGISTER_PROPERTY_READONLY(UIButtonComponent, IsHovered)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, NormalTextureHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, HoveredTextureHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, PressedTextureHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, DisabledTextureHandle)
REGISTER_PROPERTY(UIButtonComponent, NormalColor)
REGISTER_PROPERTY(UIButtonComponent, HoveredColor)
REGISTER_PROPERTY(UIButtonComponent, PressedColor)
REGISTER_PROPERTY(UIButtonComponent, DisabledColor)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, ShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, VertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UIButtonComponent, PixelShaderHandle)

namespace
{

	std::vector<std::string> ResolveInventoryInfoPanelCandidates(const std::string& objectName)
	{
		if (objectName == "Player_Melee" || objectName == "Player_MeleeButton" || objectName == "MeleeButton" || objectName == "MainWeaponButton" || objectName == "MainWeapon")
		{
			return { "MainWeaponInfo" };
		}

		if (objectName == "Player_Throw1" || objectName == "Player_Throw_1" || objectName == "SubWeapon1")
		{
			return { "SubWeapon1Info" };
		}

		if (objectName == "Player_Throw2" || objectName == "Player_Throw_2" || objectName == "SubWeapon2")
		{
			return { "SubWeapon2Info" };
		}

		if (objectName == "Player_Throw3" || objectName == "Player_Throw_3" || objectName == "SubWeapon3")
		{
			return { "SubWeapon3Info" };
		}

		return {};
	}

	std::shared_ptr<UIObject> FindInventoryInfoPanelObject(Scene* scene, const std::string& ownerName) 
	{
		if (!scene)
		{
			return nullptr;
		}

		auto& services = scene->GetServices();
		if (!services.Has<UIManager>())
		{
			return nullptr;
		}

		auto& uiManager = services.Get<UIManager>();
		const std::string sceneName = scene->GetName();

		std::vector<std::string> candidates = ResolveInventoryInfoPanelCandidates(ownerName);
		if (candidates.empty())
		{
			return nullptr;
		}

		static const std::vector<std::string> kFallbackCandidates = {
			"InventoryInfo",
			"Player_InventoryInfo",
			"ItemInfo",
			"Player_ItemInfo",
			"InventoryInfoPanel"
		};

		candidates.insert(candidates.end(), kFallbackCandidates.begin(), kFallbackCandidates.end());

		for (const auto& candidate : candidates) 
		{
			auto uiObject = uiManager.FindUIObject(sceneName, candidate);
			if (uiObject)
			{
				return uiObject;
			}
		}

		return nullptr;
	}
	std::string ResolveInventoryHoverEventName(const std::string& objectName, bool isHovered)
	{
		const std::string prefix = isHovered ? "UI_RequestInventoryInfoShow_" : "UI_RequestInventoryInfoHide_";
		if (objectName == "Player_Melee" || objectName == "Player_MeleeButton" || objectName == "MeleeButton" || objectName == "MainWeaponButton" || objectName == "MainWeapon")
		{
			return prefix + "Melee";
		}

		if (objectName == "Player_Throw1" || objectName == "Player_Throw_1" || objectName == "SubWeapon1")
		{
			return prefix + "Throw1";
		}

		if (objectName == "Player_Throw2" || objectName == "Player_Throw_2" || objectName == "SubWeapon2")
		{
			return prefix + "Throw2";
		}

		if (objectName == "Player_Throw3" || objectName == "Player_Throw_3" || objectName == "SubWeapon3")
		{
			return prefix + "Throw3";
		}

		return {};
	}
}

void UIButtonComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UIButtonComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void UIButtonComponent::SetIsEnabled(const bool& enabled)
{
	if (m_IsEnabled == enabled)
	{
		return;
	}

	m_IsEnabled = enabled;

	if (!m_IsEnabled)
	{
		m_IsPressed = false;
		if (m_IsHovered)
		{
			m_IsHovered = false;
			if (m_OnHovered)
			{
				m_OnHovered(false);
			}
		}
	}
}

TextureHandle UIButtonComponent::GetCurrentTextureHandle() const
{
	if (!m_IsEnabled && m_Style.disabledTexture.IsValid())
	{
		return m_Style.disabledTexture;
	}
	if (m_IsPressed && m_Style.pressedTexture.IsValid())
	{
		return m_Style.pressedTexture;
	}
	if (m_IsHovered && m_Style.hoveredTexture.IsValid())
	{
		return m_Style.hoveredTexture;
	}
	return m_Style.normalTexture;
}

XMFLOAT4 UIButtonComponent::GetCurrentTintColor() const
{
	if (!m_IsEnabled)
	{
		return m_Style.disabledColor;
	}
	if (m_IsPressed)
	{
		return m_Style.pressedColor;
	}
	if (m_IsHovered)
	{
		return m_Style.hoveredColor;
	}
	return m_Style.normalColor;
}

bool UIButtonComponent::HasStyleOverrides() const
{
	return m_Style.normalTexture.IsValid()
		|| m_Style.hoveredTexture.IsValid()
		|| m_Style.pressedTexture.IsValid()
		|| m_Style.disabledTexture.IsValid()
		|| m_Style.shaderAsset.IsValid()
		|| m_Style.vertexShader.IsValid()
		|| m_Style.pixelShader.IsValid();
}

bool UIButtonComponent::HasColorOverrides() const
{
	constexpr XMFLOAT4 kDefault{ 1.0f, 1.0f, 1.0f, 1.0f };
	return m_Style.normalColor.x != kDefault.x
		|| m_Style.normalColor.y != kDefault.y
		|| m_Style.normalColor.z != kDefault.z
		|| m_Style.normalColor.w != kDefault.w
		|| m_Style.hoveredColor.x != kDefault.x
		|| m_Style.hoveredColor.y != kDefault.y
		|| m_Style.hoveredColor.z != kDefault.z
		|| m_Style.hoveredColor.w != kDefault.w
		|| m_Style.pressedColor.x != kDefault.x
		|| m_Style.pressedColor.y != kDefault.y
		|| m_Style.pressedColor.z != kDefault.z
		|| m_Style.pressedColor.w != kDefault.w
		|| m_Style.disabledColor.x != kDefault.x
		|| m_Style.disabledColor.y != kDefault.y
		|| m_Style.disabledColor.z != kDefault.z
		|| m_Style.disabledColor.w != kDefault.w;
}

void UIButtonComponent::HandlePressed()
{
	if (!m_IsEnabled)
		return;

	if (!m_IsPressed)
	{
		m_IsPressed = true;
		if (m_OnPressed)
		{
			m_OnPressed();
		}
	}
}

void UIButtonComponent::HandleReleased()
{
	if (!m_IsEnabled)
		return;

	if (m_IsPressed && m_OnClicked)
	{
		m_OnClicked();
	}
	if (m_IsPressed && m_OnReleased)
	{
		m_OnReleased();
	}
	if (m_IsPressed)
	{
		if (auto* owner = GetOwner())
		{
			if (auto* fsm = owner->GetComponent<UIFSMComponent>())
			{
				fsm->TriggerEventByName("UI_Clicked");
			}
		}
	}

	m_IsPressed = false;
}


void UIButtonComponent::HandleHover(bool isHovered)
{
	if (!m_IsEnabled)
		return;

	if (m_IsHovered == isHovered)
	{
		return;
	}

	m_IsHovered = isHovered;
	if (auto* owner = GetOwner())
	{
		if (auto* fsm = owner->GetComponent<UIFSMComponent>())
		{
			fsm->TriggerEventByName(isHovered ? "UI_HoverEnter" : "UI_HoverExit");

			const std::string eventName = ResolveInventoryHoverEventName(owner->GetName(), isHovered);
			if (!eventName.empty())
			{
				auto* scene = owner->GetScene();
				auto infoPanel = FindInventoryInfoPanelObject(scene, owner->GetName());
				if (infoPanel && infoPanel.get() != owner)
				{
					if (auto* infoPanelFsm = infoPanel->GetComponent<UIFSMComponent>())
					{
						infoPanelFsm->TriggerEventByName(eventName);
					}
				}
			}
		}
	}

	if (m_OnHovered)
	{
		m_OnHovered(isHovered);
	}
}
