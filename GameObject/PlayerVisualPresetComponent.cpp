#include "PlayerVisualPresetComponent.h"

#include "ReflectionMacro.h"
#include "Object.h"
#include "AssetLoader.h"
#include "SkeletalMeshComponent.h"
#include "SkinningAnimationComponent.h"

REGISTER_COMPONENT(PlayerVisualPresetComponent)
REGISTER_PROPERTY(PlayerVisualPresetComponent, ApplyOnStart)
REGISTER_PROPERTY(PlayerVisualPresetComponent, ApplyAnimationWithBlend)
REGISTER_PROPERTY(PlayerVisualPresetComponent, CurrentStateTag)

REGISTER_PROPERTY(PlayerVisualPresetComponent, StateTag0)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, MeshHandle0)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Mesh0)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, SkeletonHandle0)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Skeleton0)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, AnimationHandle0)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Animation0)
REGISTER_PROPERTY(PlayerVisualPresetComponent, UseAnimation0)
REGISTER_PROPERTY(PlayerVisualPresetComponent, BlendTime0)

REGISTER_PROPERTY(PlayerVisualPresetComponent, StateTag1)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, MeshHandle1)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Mesh1)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, SkeletonHandle1)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Skeleton1)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, AnimationHandle1)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Animation1)
REGISTER_PROPERTY(PlayerVisualPresetComponent, UseAnimation1)
REGISTER_PROPERTY(PlayerVisualPresetComponent, BlendTime1)

REGISTER_PROPERTY(PlayerVisualPresetComponent, StateTag2)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, MeshHandle2)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Mesh2)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, SkeletonHandle2)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Skeleton2)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, AnimationHandle2)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Animation2)
REGISTER_PROPERTY(PlayerVisualPresetComponent, UseAnimation2)
REGISTER_PROPERTY(PlayerVisualPresetComponent, BlendTime2)

REGISTER_PROPERTY(PlayerVisualPresetComponent, StateTag3)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, MeshHandle3)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Mesh3)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, SkeletonHandle3)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Skeleton3)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, AnimationHandle3)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Animation3)
REGISTER_PROPERTY(PlayerVisualPresetComponent, UseAnimation3)
REGISTER_PROPERTY(PlayerVisualPresetComponent, BlendTime3)

REGISTER_PROPERTY(PlayerVisualPresetComponent, StateTag4)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, MeshHandle4)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Mesh4)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, SkeletonHandle4)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Skeleton4)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, AnimationHandle4)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Animation4)
REGISTER_PROPERTY(PlayerVisualPresetComponent, UseAnimation4)
REGISTER_PROPERTY(PlayerVisualPresetComponent, BlendTime4)

REGISTER_PROPERTY(PlayerVisualPresetComponent, StateTag5)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, MeshHandle5)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Mesh5)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, SkeletonHandle5)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Skeleton5)
REGISTER_PROPERTY_HANDLE(PlayerVisualPresetComponent, AnimationHandle5)
REGISTER_PROPERTY(PlayerVisualPresetComponent, Animation5)
REGISTER_PROPERTY(PlayerVisualPresetComponent, UseAnimation5)
REGISTER_PROPERTY(PlayerVisualPresetComponent, BlendTime5)

void PlayerVisualPresetComponent::Start()
{
	if (m_ApplyOnStart)
	{
		ApplyCurrentState();
	}
}

void PlayerVisualPresetComponent::Update(float deltaTime)
{
}

void PlayerVisualPresetComponent::OnEvent(EventType type, const void* data)
{
}

void PlayerVisualPresetComponent::SetMeshHandle0(const MeshHandle& value)
{
	m_MeshHandle0 = value;

	MeshRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetMeshAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Mesh0 = ref;
}

void PlayerVisualPresetComponent::SetSkeletonHandle0(const SkeletonHandle& value)
{
	m_SkeletonHandle0 = value;

	SkeletonRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetSkeletonAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Skeleton0 = ref;
}

void PlayerVisualPresetComponent::SetAnimationHandle0(const AnimationHandle& value)
{
	m_AnimationHandle0 = value;

	AnimationRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetAnimationAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Animation0 = ref;
}

void PlayerVisualPresetComponent::SetMeshHandle1(const MeshHandle& value)
{
	m_MeshHandle1 = value;

	MeshRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetMeshAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Mesh1 = ref;
}

void PlayerVisualPresetComponent::SetSkeletonHandle1(const SkeletonHandle& value)
{
	m_SkeletonHandle1 = value;

	SkeletonRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetSkeletonAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Skeleton1 = ref;
}

