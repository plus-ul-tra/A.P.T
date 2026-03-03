#include "UISliderComponent.h"
#include "ReflectionMacro.h"
#include "UIButtonComponent.h"
#include "UIFSMComponent.h"

REGISTER_UI_COMPONENT(UISliderComponent)
REGISTER_PROPERTY(UISliderComponent, Value)
REGISTER_PROPERTY(UISliderComponent, MinValue)
REGISTER_PROPERTY(UISliderComponent, MaxValue)
REGISTER_PROPERTY(UISliderComponent, FillDirection)
REGISTER_PROPERTY(UISliderComponent, HandleSizeOverride)
REGISTER_PROPERTY_READONLY(UISliderComponent, IsDragging)
REGISTER_PROPERTY_HANDLE(UISliderComponent, BackgroundTextureHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, BackgroundShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, BackgroundVertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, BackgroundPixelShaderHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, FillTextureHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, FillShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, FillVertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, FillPixelShaderHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, HandleTextureHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, HandleShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, HandleVertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UISliderComponent, HandlePixelShaderHandle)


void UISliderComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UISliderComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void UISliderComponent::SetRange(const float& minValue, const float& maxValue)
{
	if (minValue > maxValue)
	{
		m_MinValue = maxValue;
		m_MaxValue = minValue;
	}
	else
	{
		m_MinValue = minValue;
		m_MaxValue = maxValue;
	}


	SetValue(m_Value);
}

void UISliderComponent::SetValue(const float& value)
{
	const float previous = m_Value;
	m_Value = std::clamp(value, m_MinValue, m_MaxValue);

	if (m_Value != previous)
	{
		if (m_OnValueChanged)
		{
			m_OnValueChanged(m_Value);
		}
		if (auto* owner = GetOwner())
		{
			if (auto* fsm = owner->GetComponent<UIFSMComponent>())
			{
				const float currentValue = m_Value;
				fsm->TriggerEventByName("UI_SliderValueChanged", &currentValue);
			}
		}
	}
}

void UISliderComponent::SetNormalizedValue(float normalizedValue)
{
	normalizedValue = std::clamp(normalizedValue, 0.0f, 1.0f);
	const float value = m_MinValue + (m_MaxValue - m_MinValue) * normalizedValue;
	SetValue(value);
}

float UISliderComponent::GetNormalizedValue() const
{
	if (m_MaxValue <= m_MinValue)
	{
		return 0.0f;
	}
	return (m_Value - m_MinValue) / (m_MaxValue - m_MinValue);
	return 0.0f;
}

void UISliderComponent::HandleDrag(float normalizedValue)
{
	m_IsDragging = true;
	SetNormalizedValue(normalizedValue);
}

void UISliderComponent::HandleReleased()
{
	m_IsDragging = false;
}

