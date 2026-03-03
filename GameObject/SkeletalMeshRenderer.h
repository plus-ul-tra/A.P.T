#pragma once

#include "MeshRenderer.h"
#include "RenderData.h"
#include "ResourceRefs.h"

class TransformComponent;

class SkeletalMeshRenderer : public MeshRenderer
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "SkeletalMeshRenderer";
	const char* GetTypeName() const override;
	
	SkeletalMeshRenderer() = default;
	virtual ~SkeletalMeshRenderer() = default;

	void LoadSetSkeleton(const SkeletonRef& skeletonRef) { m_Skeleton = skeletonRef; }
	const SkeletonRef& GetSkeleton() const				  { return m_Skeleton;        }

	bool BuildRenderItem(RenderData::RenderItem& out) override;

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

private:
	void ResolveHandles(MeshHandle& mesh, MaterialHandle& material, SkeletonHandle& skeleton);

protected:
	SkeletonHandle m_SkeletonHandle  = SkeletonHandle::Invalid();
	SkeletonRef    m_Skeleton;
};