void PlayerVisualPresetComponent::SetAnimationHandle1(const AnimationHandle& value)
{
	m_AnimationHandle1 = value;

	AnimationRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetAnimationAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Animation1 = ref;
}

void PlayerVisualPresetComponent::SetMeshHandle2(const MeshHandle& value)
{
	m_MeshHandle2 = value;

	MeshRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetMeshAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Mesh2 = ref;
}

void PlayerVisualPresetComponent::SetSkeletonHandle2(const SkeletonHandle& value)
{
	m_SkeletonHandle2 = value;

	SkeletonRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetSkeletonAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Skeleton2 = ref;
}

void PlayerVisualPresetComponent::SetAnimationHandle2(const AnimationHandle& value)
{
	m_AnimationHandle2 = value;

	AnimationRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetAnimationAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Animation2 = ref;
}

void PlayerVisualPresetComponent::SetMeshHandle3(const MeshHandle& value)
{
	m_MeshHandle3 = value;

	MeshRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetMeshAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Mesh3 = ref;
}

void PlayerVisualPresetComponent::SetSkeletonHandle3(const SkeletonHandle& value)
{
	m_SkeletonHandle3 = value;

	SkeletonRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetSkeletonAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Skeleton3 = ref;
}

void PlayerVisualPresetComponent::SetAnimationHandle3(const AnimationHandle& value)
{
	m_AnimationHandle3 = value;

	AnimationRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetAnimationAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Animation3 = ref;
}

void PlayerVisualPresetComponent::SetMeshHandle4(const MeshHandle& value)
{
	m_MeshHandle4 = value;

	MeshRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetMeshAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Mesh4 = ref;
}

void PlayerVisualPresetComponent::SetSkeletonHandle4(const SkeletonHandle& value)
{
	m_SkeletonHandle4 = value;

	SkeletonRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetSkeletonAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Skeleton4 = ref;
}

void PlayerVisualPresetComponent::SetAnimationHandle4(const AnimationHandle& value)
{
	m_AnimationHandle4 = value;

	AnimationRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetAnimationAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Animation4 = ref;
}

void PlayerVisualPresetComponent::SetMeshHandle5(const MeshHandle& value)
{
	m_MeshHandle5 = value;

	MeshRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetMeshAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Mesh5 = ref;
}

void PlayerVisualPresetComponent::SetSkeletonHandle5(const SkeletonHandle& value)
{
	m_SkeletonHandle5 = value;

	SkeletonRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetSkeletonAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Skeleton5 = ref;
}

void PlayerVisualPresetComponent::SetAnimationHandle5(const AnimationHandle& value)
{
	m_AnimationHandle5 = value;

	AnimationRef ref{};
	if (auto* loader = AssetLoader::GetActive())
		loader->GetAnimationAssetReference(value, ref.assetPath, ref.assetIndex);

	m_Animation5 = ref;
}


bool PlayerVisualPresetComponent::ApplyByStateTag(const std::string& stateTag)
{
	if (stateTag.empty())
	{
		return false;
	}

	m_CurrentStateTag = stateTag;
	return TryApplySlotByTag(stateTag);
}

bool PlayerVisualPresetComponent::ApplyCurrentState()
{
	if (TryApplySlotByTag(m_CurrentStateTag))
	{
		return true;
	}

	if (!m_StateTag0.empty() && m_StateTag0 != m_CurrentStateTag)
	{
		return TryApplySlotByTag(m_StateTag0);
	}

	return false;
}

bool PlayerVisualPresetComponent::TryApplySlotByTag(const std::string& stateTag)
{
	if (stateTag == m_StateTag0)
	{
		return ApplySlot(0);
	}

	if (stateTag == m_StateTag1)
	{
		return ApplySlot(1);
	}

	if (stateTag == m_StateTag2)
	{
		return ApplySlot(2);
	}

	if (stateTag == m_StateTag3)
	{
		return ApplySlot(3);
	}

	if (stateTag == m_StateTag4)
	{
		return ApplySlot(4);
	}

	if (stateTag == m_StateTag5)
	{
		return ApplySlot(5);
	}

	return false;
}

