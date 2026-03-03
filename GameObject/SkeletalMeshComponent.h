#pragma once

#include "MeshComponent.h"
#include "RenderData.h"
#include "ResourceHandle.h"
#include "ResourceRefs.h"


class SkeletalMeshComponent : public MeshComponent
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "SkeletalMeshComponent";
	const char* GetTypeName() const override;

	SkeletalMeshComponent() = default;
	virtual ~SkeletalMeshComponent() = default;

	void  SetSkeletonHandle(const SkeletonHandle& handle);
	const SkeletonHandle& GetSkeletonHandle() const                { return m_SkeletonHandle;   }

	void LoadSetSkeleton(const SkeletonRef& skeletonRef) { m_Skeleton = skeletonRef; }
	const SkeletonRef& GetSkeleton() const				  { return m_Skeleton;        }

	void LoadSetSkinningPalette(const std::vector<DirectX::XMFLOAT4X4>& palette)
	{
		m_SkinningPalette = palette;
	}

	const std::vector<DirectX::XMFLOAT4X4>& GetSkinningPalette() const { return m_SkinningPalette; }
	bool                                    HasSkinningPalette() const { return !m_SkinningPalette.empty(); }

	void Update (float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

protected:
	SkeletonHandle m_SkeletonHandle = SkeletonHandle::Invalid();

	SkeletonRef    m_Skeleton;

	std::vector<DirectX::XMFLOAT4X4> m_SkinningPalette;
};

