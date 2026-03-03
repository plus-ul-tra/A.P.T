#pragma once

#include "Component.h"
#include "RenderData.h"
#include "ResourceHandle.h"
#include "ResourceStore.h"
#include <functional>
#include "ResourceRefs.h"
#include "AssetLoader.h"

class SkeletalMeshComponent;

class AnimationComponent : public Component
{
	friend class Editor;
	friend class SkeletalMeshComponent;

public:
	enum class BlendType
	{
		Linear,
		Curve
	};

	enum class BoneMaskSource
	{
		None,
		UpperBody,
		LowerBody
	};

	using BlendCurveFn = std::function<float(float)>; // 0~1 -> 0~1

	struct PlaybackState
	{
		float time    = 0.0f;
		float speed   = 1.0f;
		bool  looping = true;
		bool  playing = true;
		bool  reverse = false;
	};

	struct RetargetOffset
	{
		DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 rotation   { 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 scale      { 1.0f, 1.0f, 1.0f };
	};

	struct BlendState
	{
		bool active = false;
		AnimationHandle fromClip = AnimationHandle::Invalid();
		AnimationHandle toClip = AnimationHandle::Invalid();
		float duration = 0.0f;
		float elapsed = 0.0f;
		float fromTime = 0.0f;
		float toTime = 0.0f;
		BlendType blendType = BlendType::Linear;
		BlendCurveFn curveFn = nullptr;	// Curve일 때만 유효
	};

	struct BlendConfig
	{
		AnimationHandle fromClip = AnimationHandle::Invalid();
		AnimationHandle toClip = AnimationHandle::Invalid();
		float blendTime = 0.2f;
		BlendType blendType = BlendType::Linear;
		std::string curveName = "Linear";
	};
	struct LocalPose
	{
		DirectX::XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
		DirectX::XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 1.0f };
		DirectX::XMFLOAT3 scale{ 1.0f, 1.0f, 1.0f };
	};

private:


public:
	static constexpr const char* StaticTypeName = "AnimationComponent";
	const char* GetTypeName() const override;

	AnimationComponent() = default;
	virtual ~AnimationComponent() = default;

	void SetClipHandle(const AnimationHandle& handle)  
	{
		if (m_ClipHandle == handle)
			return;

		m_ClipHandle = handle; 
		EnsureResourceStores();

		// 파생값 갱신 (읽기전용 표시용)
		AnimationRef ref{};
		if (auto* loader = AssetLoader::GetActive())
			loader->GetAnimationAssetReference(handle, ref.assetPath, ref.assetIndex);

		LoadSetAnimation(ref); // 또는 내부 갱신 함수
		
		// 클립 바뀌면 파생 상태를 일관되게 정리/재계산
		RefreshDerivedAfterClipChanged();
	}

	const AnimationHandle& GetClipHandle()			   { return m_ClipHandle;   }

	/// 재생을 시작/재개한다.
	/// playing 플래그만 true로 변경한다.
	void Play();

	/// 재생을 정지하고 시간을 0으로 되돌린다.
	/// playing=false, time=0 설정.
	void Stop();

	/// 재생을 일시정지한다.
	/// time은 유지되고 playing=false로 변경된다.
	void Pause();

	/// 일시정지 상태에서 재생을 재개한다.
	/// playing=true로 변경된다.
	void Resume();

	/// 현재 클립의 재생 시간을 초 단위로 설정한다.
	/// @param timeSec 설정할 시간(초). 내부에서 [0, duration]으로 clamp된다.
	void SeekTime(float timeSec);

	/// 현재 클립의 재생 시간을 정규화 시간으로 설정한다.
	/// @param normalizedTime 0~1 범위의 정규화 시간. 내부에서 clamp 후 duration을 곱해 time으로 변환한다.
	void SeekNormalized(float normalizedTime);

	/// 현재 클립의 재생 시간을 정규화 값(0~1)으로 반환한다.
	/// 클립이 없거나 duration<=0이면 0을 반환한다.
	float GetNormalizedTime() const;

	/// 선형(Linear) 블렌드로 다른 클립으로 전이한다.
	/// - fromClip이 없거나 blendTime<=0이면 즉시 전환한다.
	/// - 블렌드 진행 중에도 현재 클립 핸들은 toClip으로 설정된다.
	/// @param toClip    블렌드 대상 클립 핸들
	/// @param blendTime 블렌드 시간(초)
	void StartBlend(AnimationHandle toClip, float blendTime);

