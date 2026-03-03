#pragma once
#include "Component.h"
#include "RenderData.h"
#include "ResourceRefs.h"

class TransformComponent;

class MeshRenderer : public Component
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "MeshRenderer";
	const char* GetTypeName() const override;
	
	MeshRenderer() = default;
	virtual ~MeshRenderer() = default;

	void LoadSetMesh(const MeshRef& meshRef)
	{
		m_Mesh = meshRef;
	}

	const MeshRef& GetMesh() const { return m_Mesh; }


	void LoadSetMaterial(const MaterialRef& materialRef)
	{
		m_Material = materialRef;
	}

	const MaterialRef& GetMaterial() const { return m_Material; }


	void  SetRenderLayer(const UINT8& layer);
	const UINT8& GetRenderLayer() const;

	void		SetVisible(const bool& visible) { m_Visible = visible; }
	const bool& GetVisible() const		 { return m_Visible;    }

	void		SetCastShadow(const bool& castShadow) { m_CastShadow = castShadow; }
	const bool& GetCastShadow() const		   { return m_CastShadow;	    }

	void		SetReceiveShadow(const bool& receiveShadow) { m_ReceiveShadow = receiveShadow; }
	const bool& GetReceiveShadow() const		     { return m_ReceiveShadow;	        }

	virtual bool BuildRenderItem(RenderData::RenderItem& out);

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

private:
	void ResolveHandles(MeshHandle& mesh, MaterialHandle& material);

protected:
	static UINT64 BuildSortKey(MeshHandle mesh, MaterialHandle material, UINT8 layer);
	TransformComponent* GetTransform() const;

	MeshHandle     m_MeshHandle      = MeshHandle::Invalid();
	MaterialHandle m_MaterialHandle  = MaterialHandle::Invalid();
	MeshRef		   m_Mesh;
	MaterialRef	   m_Material;
	bool  m_Visible                  = true;
	bool  m_CastShadow               = true;
	bool  m_ReceiveShadow            = true;
	UINT8 m_RenderLayer              = 0;
};

