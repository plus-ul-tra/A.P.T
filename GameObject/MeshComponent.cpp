#include "MeshComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "Scene.h"
#include "ServiceRegistry.h"

REGISTER_COMPONENT(MeshComponent);
REGISTER_PROPERTY_HANDLE(MeshComponent, MeshHandle)
REGISTER_PROPERTY_READONLY_LOADABLE(MeshComponent, Mesh)
REGISTER_PROPERTY(MeshComponent, SubMeshMaterialOverrides)

void MeshComponent::SetSubMeshMaterialOverrides(const std::vector<SubMeshMaterialOverride>& overrides)
{
	m_SubMeshMaterialOverrides = overrides;
}

void MeshComponent::SetSubMeshMaterialOverride(size_t index, const MaterialRef& overrideRef)
{
	if (m_SubMeshMaterialOverrides.size() <= index)
	{
		m_SubMeshMaterialOverrides.resize(index + 1);
	}

	m_SubMeshMaterialOverrides[index].material = overrideRef;
}

void MeshComponent::SetSubMeshShaderAssetOverride(size_t index, const ShaderAssetHandle& handle)
{
	if (m_SubMeshMaterialOverrides.size() <= index)
	{
		m_SubMeshMaterialOverrides.resize(index + 1);
	}

	m_SubMeshMaterialOverrides[index].shaderAsset = handle;
}

void MeshComponent::SetSubMeshVertexShaderOverride(size_t index, const VertexShaderHandle& handle)
{
	if (m_SubMeshMaterialOverrides.size() <= index)
	{
		m_SubMeshMaterialOverrides.resize(index + 1);
	}

	m_SubMeshMaterialOverrides[index].vertexShader = handle;
}

void MeshComponent::SetSubMeshPixelShaderOverride(size_t index, const PixelShaderHandle& handle)
{
	if (m_SubMeshMaterialOverrides.size() <= index)
	{
		m_SubMeshMaterialOverrides.resize(index + 1);
	}

	m_SubMeshMaterialOverrides[index].pixelShader = handle;
}

void MeshComponent::ClearSubMeshMaterialOverride(size_t index)
{
	if (index >= m_SubMeshMaterialOverrides.size())
	{
		return;
	}

	m_SubMeshMaterialOverrides[index].material = MaterialRef{};
}

void MeshComponent::ClearSubMeshShaderOverrides(size_t index)
{
	if (index >= m_SubMeshMaterialOverrides.size())
	{
		return;
	}

	m_SubMeshMaterialOverrides[index].shaderAsset = ShaderAssetHandle::Invalid();
	m_SubMeshMaterialOverrides[index].vertexShader = VertexShaderHandle::Invalid();
	m_SubMeshMaterialOverrides[index].pixelShader = PixelShaderHandle::Invalid();
}

void MeshComponent::Update(float deltaTime)
{
}

void MeshComponent::OnEvent(EventType type, const void* data)
{
}