	/// Curve 블렌드로 다른 클립으로 전이한다.
	/// - blendType이 Curve이면 curveFn으로 alpha를 보정한다.
	/// - curveFn은 [0,1] 입력 → [0,1] 출력으로 가정한다. (결과는 내부에서 clamp)
	/// - fromClip이 없거나 blendDurationSec<=0이면 즉시 전환한다.
	/// @param toClip             블렌드 대상 클립 핸들
	/// @param blendDurationSec   블렌드 시간(초)
	/// @param blendType          블렌드 방식(Linear/Curve)
	/// @param curveFn            Curve일 때 사용할 곡선 함수(없으면 Linear 동작)
	void StartBlend(AnimationHandle toClip,
		float blendDurationSec,
		BlendType blendType,
		std::function<float(float)> curveFn);

	/// 본 마스크 가중치를 설정한다.
	/// @param weights 본 인덱스별 가중치(0~1). BlendLocalPoses에서 alpha에 곱해진다.
	void SetBoneMask(const std::vector<float>& weights);

	void SetBoneMaskFromIndices(size_t boneCount,
								const std::vector<int>& indices,
								float weight = 1.0f,
								float defaultWeight = 0.0f);

	/// 본 마스크를 제거한다. (모든 본 weight=1.0 동작)
	void ClearBoneMask();

	/// 리타게팅 오프셋을 설정한다.
	/// 각 본에 대해 translation/rotation/scale 오프셋을 누적 적용한다.
	void SetRetargetOffsets(const std::vector<RetargetOffset>& offsets);

	/// 리타게팅 오프셋을 제거한다.
	void ClearRetargetOffsets();

	
	void SetRetargetFromSkeletonHandles(SkeletonHandle sourceHandle, SkeletonHandle targetHandle);

	void UseSkeletonUpperBodyMask  (float weight = 1.0f, float defaultWeight = 0.0f);
	void UseSkeletonLowerBodyMask  (float weight = 1.0f, float defaultWeight = 0.0f);
	void ClearSkeletonMask		   ();

	void LoadSetAnimation(const AnimationRef& animationRef) { m_Animation = animationRef; }
	const AnimationRef& GetAnimation() const			    { return m_Animation;		  }

	void SetResourceStores(ResourceStore<RenderData::Skeleton, SkeletonHandle>* skeletons,
						   ResourceStore<RenderData::AnimationClip, AnimationHandle>* animations)
	{
		m_Skeletons = skeletons;
		m_Animations = animations;
	}

	void SetPlayback(const PlaybackState& playback) { m_Playback = playback; }
	const PlaybackState& GetPlayback() const			{ return m_Playback;	 }

	const BlendState& GetBlend() const { return m_Blend; }

	void SetBlendConfig(const BlendConfig& config)
	{
		m_BlendConfig = config;
		m_Blend.fromClip = config.fromClip;
		m_Blend.toClip = config.toClip;
		m_Blend.duration = config.blendTime;
		m_Blend.blendType = config.blendType;
	}

	const BlendConfig& GetBlendConfig() const { return m_BlendConfig; }


	void LoadSetBoneMaskWeights(const std::vector<float>& boneMaskWeights) { m_BoneMaskWeights = boneMaskWeights; }
	const std::vector<float>& GetBoneMaskWeights() const { return m_BoneMaskWeights; }
	void LoadSetRetargetOffsets(const std::vector<LocalPose>& retargetOffset) { m_RetargetOffsets = retargetOffset; }
	const std::vector<LocalPose>& GetRetargetOffsets() const { return m_RetargetOffsets; }
	void LoadSetBoneMaskSource(const BoneMaskSource& boneMaskSource) { m_BoneMaskSource = boneMaskSource; }
	const BoneMaskSource& GetBoneMaskSource() const { return m_BoneMaskSource; }
	void LoadSetBoneMaskWeight(const float& boneMaskWeight) { m_BoneMaskWeight = boneMaskWeight; }
	const float& GetBoneMaskWeight() const { return m_BoneMaskWeight; }
	void LoadSetBoneMaskDefaultWeight(const float& boneMaskDefaultWeight) { m_BoneMaskDefaultWeight = boneMaskDefaultWeight; }
	const float& GetBoneMaskDefaultWeight() const { return m_BoneMaskDefaultWeight; }
	const bool& GetAutoBoneMaskApplied() const { return m_AutoBoneMaskApplied; }
	
	const std::vector<DirectX::XMFLOAT4X4>& GetLocalPose      () const { return m_LocalPose;       }
	const std::vector<DirectX::XMFLOAT4X4>& GetGlobalPose     () const { return m_GlobalPose;      }
	const std::vector<DirectX::XMFLOAT4X4>& GetSkinningPalette() const { return m_SkinningPalette; }

	
	void Start() override;
	/// 애니메이션을 업데이트하고 스키닝 팔레트를 계산한다.
	/// - playing=false면 아무 것도 하지 않는다.
	/// - 블렌드 중이면 from/to 포즈를 샘플링 후 alpha로 블렌딩한다.
	/// - 결과 스키닝 팔레트를 SkeletalMeshComponent에 전달한다.
	/// @param deltaTime 프레임 델타 시간(초)
	void Update(float deltaTime) override;