bool PlayerVisualPresetComponent::ApplySlot(int slotIndex)
{
	Object* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	auto* skeletal = owner->GetComponent<SkeletalMeshComponent>();
	if (!skeletal)
	{
		return false;
	}

	auto* loader = AssetLoader::GetActive();
	if (!loader)
	{
		return false;
	}

	const MeshRef* meshRef = nullptr;
	const SkeletonRef* skeletonRef = nullptr;
	const AnimationRef* animationRef = nullptr;
	MeshHandle meshHandle = MeshHandle::Invalid();
	SkeletonHandle skeletonHandle = SkeletonHandle::Invalid();
	AnimationHandle animationHandle = AnimationHandle::Invalid();
	bool useAnimation = false;
	float blendTime = 0.0f;

	switch (slotIndex)
	{
	case 0:
		meshRef = &m_Mesh0;
		meshHandle = m_MeshHandle0;
		skeletonRef = &m_Skeleton0;
		skeletonHandle = m_SkeletonHandle0;
		animationRef = &m_Animation0;
		animationHandle = m_AnimationHandle0;
		useAnimation = m_UseAnimation0;
		blendTime = m_BlendTime0;
		break;
	case 1:
		meshRef = &m_Mesh1;
		meshHandle = m_MeshHandle1;
		skeletonRef = &m_Skeleton1;
		skeletonHandle = m_SkeletonHandle1;
		animationRef = &m_Animation1;
		animationHandle = m_AnimationHandle1;
		useAnimation = m_UseAnimation1;
		blendTime = m_BlendTime1;
		break;
	case 2:
		meshRef = &m_Mesh2;
		meshHandle = m_MeshHandle2;
		skeletonRef = &m_Skeleton2;
		skeletonHandle = m_SkeletonHandle2;
		animationRef = &m_Animation2;
		animationHandle = m_AnimationHandle2;
		useAnimation = m_UseAnimation2;
		blendTime = m_BlendTime2;
		break;
	case 3:
		meshRef = &m_Mesh3;
		meshHandle = m_MeshHandle3;
		skeletonRef = &m_Skeleton3;
		skeletonHandle = m_SkeletonHandle3;
		animationRef = &m_Animation3;
		animationHandle = m_AnimationHandle3;
		useAnimation = m_UseAnimation3;
		blendTime = m_BlendTime3;
		break;
	case 4:
		meshRef = &m_Mesh4;
		meshHandle = m_MeshHandle4;
		skeletonRef = &m_Skeleton4;
		skeletonHandle = m_SkeletonHandle4;
		animationRef = &m_Animation4;
		animationHandle = m_AnimationHandle4;
		useAnimation = m_UseAnimation4;
		blendTime = m_BlendTime4;
		break;
	case 5:
		meshRef = &m_Mesh5;
		meshHandle = m_MeshHandle5;
		skeletonRef = &m_Skeleton5;
		skeletonHandle = m_SkeletonHandle5;
		animationRef = &m_Animation5;
		animationHandle = m_AnimationHandle5;
		useAnimation = m_UseAnimation5;
		blendTime = m_BlendTime5;
		break;
	default:
		return false;
	}

	if (!meshRef || !skeletonRef)
	{
		return false;
	}

	if (meshHandle.IsValid())
	{
		skeletal->SetMeshHandle(meshHandle);
	}
	else if (!meshRef->assetPath.empty())
	{
		const MeshHandle mesh = loader->ResolveMesh(meshRef->assetPath, meshRef->assetIndex);
		if (!mesh.IsValid())
		{
			return false;
		}
		skeletal->SetMeshHandle(mesh);
	}

	if (skeletonHandle.IsValid())
	{
		skeletal->SetSkeletonHandle(skeletonHandle);
	}
	else if (!skeletonRef->assetPath.empty())
	{
		const SkeletonHandle skeleton = loader->ResolveSkeleton(skeletonRef->assetPath, skeletonRef->assetIndex);
		if (!skeleton.IsValid())
		{
			return false;
		}
		skeletal->SetSkeletonHandle(skeleton);
	}

	if (useAnimation)
	{
		auto* skinningAnim = owner->GetComponent<SkinningAnimationComponent>();
		if (skinningAnim)
		{
			AnimationHandle clip = animationHandle;
			if (!clip.IsValid() && animationRef && !animationRef->assetPath.empty())
			{
				clip = loader->ResolveAnimation(animationRef->assetPath, animationRef->assetIndex);
			}

			if (clip.IsValid())
			{
				if (m_ApplyAnimationWithBlend)
				{
					skinningAnim->StartBlend(clip, blendTime);
				}
				else
				{
					skinningAnim->SetClipHandle(clip);
				}
			}
		}
	}

	return true;
}