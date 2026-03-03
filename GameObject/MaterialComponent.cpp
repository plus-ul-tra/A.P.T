#include "MaterialComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(MaterialComponent);
REGISTER_PROPERTY_HANDLE(MaterialComponent, MaterialHandle)
REGISTER_PROPERTY(MaterialComponent, Overrides)
REGISTER_PROPERTY_READONLY_LOADABLE(MaterialComponent, Material)

void MaterialComponent::SetOverrides(const RenderData::MaterialData& overrides)
{
	m_Overrides = overrides;
	m_UseOverrides = true;	
}

void MaterialComponent::ClearOverrides()
{
	m_Overrides = RenderData::MaterialData{};
	m_UseOverrides = false;
}

void MaterialComponent::Update(float deltaTime)
{
}

void MaterialComponent::OnEvent(EventType type, const void* data)
{
}

void MaterialComponent::RefreshDerivedAfterMaterialChanged()
{
	// 1) 머티리얼 원본 데이터 얻기
	auto* loader = AssetLoader::GetActive();
	if (!loader || !m_MaterialHandle.IsValid())
	{
		m_Overrides = RenderData::MaterialData{}; // 기본값
		return;
	}

	const auto* src = loader->GetMaterials().Get(m_MaterialHandle);
	if (!src)
	{
		m_Overrides = RenderData::MaterialData{};
		return;
	}

	m_Overrides = *src;

	SetOverrides(m_Overrides); 
}
