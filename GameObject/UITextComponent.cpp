#include "UITextComponent.h"
#include "ReflectionMacro.h"

REGISTER_UI_COMPONENT(UITextComponent)
REGISTER_PROPERTY(UITextComponent, Text)
REGISTER_PROPERTY(UITextComponent, FontSize)
REGISTER_PROPERTY(UITextComponent, TextColor)

void UITextComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UITextComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}