	/// 컴포넌트 이벤트를 처리한다. (현재 구현은 비어있음)
	void OnEvent(EventType type, const void* data) override;

protected:
	bool AdvancePlayback(float deltaTime);
	const RenderData::AnimationClip* GetActiveClip() const;
	static bool SampleTrackForBone(const RenderData::AnimationClip& clip, int boneIndex, float timeSec, LocalPose& outPose);
	static bool SampleTrackByIndex(const RenderData::AnimationClip& clip, size_t trackIndex, float timeSec, LocalPose& outPose);
	virtual void ApplyPoseToSkeletal(SkeletalMeshComponent* skeletal);

private:
	void EnsureResourceStores();
	void RefreshDerivedAfterClipChanged();
	void ApplyStaticPoseToSkeletal();

	void SetRetargetFromBindPose(const std::vector<DirectX::XMFLOAT4X4>& sourceBind,
							     const std::vector<DirectX::XMFLOAT4X4>& targetBind);

	/// 리타게팅 오프셋을 LocalPose로 변환한다.
	static LocalPose ToLocalPose(const RetargetOffset& offset);

	/// 키프레임을 LocalPose로 변환한다.
	static LocalPose ToLocalPose(const RenderData::AnimationKeyFrame& key);

	/// 현재 m_ClipHandle 기준으로 클립을 조회한다. 유효하지 않으면 nullptr.
	const RenderData::AnimationClip* ResolveClip() const;

	/// 지정 핸들 기준으로 클립을 조회한다. 유효하지 않으면 nullptr.
	const RenderData::AnimationClip* ResolveClip(AnimationHandle handle) const;

	/// 스켈레톤을 조회한다. 유효하지 않으면 nullptr.
	const RenderData::Skeleton* ResolveSkeleton(SkeletonHandle handle) const;

	/// 트랙에서 특정 시점의 로컬 포즈를 샘플링한다.
	/// 키프레임 사이를 보간하며 rotation은 Slerp 후 정규화한다.
	static LocalPose SampleTrack(const RenderData::AnimationTrack& track, float timeSec);

	/// 클립의 모든 트랙을 샘플링하여 boneIndex별 로컬 포즈 배열을 채운다.
	void SampleLocalPoses(const RenderData::Skeleton& skeleton,
	                      const RenderData::AnimationClip& clip,
	                      float timeSec,
	                      std::vector<LocalPose>& localPoses) const;

	/// 로컬 포즈를 샘플링 → 리타게팅 적용 → 글로벌/스키닝 행렬을 만든다.
	void BuildPose(const RenderData::Skeleton& skeleton,
	               const RenderData::AnimationClip& clip,
	               float timeSec);

	/// 로컬 포즈 배열로부터 글로벌 포즈 및 스키닝 팔레트를 계산한다.
	/// skin = globalPose * inverseBindPose
	void BuildPoseFromLocal(const RenderData::Skeleton& skeleton,
	                        const std::vector<LocalPose>& localPoses);

	/// from/to 로컬 포즈를 alpha로 블렌드한다. 본 마스크가 있으면 alpha에 곱해 적용한다.
	void BlendLocalPoses(const std::vector<LocalPose>& fromPoses,
	                     const std::vector<LocalPose>& toPoses,
	                     float alpha,
	                     std::vector<LocalPose>& blended) const;

	/// 리타게팅 오프셋을 로컬 포즈에 적용한다.
	/// rotation은 offset * base 순서로 결합한다.
	void ApplyRetargetOffsets(std::vector<LocalPose>& localPoses) const;

	void EnsureAutoBoneMask		  (const RenderData::Skeleton& skeleton);
private:
	ResourceStore<RenderData::Skeleton, SkeletonHandle>*       m_Skeletons  = nullptr;
	ResourceStore<RenderData::AnimationClip, AnimationHandle>* m_Animations = nullptr;

	AnimationHandle                  m_ClipHandle = AnimationHandle::Invalid();
	AnimationRef					 m_Animation;
						             
	PlaybackState                    m_Playback{};
	BlendState                       m_Blend{};
	BlendConfig                      m_BlendConfig{};
	std::vector<float>               m_BoneMaskWeights;
	std::vector<LocalPose>           m_RetargetOffsets;
	BoneMaskSource					 m_BoneMaskSource			  = BoneMaskSource::None;
	float							 m_BoneMaskWeight			  = 1.0f;
	float						     m_BoneMaskDefaultWeight	  = 0.0f;
	bool						     m_AutoBoneMaskApplied		  = false;
	


	std::vector<DirectX::XMFLOAT4X4> m_LocalPose;
	std::vector<DirectX::XMFLOAT4X4> m_GlobalPose;
	std::vector<DirectX::XMFLOAT4X4> m_SkinningPalette;
};

