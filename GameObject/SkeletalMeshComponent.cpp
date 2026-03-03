#include "SkeletalMeshComponent.h"
#include "AnimationComponent.h"
#include "Object.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT_DERIVED(SkeletalMeshComponent, MeshComponent)
REGISTER_PROPERTY_HANDLE(SkeletalMeshComponent, SkeletonHandle)
REGISTER_PROPERTY_READONLY_LOADABLE(SkeletalMeshComponent, Skeleton)
REGISTER_PROPERTY_READONLY(SkeletalMeshComponent, SkinningPalette)

void SkeletalMeshComponent::Update(float deltaTime)
{
}

void SkeletalMeshComponent::SetSkeletonHandle(const SkeletonHandle& handle)
{
	m_SkeletonHandle = handle;

	// 파생값 갱신 (읽기전용 표시용)
	SkeletonRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetSkeletonAssetReference(handle, ref.assetPath, ref.assetIndex);

	LoadSetSkeleton(ref);

	m_SkinningPalette.clear();

	if (auto* owner = GetOwner())
	{
		const auto anims = owner->GetComponentsDerived<AnimationComponent>();
		if (!anims.empty())
		{
			anims.front()->RefreshDerivedAfterClipChanged();
		}
	}
}


void SkeletalMeshComponent::OnEvent(EventType type, const void* data)
{
}

