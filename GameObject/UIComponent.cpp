#include "UIComponent.h"
#include "ReflectionMacro.h"
#include "UIObject.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"

REGISTER_UI_COMPONENT(UIComponent);
REGISTER_PROPERTY(UIComponent, Visible)
REGISTER_PROPERTY(UIComponent, ZOrder)
REGISTER_PROPERTY(UIComponent, Opacity)

void UIComponent::Update(float deltaTime)
{

}

void UIComponent::OnEvent(EventType type, const void* data)
{

}

void UIComponent::SetVisible(const bool& visible) 
{
	m_Visible = visible;
	if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
	{
		owner->SetIsVisibleFromComponent(visible);
	}
}

void UIComponent::SetZOrder(const int& value)
{
	m_ZOrder = value;
	if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
	{
		owner->SetZOrderFromComponent(value);
	}
}

void UIComponent::SetOpacity(const float& value)
{
	m_Opacity = value;
	if (auto* owner = dynamic_cast<UIObject*>(GetOwner()))
	{
		owner->SetOpacityFromComponent(value);
	}
}

void UIComponent::Serialize(nlohmann::json& j) const
{
	Component::Serialize(j);
}

void UIComponent::Deserialize(const nlohmann::json& j)
{
	Component::Deserialize(j);
}

Scene* UIComponent::GetScene() const
{
	auto* owner = GetOwner();
	return owner ? owner->GetScene() : nullptr;
}

UIManager* UIComponent::GetUIManager() const
{
	auto* scene = GetScene();
	if (!scene)
	{
		m_UIManager = nullptr;
		m_UIScene = nullptr;
		return nullptr;
	}

	if (m_UIManager && m_UIScene == scene)
	{
		return m_UIManager;
	}

	auto& services = scene->GetServices();
	if (!services.Has<UIManager>())
	{
		m_UIManager = nullptr;
		m_UIScene = scene;
		return nullptr;
	}

	m_UIManager = &services.Get<UIManager>();
	m_UIScene = scene;
	return m_UIManager;
}