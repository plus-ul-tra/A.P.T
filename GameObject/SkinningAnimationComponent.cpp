#include "SkinningAnimationComponent.h"
#include "ReflectionMacro.h"
#include "SkeletalMeshComponent.h"

REGISTER_COMPONENT_DERIVED(SkinningAnimationComponent, AnimationComponent)

void SkinningAnimationComponent::ApplyPoseToSkeletal(SkeletalMeshComponent* skeletal)
{
	if (!skeletal)
		return;

	skeletal->LoadSetSkinningPalette(GetSkinningPalette());
}
