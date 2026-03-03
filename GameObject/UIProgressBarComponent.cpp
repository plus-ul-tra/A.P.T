#include "UIProgressBarComponent.h"
#include "ReflectionMacro.h"
#include "UIFSMComponent.h"
#include <algorithm>

REGISTER_UI_COMPONENT(UIProgressBarComponent)
REGISTER_PROPERTY(UIProgressBarComponent, Percent)
REGISTER_PROPERTY(UIProgressBarComponent, FillDirection)
REGISTER_PROPERTY(UIProgressBarComponent, FillMode)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, BackgroundTextureHandle)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, BackgroundShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, BackgroundVertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, BackgroundPixelShaderHandle)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, FillTextureHandle)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, FillShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, FillVertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, FillPixelShaderHandle)
REGISTER_PROPERTY_HANDLE(UIProgressBarComponent, FillMaskTextureHandle)

void UIProgressBarComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UIProgressBarComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void UIProgressBarComponent::SetPercent(const float& percent)
{
	const float previous = m_Percent;
	m_Percent = std::clamp(percent, 0.0f, 1.0f);
	if (m_Percent != previous)
	{
		if (m_OnPercentChanged)
		{
			m_OnPercentChanged(m_Percent);
		}
		if (auto* owner = GetOwner())
		{
			if (auto* fsm = owner->GetComponent<UIFSMComponent>())
			{
				const float currentPercent = m_Percent;
				fsm->TriggerEventByName("UI_ProgressChanged", &currentPercent);
			}
		}
	}
}
