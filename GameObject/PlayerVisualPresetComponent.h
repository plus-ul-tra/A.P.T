#pragma once

#include "Component.h"
#include "ResourceHandle.h"
#include "ResourceRefs.h"

class PlayerVisualPresetComponent : public Component
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "PlayerVisualPresetComponent";
	const char* GetTypeName() const override;

	PlayerVisualPresetComponent() = default;
	virtual ~PlayerVisualPresetComponent() = default;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const bool& GetApplyOnStart() const { return m_ApplyOnStart; }
	void SetApplyOnStart(const bool& value) { m_ApplyOnStart = value; }

	const bool& GetApplyAnimationWithBlend() const { return m_ApplyAnimationWithBlend; }
	void SetApplyAnimationWithBlend(const bool& value) { m_ApplyAnimationWithBlend = value; }

	const std::string& GetCurrentStateTag() const { return m_CurrentStateTag; }
	void SetCurrentStateTag(const std::string& stateTag) { m_CurrentStateTag = stateTag; }

	const std::string& GetStateTag0() const { return m_StateTag0; }
	void SetStateTag0(const std::string& value) { m_StateTag0 = value; }
	const MeshHandle& GetMeshHandle0() const { return m_MeshHandle0; }
	void SetMeshHandle0(const MeshHandle& value);
	const MeshRef& GetMesh0() const { return m_Mesh0; }
	void SetMesh0(const MeshRef& value) { m_Mesh0 = value; }
	const SkeletonHandle& GetSkeletonHandle0() const { return m_SkeletonHandle0; }
	void SetSkeletonHandle0(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton0() const { return m_Skeleton0; }
	void SetSkeleton0(const SkeletonRef& value) { m_Skeleton0 = value; }
	const AnimationHandle& GetAnimationHandle0() const { return m_AnimationHandle0; }
	void SetAnimationHandle0(const AnimationHandle& value);
	const AnimationRef& GetAnimation0() const { return m_Animation0; }
	void SetAnimation0(const AnimationRef& value) { m_Animation0 = value; }
	const bool& GetUseAnimation0() const { return m_UseAnimation0; }
	void SetUseAnimation0(const bool& value) { m_UseAnimation0 = value; }
	const float& GetBlendTime0() const { return m_BlendTime0; }
	void SetBlendTime0(const float& value) { m_BlendTime0 = value; }

	const std::string& GetStateTag1() const { return m_StateTag1; }
	void SetStateTag1(const std::string& value) { m_StateTag1 = value; }
	const MeshHandle& GetMeshHandle1() const { return m_MeshHandle1; }
	void SetMeshHandle1(const MeshHandle& value);
	const MeshRef& GetMesh1() const { return m_Mesh1; }
	void SetMesh1(const MeshRef& value) { m_Mesh1 = value; }
	const SkeletonHandle& GetSkeletonHandle1() const { return m_SkeletonHandle1; }
	void SetSkeletonHandle1(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton1() const { return m_Skeleton1; }
	void SetSkeleton1(const SkeletonRef& value) { m_Skeleton1 = value; }
	const AnimationHandle& GetAnimationHandle1() const { return m_AnimationHandle1; }
	void SetAnimationHandle1(const AnimationHandle& value);
	const AnimationRef& GetAnimation1() const { return m_Animation1; }
	void SetAnimation1(const AnimationRef& value) { m_Animation1 = value; }
	const bool& GetUseAnimation1() const { return m_UseAnimation1; }
	void SetUseAnimation1(const bool& value) { m_UseAnimation1 = value; }
	const float& GetBlendTime1() const { return m_BlendTime1; }
	void SetBlendTime1(const float& value) { m_BlendTime1 = value; }

	const std::string& GetStateTag2() const { return m_StateTag2; }
	void SetStateTag2(const std::string& value) { m_StateTag2 = value; }
	const MeshHandle& GetMeshHandle2() const { return m_MeshHandle2; }
	void SetMeshHandle2(const MeshHandle& value);
	const MeshRef& GetMesh2() const { return m_Mesh2; }
	void SetMesh2(const MeshRef& value) { m_Mesh2 = value; }
	const SkeletonHandle& GetSkeletonHandle2() const { return m_SkeletonHandle2; }
	void SetSkeletonHandle2(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton2() const { return m_Skeleton2; }
	void SetSkeleton2(const SkeletonRef& value) { m_Skeleton2 = value; }
	const AnimationHandle& GetAnimationHandle2() const { return m_AnimationHandle2; }
	void SetAnimationHandle2(const AnimationHandle& value);
	const AnimationRef& GetAnimation2() const { return m_Animation2; }
	void SetAnimation2(const AnimationRef& value) { m_Animation2 = value; }
	const bool& GetUseAnimation2() const { return m_UseAnimation2; }
	void SetUseAnimation2(const bool& value) { m_UseAnimation2 = value; }
	const float& GetBlendTime2() const { return m_BlendTime2; }
	void SetBlendTime2(const float& value) { m_BlendTime2 = value; }

	const std::string& GetStateTag3() const { return m_StateTag3; }
	void SetStateTag3(const std::string& value) { m_StateTag3 = value; }
	const MeshHandle& GetMeshHandle3() const { return m_MeshHandle3; }
	void SetMeshHandle3(const MeshHandle& value);
	const MeshRef& GetMesh3() const { return m_Mesh3; }
	void SetMesh3(const MeshRef& value) { m_Mesh3 = value; }
	const SkeletonHandle& GetSkeletonHandle3() const { return m_SkeletonHandle3; }
	void SetSkeletonHandle3(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton3() const { return m_Skeleton3; }
	void SetSkeleton3(const SkeletonRef& value) { m_Skeleton3 = value; }
	const AnimationHandle& GetAnimationHandle3() const { return m_AnimationHandle3; }
	void SetAnimationHandle3(const AnimationHandle& value);
	const AnimationRef& GetAnimation3() const { return m_Animation3; }
	void SetAnimation3(const AnimationRef& value) { m_Animation3 = value; }
	const bool& GetUseAnimation3() const { return m_UseAnimation3; }
	void SetUseAnimation3(const bool& value) { m_UseAnimation3 = value; }
	const float& GetBlendTime3() const { return m_BlendTime3; }
	void SetBlendTime3(const float& value) { m_BlendTime3 = value; }

	const std::string& GetStateTag4() const { return m_StateTag4; }
	void SetStateTag4(const std::string& value) { m_StateTag4 = value; }
	const MeshHandle& GetMeshHandle4() const { return m_MeshHandle4; }
	void SetMeshHandle4(const MeshHandle& value);
	const MeshRef& GetMesh4() const { return m_Mesh4; }
	void SetMesh4(const MeshRef& value) { m_Mesh4 = value; }
	const SkeletonHandle& GetSkeletonHandle4() const { return m_SkeletonHandle4; }
	void SetSkeletonHandle4(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton4() const { return m_Skeleton4; }
	void SetSkeleton4(const SkeletonRef& value) { m_Skeleton4 = value; }
	const AnimationHandle& GetAnimationHandle4() const { return m_AnimationHandle4; }
	void SetAnimationHandle4(const AnimationHandle& value);
	const AnimationRef& GetAnimation4() const { return m_Animation4; }
	void SetAnimation4(const AnimationRef& value) { m_Animation4 = value; }
	const bool& GetUseAnimation4() const { return m_UseAnimation4; }
	void SetUseAnimation4(const bool& value) { m_UseAnimation4 = value; }
	const float& GetBlendTime4() const { return m_BlendTime4; }
	void SetBlendTime4(const float& value) { m_BlendTime4 = value; }

	const std::string& GetStateTag5() const { return m_StateTag5; }
	void SetStateTag5(const std::string& value) { m_StateTag5 = value; }
	const MeshHandle& GetMeshHandle5() const { return m_MeshHandle5; }
	void SetMeshHandle5(const MeshHandle& value);
	const MeshRef& GetMesh5() const { return m_Mesh5; }
	void SetMesh5(const MeshRef& value) { m_Mesh5 = value; }
	const SkeletonHandle& GetSkeletonHandle5() const { return m_SkeletonHandle5; }
	void SetSkeletonHandle5(const SkeletonHandle& value);
	const SkeletonRef& GetSkeleton5() const { return m_Skeleton5; }
	void SetSkeleton5(const SkeletonRef& value) { m_Skeleton5 = value; }
	const AnimationHandle& GetAnimationHandle5() const { return m_AnimationHandle5; }
	void SetAnimationHandle5(const AnimationHandle& value);
	const AnimationRef& GetAnimation5() const { return m_Animation5; }
	void SetAnimation5(const AnimationRef& value) { m_Animation5 = value; }
	const bool& GetUseAnimation5() const { return m_UseAnimation5; }
	void SetUseAnimation5(const bool& value) { m_UseAnimation5 = value; }
	const float& GetBlendTime5() const { return m_BlendTime5; }
	void SetBlendTime5(const float& value) { m_BlendTime5 = value; }

	bool ApplyByStateTag(const std::string& stateTag);
	bool ApplyCurrentState();

private:
	bool ApplySlot(int slotIndex);
	bool TryApplySlotByTag(const std::string& stateTag);

private:
	bool m_ApplyOnStart = true;
	bool m_ApplyAnimationWithBlend = true;
	std::string m_CurrentStateTag = "Default";

	std::string m_StateTag0 = "Default";
	MeshHandle m_MeshHandle0 = MeshHandle::Invalid();
	MeshRef m_Mesh0;
	SkeletonHandle m_SkeletonHandle0 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton0;
	AnimationHandle m_AnimationHandle0 = AnimationHandle::Invalid();
	AnimationRef m_Animation0;
	bool m_UseAnimation0 = false;
	float m_BlendTime0 = 0.f;

	std::string m_StateTag1;
	MeshHandle m_MeshHandle1 = MeshHandle::Invalid();
	MeshRef m_Mesh1;
	SkeletonHandle m_SkeletonHandle1 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton1;
	AnimationHandle m_AnimationHandle1 = AnimationHandle::Invalid();
	AnimationRef m_Animation1;
	bool m_UseAnimation1 = false;
	float m_BlendTime1 = 0.f;

	std::string m_StateTag2;
	MeshHandle m_MeshHandle2 = MeshHandle::Invalid();
	MeshRef m_Mesh2;
	SkeletonHandle m_SkeletonHandle2 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton2;
	AnimationHandle m_AnimationHandle2 = AnimationHandle::Invalid();
	AnimationRef m_Animation2;
	bool m_UseAnimation2 = false;
	float m_BlendTime2 = 0.f;

	std::string m_StateTag3;
	MeshHandle m_MeshHandle3 = MeshHandle::Invalid();
	MeshRef m_Mesh3;
	SkeletonHandle m_SkeletonHandle3 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton3;
	AnimationHandle m_AnimationHandle3 = AnimationHandle::Invalid();
	AnimationRef m_Animation3;
	bool m_UseAnimation3 = false;
	float m_BlendTime3 = 0.f;

	std::string m_StateTag4;
	MeshHandle m_MeshHandle4 = MeshHandle::Invalid();
	MeshRef m_Mesh4;
	SkeletonHandle m_SkeletonHandle4 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton4;
	AnimationHandle m_AnimationHandle4 = AnimationHandle::Invalid();
	AnimationRef m_Animation4;
	bool m_UseAnimation4 = false;
	float m_BlendTime4 = 0.f;

	std::string m_StateTag5;
	MeshHandle m_MeshHandle5 = MeshHandle::Invalid();
	MeshRef m_Mesh5;
	SkeletonHandle m_SkeletonHandle5 = SkeletonHandle::Invalid();
	SkeletonRef m_Skeleton5;
	AnimationHandle m_AnimationHandle5 = AnimationHandle::Invalid();
	AnimationRef m_Animation5;
	bool m_UseAnimation5 = false;
	float m_BlendTime5 = 0.f;
};
