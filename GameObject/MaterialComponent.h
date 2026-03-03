#pragma once
#include "Component.h"
#include "RenderData.h"
#include "ResourceRefs.h"
#include "AssetLoader.h"

class MaterialComponent : public Component
{
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "MaterialComponent";
	const char* GetTypeName() const override;

	MaterialComponent() = default;
	virtual ~MaterialComponent() = default;

	void SetMaterialHandle(const MaterialHandle& handle)
	{ 
		m_MaterialHandle = handle;
		// 파생값 갱신 (읽기전용 표시용)
		MaterialRef ref{};
		if (auto* loader = AssetLoader::GetActive())
			loader->GetMaterialAssetReference(handle, ref.assetPath, ref.assetIndex);

		LoadSetMaterial(ref); // 또는 내부 갱신 함수

		RefreshDerivedAfterMaterialChanged(); // 여기서 텍스처 포함 파생 갱신
	}

	const MaterialHandle& GetMaterialHandle() const						  { return m_MaterialHandle;   }

	void LoadSetMaterial(const MaterialRef& materialRef)
	{
		m_Material = materialRef;
	}

	const MaterialRef& GetMaterial () const { return m_Material;  }


	void SetOverrides(const RenderData::MaterialData& overrides);
	void ClearOverrides();
	bool HasOverrides() const { return m_UseOverrides; }
	const RenderData::MaterialData& GetOverrides() const { return m_Overrides; }

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	void RefreshDerivedAfterMaterialChanged();

	MaterialHandle			 m_MaterialHandle;
	MaterialRef				 m_Material;
	RenderData::MaterialData m_Overrides;
	bool					 m_UseOverrides = false;
};



