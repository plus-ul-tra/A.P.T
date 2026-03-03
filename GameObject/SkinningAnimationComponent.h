#pragma once

#include "AnimationComponent.h"

class SkinningAnimationComponent : public AnimationComponent
{
public:
	static constexpr const char* StaticTypeName = "SkinningAnimationComponent";
	const char* GetTypeName() const override;

	SkinningAnimationComponent() = default;
	virtual ~SkinningAnimationComponent() = default;

protected:
	void ApplyPoseToSkeletal(SkeletalMeshComponent* skeletal) override;
};