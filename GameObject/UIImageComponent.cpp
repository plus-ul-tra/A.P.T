#include "UIImageComponent.h"
#include "ReflectionMacro.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "UIManager.h"

REGISTER_UI_COMPONENT(UIImageComponent)
REGISTER_PROPERTY_HANDLE(UIImageComponent, TextureHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, ShaderAssetHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, VertexShaderHandle)
REGISTER_PROPERTY_HANDLE(UIImageComponent, PixelShaderHandle)
REGISTER_PROPERTY(UIImageComponent, TintColor)

void UIImageComponent::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void UIImageComponent::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}