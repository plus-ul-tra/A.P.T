#pragma once
#include "Component.h"
#include "ResourceHandle.h"
#include "ResourceRefs.h"
#include "AssetLoader.h"

class MeshComponent : public Component
{
	friend class Editor;

public:
	struct SubMeshMaterialOverride
	{
		MaterialRef material{};
		ShaderAssetHandle shaderAsset = ShaderAssetHandle::Invalid();
		VertexShaderHandle vertexShader = VertexShaderHandle::Invalid();
		PixelShaderHandle pixelShader = PixelShaderHandle::Invalid();

		bool HasMaterialOverride() const { return !material.assetPath.empty(); }
		bool HasShaderOverrides() const { return shaderAsset.IsValid() || vertexShader.IsValid() || pixelShader.IsValid(); }
	};


	static constexpr const char* StaticTypeName = "MeshComponent";
	const char* GetTypeName() const override;

	MeshComponent() = default;
	virtual ~MeshComponent() = default;

	void			  SetMeshHandle(const MeshHandle& handle) 
	{ 
		m_MeshHandle = handle; 

		// 파생값 갱신 (읽기전용 표시용)
		MeshRef ref{};
		if (auto* loader = AssetLoader::GetActive())
			loader->GetMeshAssetReference(handle, ref.assetPath, ref.assetIndex);

		LoadSetMesh(ref); // 또는 내부 갱신 함수
	}
	const MeshHandle& GetMeshHandle() const					  { return m_MeshHandle;   }

	void LoadSetMesh(const MeshRef& meshRef)
	{
		m_Mesh = meshRef;
	}

	const MeshRef& GetMesh() const { return m_Mesh; }

	const std::vector<SubMeshMaterialOverride>& GetSubMeshMaterialOverrides() const { return m_SubMeshMaterialOverrides; }
	void SetSubMeshMaterialOverrides(const std::vector<SubMeshMaterialOverride>& overrides);
	void SetSubMeshMaterialOverride(size_t index, const MaterialRef& overrideRef);
	void SetSubMeshShaderAssetOverride(size_t index, const ShaderAssetHandle& handle);
	void SetSubMeshVertexShaderOverride(size_t index, const VertexShaderHandle& handle);
	void SetSubMeshPixelShaderOverride(size_t index, const PixelShaderHandle& handle);
	void ClearSubMeshMaterialOverride(size_t index);
	void ClearSubMeshShaderOverrides(size_t index);


	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	MeshHandle   m_MeshHandle;
	MeshRef		 m_Mesh;
	std::vector<SubMeshMaterialOverride> m_SubMeshMaterialOverrides;
};



