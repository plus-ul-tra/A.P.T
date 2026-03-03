#include "AnimationComponent.h"

#include <algorithm>
#include <cmath>
#include <fstream>

#include "AssetLoader.h"
#include "MathHelper.h"
#include "Object.h"
#include "SkeletalMeshComponent.h"
#include "ReflectionMacro.h"

REGISTER_COMPONENT(AnimationComponent);
REGISTER_PROPERTY_HANDLE(AnimationComponent, ClipHandle)
REGISTER_PROPERTY_READONLY(AnimationComponent, Animation)
REGISTER_PROPERTY(AnimationComponent, Playback)
REGISTER_PROPERTY_READONLY(AnimationComponent, Blend)
REGISTER_PROPERTY(AnimationComponent, BlendConfig)
REGISTER_PROPERTY_READONLY(AnimationComponent, BoneMaskWeights)
REGISTER_PROPERTY_READONLY(AnimationComponent, RetargetOffsets)
REGISTER_PROPERTY_READONLY(AnimationComponent, BoneMaskSource)
REGISTER_PROPERTY_READONLY(AnimationComponent, BoneMaskWeight)
REGISTER_PROPERTY_READONLY(AnimationComponent, BoneMaskDefaultWeight)
REGISTER_PROPERTY_READONLY(AnimationComponent, AutoBoneMaskApplied)
REGISTER_PROPERTY_READONLY(AnimationComponent, LocalPose)
REGISTER_PROPERTY_READONLY(AnimationComponent, GlobalPose)
REGISTER_PROPERTY_READONLY(AnimationComponent, SkinningPalette)

#ifdef _DEBUG
namespace
{
	bool MatrixHasNonFinite(const DirectX::XMFLOAT4X4& matrix)
	{
		const float* data = reinterpret_cast<const float*>(&matrix);
		for (int i = 0; i < 16; ++i)
		{
			if (!std::isfinite(data[i]))
				return true;
		}
		return false;
	}

	void AppendSkinningPaletteDebug(
		const Object* owner,
		const RenderData::Skeleton& skeleton,
		const RenderData::AnimationClip* clip,
		const char* reason)
	{
		std::ofstream ofs("skinning_palette.debug.txt", std::ios::app);
		if (!ofs)
			return;

		const std::string objectName = owner ? owner->GetName() : std::string("unknown");
		ofs << "object=" << objectName
			<< " reason=" << (reason ? reason : "unknown")
			<< " bones=" << skeleton.bones.size()
			<< " clip=" << (clip ? clip->name : "none")
			<< "\n";
	}
}
#else
	namespace
{
	void AppendSkinningPaletteDebug(
		const Object*,
		const RenderData::Skeleton&,
		const RenderData::AnimationClip*,
		const char*)
	{
	}
}
#endif

namespace
{
	// LocalPose 기본값이 0이면(scale=0, rot=0) TRS가 망가짐.
	// 기본을 T=0, R=identity, S=1로 강제.
	AnimationComponent::LocalPose MakeIdentityLocalPose()
	{
		AnimationComponent::LocalPose p{};
		p.translation = { 0.0f, 0.0f, 0.0f };
		p.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
		p.scale = { 1.0f, 1.0f, 1.0f };
		return p;
	}


	AnimationComponent::LocalPose MakeLocalPoseFromMatrix(const DirectX::XMFLOAT4X4& matrix)
	{
		AnimationComponent::LocalPose pose{};

		DirectX::XMVECTOR scale{};
		DirectX::XMVECTOR rotation{};
		DirectX::XMVECTOR translation{};
		if (DirectX::XMMatrixDecompose(&scale, &rotation, &translation, DirectX::XMLoadFloat4x4(&matrix)))
		{
			DirectX::XMStoreFloat3(&pose.translation, translation);
			DirectX::XMStoreFloat4(&pose.rotation, DirectX::XMQuaternionNormalize(rotation));
			DirectX::XMStoreFloat3(&pose.scale, scale);
		}
		else
		{
			pose.translation = { 0.0f, 0.0f, 0.0f };
			pose.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };
			pose.scale = { 1.0f, 1.0f, 1.0f };
		}

		return pose;
	}


	void BuildBindPosePalette(const RenderData::Skeleton& skeleton, std::vector<DirectX::XMFLOAT4X4>& outPalette)
	{
		const size_t boneCount = skeleton.bones.size();
		outPalette.resize(boneCount);
		std::vector<DirectX::XMFLOAT4X4> globalPose(boneCount);

		const auto globalInvRoot = DirectX::XMLoadFloat4x4(&skeleton.globalInverseTransform);

		for (size_t i = 0; i < boneCount; ++i)
		{
			const auto local = DirectX::XMLoadFloat4x4(&skeleton.bones[i].bindPose);
			const int parentIndex = skeleton.bones[i].parentIndex;

			if (parentIndex >= 0 && static_cast<size_t>(parentIndex) < boneCount)
			{
				const auto parent = DirectX::XMLoadFloat4x4(&globalPose[static_cast<size_t>(parentIndex)]);
				const auto global = MathUtils::Mul(local, parent); // local * parent (네 방식)
				DirectX::XMStoreFloat4x4(&globalPose[i], global);
			}
			else
			{
				DirectX::XMStoreFloat4x4(&globalPose[i], local);
			}

			const auto global = DirectX::XMLoadFloat4x4(&globalPose[i]);
			const auto invBind = DirectX::XMLoadFloat4x4(&skeleton.bones[i].inverseBindPose);

			// 핵심: root 보정 포함
			const auto globalFixed = MathUtils::Mul(global, globalInvRoot);   // global * globalInvRoot
			const auto skin = MathUtils::Mul(invBind, globalFixed);    // invBind * globalFixed

			DirectX::XMStoreFloat4x4(&outPalette[i], skin);
		}
	}

	float Clamp01(float v)
	{
		return std::clamp(v, 0.0f, 1.0f);
	}
}

// 클립 유효 범위 내로 시간을 강제
float ClampTimeToClip(float timeSec, const RenderData::AnimationClip* clip)
{
	if (!clip || clip->duration <= 0.0f)
		return timeSec;

	if (timeSec < 0.0f)
		return 0.0f;

	if (timeSec > clip->duration)
		return clip->duration;

	return timeSec;
}

// deltaTime을 적용한 다음 재생 시간을 계산
float UpdatePlaybackTime(
	float timeSec,
	float deltaTime,
	const RenderData::AnimationClip* clip,
	bool looping,
	bool* stopped)
{
	float nextTime = timeSec + deltaTime;
	if (!clip || clip->duration <= 0.0f)
		return nextTime;

	if (looping)
	{
		nextTime = fmod(nextTime, clip->duration);
		if (nextTime < 0.0f)
			nextTime += clip->duration;
	}
	else
	{
		if (nextTime >= clip->duration)
		{
			nextTime = clip->duration;
			if (stopped) *stopped = true;
		}
		else if (nextTime < 0.0f)
		{
			nextTime = 0.0f;
		}
	}

	return nextTime;
}

void AnimationComponent::Play()
{
	m_Playback.playing = true;
}

void AnimationComponent::Stop()
{
	m_Playback.time = 0.0f;
	m_Playback.playing = false;
	ApplyStaticPoseToSkeletal();
}

void AnimationComponent::Pause()
{
	m_Playback.playing = false;
}

void AnimationComponent::Resume()
{
	m_Playback.playing = true;
}

void AnimationComponent::SeekTime(float timeSec)
{
	const RenderData::AnimationClip* clip = ResolveClip();
	m_Playback.time = ClampTimeToClip(timeSec, clip);
}

void AnimationComponent::SeekNormalized(float normalizedTime)
{
	const RenderData::AnimationClip* clip = ResolveClip();
	if (!clip || clip->duration <= 0.0f)
	{
		m_Playback.time = 0.0f;
		return;
	}

	// FIX: normalizedTime을 clip 시간 범위로 Clamp하면 안됨.
	// 0~1로 clamp 후 duration 곱.
	const float t = std::clamp(normalizedTime, 0.0f, 1.0f);
	m_Playback.time = ClampTimeToClip(t * clip->duration, clip);
}

float AnimationComponent::GetNormalizedTime() const
{
	const RenderData::AnimationClip* clip = ResolveClip();
	if (!clip || clip->duration <= 0.0f)
		return 0.0f;

	return m_Playback.time / clip->duration;
}

void AnimationComponent::StartBlend(AnimationHandle toClip, float blendTime)
{
	const RenderData::AnimationClip* fromClip = ResolveClip(m_ClipHandle);
	const RenderData::AnimationClip* nextClip = ResolveClip(toClip);
	if (!nextClip)
		return;

	if (!fromClip || blendTime <= 0.0f)
	{
		m_Blend.active = false;
		m_ClipHandle = toClip;
		m_Playback.time = 0.0f;
		m_Playback.playing = true;
		return;
	}

	m_Blend.active = true;
	m_Blend.fromClip = m_ClipHandle;
	m_Blend.toClip = toClip;
	m_Blend.duration = max(blendTime, 0.0001f);
	m_Blend.elapsed = 0.0f;
	m_Blend.fromTime = m_Playback.time;
	m_Blend.toTime = 0.0f;

	m_ClipHandle = toClip;
	m_Playback.playing = true;
}

void AnimationComponent::StartBlend(
	AnimationHandle toClip,
	float blendTime,
	BlendType blendType,
	std::function<float(float)> curveFn)
{
	const RenderData::AnimationClip* fromClip = ResolveClip(m_ClipHandle);
	const RenderData::AnimationClip* nextClip = ResolveClip(toClip);
	if (!nextClip)
		return;

	if (!fromClip || blendTime <= 0.0f)
	{
		m_Blend.active = false;
		m_ClipHandle = toClip;
		m_Playback.time = 0.0f;
		m_Playback.playing = true;
		return;
	}

	m_Blend.active = true;
	m_Blend.fromClip = m_ClipHandle;
	m_Blend.toClip = toClip;
	m_Blend.duration = max(blendTime, 0.0001f);
	m_Blend.elapsed = 0.0f;
	m_Blend.fromTime = m_Playback.time;
	m_Blend.toTime = 0.0f;
	m_Blend.blendType = blendType;
	m_Blend.curveFn = std::move(curveFn);

	m_ClipHandle = toClip;
	m_Playback.playing = true;
}

void AnimationComponent::SetBoneMask(const std::vector<float>& weights)
{
	m_BoneMaskWeights = weights;
	m_BoneMaskSource = BoneMaskSource::None;
	m_AutoBoneMaskApplied = true;
}

void AnimationComponent::SetBoneMaskFromIndices(size_t boneCount, const std::vector<int>& indices, float weight, float defaultWeight)
{
	m_BoneMaskWeights.assign(boneCount, defaultWeight);
	for (int index : indices)
	{
		if (index < 0)
			continue;

		const size_t boneIndex = static_cast<size_t>(index);
		if (boneIndex < m_BoneMaskWeights.size())
		{
			m_BoneMaskWeights[boneIndex] = weight;
		}
	}
	m_BoneMaskSource = BoneMaskSource::None;
	m_AutoBoneMaskApplied = true;
}

void AnimationComponent::ClearBoneMask()
{
	m_BoneMaskWeights.clear();
	m_BoneMaskSource = BoneMaskSource::None;
	m_AutoBoneMaskApplied = false;
}

void AnimationComponent::SetRetargetOffsets(const std::vector<RetargetOffset>& offsets)
{
	m_RetargetOffsets.clear();
	m_RetargetOffsets.reserve(offsets.size());
	for (const auto& offset : offsets)
	{
		m_RetargetOffsets.push_back(ToLocalPose(offset));
	}
}

void AnimationComponent::ClearRetargetOffsets()
{
	m_RetargetOffsets.clear();
}

void AnimationComponent::Start()
{
	EnsureResourceStores();
	ApplyStaticPoseToSkeletal();
}

void AnimationComponent::EnsureResourceStores()
{
	if (m_Skeletons && m_Animations)
		return;

	if (auto* loader = AssetLoader::GetActive())
	{
		m_Skeletons = &loader->GetSkeletons();
		m_Animations = &loader->GetAnimations();
	}
}

void AnimationComponent::RefreshDerivedAfterClipChanged()
{
	m_Blend = BlendState{};

	const RenderData::AnimationClip* clip = ResolveClip();
	m_Playback.time = 0.0f;
	m_Playback.playing = (clip != nullptr);

	if (m_BoneMaskSource != BoneMaskSource::None)
	{
		m_AutoBoneMaskApplied = false;
		m_BoneMaskWeights.clear();
	}

	m_LocalPose.clear();
	m_GlobalPose.clear();
	m_SkinningPalette.clear();

	Object* owner = GetOwner();
	if (!owner) return;

	auto* skeletal = owner->GetComponent<SkeletalMeshComponent>();
	if (!skeletal) return;

	const RenderData::Skeleton* skel = ResolveSkeleton(skeletal->GetSkeletonHandle());
	if (!skel || skel->bones.empty())
		return;

	if (!clip)
	{
		BuildBindPosePalette(*skel, m_SkinningPalette);
		ApplyPoseToSkeletal(skeletal);
		return;
	}

	EnsureAutoBoneMask(*skel);
	BuildPose(*skel, *clip, m_Playback.time);
	ApplyPoseToSkeletal(skeletal);
}

void AnimationComponent::ApplyStaticPoseToSkeletal()
{
	Object* owner = GetOwner();
	if (!owner)
		return;

	auto* skeletal = owner->GetComponent<SkeletalMeshComponent>();
	if (!skeletal)
		return;

	const RenderData::Skeleton* skeleton = ResolveSkeleton(skeletal->GetSkeletonHandle());
	if (!skeleton || skeleton->bones.empty())
		return;

	const RenderData::AnimationClip* clip = ResolveClip();
	if (!clip)
	{
		if (m_SkinningPalette.empty())
			BuildBindPosePalette(*skeleton, m_SkinningPalette);
	}
	else
	{
		EnsureAutoBoneMask(*skeleton);
		m_Playback.time = ClampTimeToClip(m_Playback.time, clip);
		BuildPose(*skeleton, *clip, m_Playback.time);
	}

	ApplyPoseToSkeletal(skeletal);
}

void AnimationComponent::SetRetargetFromBindPose(
	const std::vector<DirectX::XMFLOAT4X4>& sourceBind,
	const std::vector<DirectX::XMFLOAT4X4>& targetBind)
{
	// FIX: max()로 잡고 양쪽 벡터를 그대로 접근하면 범위 밖 접근 크래시.
	const size_t boneCount = min(sourceBind.size(), targetBind.size());
	if (boneCount == 0)
	{
		ClearRetargetOffsets();
		return;
	}

	std::vector<RetargetOffset> offsets;
	offsets.reserve(boneCount);

	for (size_t i = 0; i < boneCount; ++i)
	{
		const auto source = DirectX::XMLoadFloat4x4(&sourceBind[i]);
		const auto target = DirectX::XMLoadFloat4x4(&targetBind[i]);
		const auto sourceInv = DirectX::XMMatrixInverse(nullptr, source);
		const auto offsetMatrix = MathUtils::Mul(target, sourceInv);

		DirectX::XMVECTOR scale, rotation, translation;
		const BOOL ok = DirectX::XMMatrixDecompose(&scale, &rotation, &translation, offsetMatrix);
		if (!ok)
		{
			scale = DirectX::XMVectorSet(1, 1, 1, 0);
			rotation = DirectX::XMQuaternionIdentity();
			translation = DirectX::XMVectorZero();
		}

		RetargetOffset offset{};
		DirectX::XMStoreFloat3(&offset.translation, translation);
		DirectX::XMStoreFloat4(&offset.rotation, rotation);
		DirectX::XMStoreFloat3(&offset.scale, scale);
		offsets.push_back(offset);
	}

	SetRetargetOffsets(offsets);
}

void AnimationComponent::SetRetargetFromSkeletonHandles(SkeletonHandle sourceHandle, SkeletonHandle targetHandle)
{
	const RenderData::Skeleton* sourceSkeleton = ResolveSkeleton(sourceHandle);
	const RenderData::Skeleton* targetSkeleton = ResolveSkeleton(targetHandle);
	if (!sourceSkeleton || !targetSkeleton)
		return;

	const size_t boneCount = min(sourceSkeleton->bones.size(), targetSkeleton->bones.size());
	std::vector<DirectX::XMFLOAT4X4> sourceBind;
	std::vector<DirectX::XMFLOAT4X4> targetBind;
	sourceBind.reserve(boneCount);
	targetBind.reserve(boneCount);

	for (size_t i = 0; i < boneCount; ++i)
	{
		sourceBind.push_back(sourceSkeleton->bones[i].bindPose);
		targetBind.push_back(targetSkeleton->bones[i].bindPose);
	}

	SetRetargetFromBindPose(sourceBind, targetBind);
}

void AnimationComponent::UseSkeletonUpperBodyMask(float weight, float defaultWeight)
{
	m_BoneMaskSource = BoneMaskSource::UpperBody;
	m_BoneMaskWeight = weight;
	m_BoneMaskDefaultWeight = defaultWeight;
	m_AutoBoneMaskApplied = false;
}

void AnimationComponent::UseSkeletonLowerBodyMask(float weight, float defaultWeight)
{
	m_BoneMaskSource = BoneMaskSource::LowerBody;
	m_BoneMaskWeight = weight;
	m_BoneMaskDefaultWeight = defaultWeight;
	m_AutoBoneMaskApplied = false;
}

void AnimationComponent::ClearSkeletonMask()
{
	m_BoneMaskSource = BoneMaskSource::None;
	m_AutoBoneMaskApplied = false;
}

void AnimationComponent::Update(float deltaTime)
{
	EnsureResourceStores();
	if (!m_Playback.playing)
	{
		ApplyStaticPoseToSkeletal();
		return;
	}

	Object* owner = GetOwner();
	if (!owner)
		return;

	auto* skeletal = owner->GetComponent<SkeletalMeshComponent>();
	if (!skeletal)
		return;

	const RenderData::Skeleton* skeleton = ResolveSkeleton(skeletal->GetSkeletonHandle());
	if (!skeleton || skeleton->bones.empty())
		return;

	EnsureAutoBoneMask(*skeleton);

	if (m_Blend.active)
	{
		const RenderData::AnimationClip* fromClip = ResolveClip(m_Blend.fromClip);
		const RenderData::AnimationClip* toClip = ResolveClip(m_Blend.toClip);
		if (!fromClip || !toClip)
		{
			m_Blend.active = false;
		}
		else
		{
			const float dir = m_Playback.reverse ? -1.0f : 1.0f;
			const float scaledDelta = deltaTime * m_Playback.speed * dir;
			m_Blend.fromTime = UpdatePlaybackTime(m_Blend.fromTime, scaledDelta, fromClip, m_Playback.looping, nullptr);
			m_Blend.toTime = UpdatePlaybackTime(m_Blend.toTime, scaledDelta, toClip, m_Playback.looping, nullptr);
			m_Blend.elapsed += deltaTime;

			const float linear = min(m_Blend.elapsed / m_Blend.duration, 1.0f);
			float alpha = linear;

			if (m_Blend.blendType == BlendType::Curve && m_Blend.curveFn)
			{
				alpha = clamp(m_Blend.curveFn(linear), 0.0f, 1.0f);
			}

			std::vector<LocalPose> fromPoses;
			std::vector<LocalPose> toPoses;
			std::vector<LocalPose> blendedPoses;

			SampleLocalPoses(*skeleton, *fromClip, m_Blend.fromTime, fromPoses);
			SampleLocalPoses(*skeleton, *toClip, m_Blend.toTime, toPoses);
			BlendLocalPoses(fromPoses, toPoses, alpha, blendedPoses);
			ApplyRetargetOffsets(blendedPoses);
			BuildPoseFromLocal(*skeleton, blendedPoses);

			m_Playback.time = m_Blend.toTime;

			if (alpha >= 1.0f)
				m_Blend.active = false;
		}
	}
	else
	{
		const RenderData::AnimationClip* clip = ResolveClip();
		if (!clip)
			return;

		bool stopped = false;
		const float dir = m_Playback.reverse ? -1.0f : 1.0f;
		const float scaledDelta = deltaTime * m_Playback.speed * dir;
		const float nextTime = UpdatePlaybackTime(m_Playback.time, scaledDelta, clip, m_Playback.looping, &stopped);

		m_Playback.time = nextTime;
		if (stopped)
			m_Playback.playing = false;

		BuildPose(*skeleton, *clip, m_Playback.time);
	}

	ApplyPoseToSkeletal(skeletal);
}

void AnimationComponent::ApplyPoseToSkeletal(SkeletalMeshComponent* skeletal)
{
	(void)skeletal;
}

void AnimationComponent::OnEvent(EventType type, const void* data)
{
}

bool AnimationComponent::AdvancePlayback(float deltaTime)
{
	if (!m_Playback.playing)
		return false;

	const RenderData::AnimationClip* clip = ResolveClip();
	if (!clip)
		return false;

	bool stopped = false;
	const float dir = m_Playback.reverse ? -1.0f : 1.0f;
	const float scaledDelta = deltaTime * m_Playback.speed * dir;
	const float nextTime = UpdatePlaybackTime(m_Playback.time, scaledDelta, clip, m_Playback.looping, &stopped);

	m_Playback.time = nextTime;
	if (stopped)
		m_Playback.playing = false;

	return true;
}

const RenderData::AnimationClip* AnimationComponent::GetActiveClip() const
{
	return ResolveClip();
}

bool AnimationComponent::SampleTrackForBone(const RenderData::AnimationClip& clip, int boneIndex, float timeSec, LocalPose& outPose)
{
	for (const auto& track : clip.tracks)
	{
		if (track.boneIndex != boneIndex || track.keyFrames.empty())
			continue;

		outPose = SampleTrack(track, timeSec);
		return true;
	}

	return false;
}

bool AnimationComponent::SampleTrackByIndex(const RenderData::AnimationClip& clip, size_t trackIndex, float timeSec, LocalPose& outPose)
{
	if (trackIndex >= clip.tracks.size())
		return false;

	const auto& track = clip.tracks[trackIndex];
	if (track.keyFrames.empty())
		return false;

	outPose = SampleTrack(track, timeSec);
	return true;
}

AnimationComponent::LocalPose AnimationComponent::ToLocalPose(const RetargetOffset& offset)
{
	AnimationComponent::LocalPose pose = MakeIdentityLocalPose();
	pose.translation = offset.translation;
	pose.rotation = offset.rotation;
	pose.scale = offset.scale;
	return pose;
}

AnimationComponent::LocalPose AnimationComponent::ToLocalPose(const RenderData::AnimationKeyFrame& key)
{
	AnimationComponent::LocalPose pose = MakeIdentityLocalPose();
	pose.translation = key.translation;
	pose.rotation = key.rotation;
	pose.scale = key.scale;
	return pose;
}

const RenderData::AnimationClip* AnimationComponent::ResolveClip() const
{
	if (!m_ClipHandle.IsValid() || !m_Animations)
		return nullptr;

	return m_Animations->Get(m_ClipHandle);
}

const RenderData::AnimationClip* AnimationComponent::ResolveClip(AnimationHandle handle) const
{
	if (!handle.IsValid() || !m_Animations)
		return nullptr;

	return m_Animations->Get(handle);
}

const RenderData::Skeleton* AnimationComponent::ResolveSkeleton(SkeletonHandle handle) const
{
	if (!handle.IsValid() || !m_Skeletons)
		return nullptr;

	return m_Skeletons->Get(handle);
}

AnimationComponent::LocalPose AnimationComponent::SampleTrack(const RenderData::AnimationTrack& track, float timeSec)
{
	const auto& keys = track.keyFrames;
	if (keys.empty())
		return MakeIdentityLocalPose();

	if (keys.size() == 1 || timeSec <= keys.front().time)
		return ToLocalPose(keys.front());

	if (timeSec >= keys.back().time)
		return ToLocalPose(keys.back());

	auto it = std::upper_bound(keys.begin(), keys.end(), timeSec,
		[](float value, const RenderData::AnimationKeyFrame& key)
		{
			return value < key.time;
		});

	if (it == keys.begin())
		return ToLocalPose(*it);

	const size_t curIndex = static_cast<size_t>(std::distance(keys.begin(), it));
	const size_t prevIndex = curIndex - 1;
	const auto& prevKey = keys[prevIndex];
	const auto& curKey = keys[curIndex];

	const float term = curKey.time - prevKey.time;
	const float t = term > 0.0f ? (timeSec - prevKey.time) / term : 0.0f;

	LocalPose pose = MakeIdentityLocalPose();
	pose.translation = MathUtils::Lerp3(prevKey.translation, curKey.translation, t);
	pose.scale = MathUtils::Lerp3(prevKey.scale, curKey.scale, t);
	pose.rotation = MathUtils::Slerp4(prevKey.rotation, curKey.rotation, t);

	const auto q = DirectX::XMQuaternionNormalize(XMLoadFloat4(&pose.rotation));
	DirectX::XMStoreFloat4(&pose.rotation, q);

	return pose;
}

void AnimationComponent::SampleLocalPoses(
	const RenderData::Skeleton& skeleton,
	const RenderData::AnimationClip& clip,
	float timeSec,
	std::vector<LocalPose>& localPoses) const
{
	const size_t boneCount = skeleton.bones.size();
	localPoses.resize(boneCount);
	for (size_t i = 0; i < boneCount; ++i)
	{
		localPoses[i] = MakeLocalPoseFromMatrix(skeleton.bones[i].bindPose);
	}

	for (const auto& track : clip.tracks)
	{
		if (track.boneIndex < 0 || static_cast<size_t>(track.boneIndex) >= boneCount)
		{
#ifdef _DEBUG
			//AppendSkinningPaletteDebug(GetOwner(), skeleton, &clip, "track_bone_index_out_of_range");
#endif
			continue;
		}

		if (track.keyFrames.empty())
		{
			continue;
		}

		localPoses[static_cast<size_t>(track.boneIndex)] = SampleTrack(track, timeSec);
	}
}

void AnimationComponent::BuildPose(const RenderData::Skeleton& skeleton, const RenderData::AnimationClip& clip, float timeSec)
{
	std::vector<LocalPose> localPoses;
	SampleLocalPoses(skeleton, clip, timeSec, localPoses);
	ApplyRetargetOffsets(localPoses);
	BuildPoseFromLocal(skeleton, localPoses);
}

void AnimationComponent::BuildPoseFromLocal(const RenderData::Skeleton& skeleton, const std::vector<LocalPose>& localPoses)
{
	const size_t boneCount = skeleton.bones.size();
	m_LocalPose.resize(boneCount);
	m_GlobalPose.resize(boneCount);
	m_SkinningPalette.resize(boneCount);

	const size_t poseCount = localPoses.size();
	const auto globalInvRoot = DirectX::XMLoadFloat4x4(&skeleton.globalInverseTransform);

	for (size_t i = 0; i < boneCount; ++i)
	{
		const LocalPose& pose = (i < poseCount) ? localPoses[i] : MakeIdentityLocalPose();
		const auto local = MathUtils::CreateTRS(pose.translation, pose.rotation, pose.scale);
		DirectX::XMStoreFloat4x4(&m_LocalPose[i], local);

		const int parentIndex = skeleton.bones[i].parentIndex;
		if (parentIndex >= 0 && static_cast<size_t>(parentIndex) < boneCount)
		{
			const auto parent = DirectX::XMLoadFloat4x4(&m_GlobalPose[static_cast<size_t>(parentIndex)]);
			const auto global = MathUtils::Mul(local, parent); // local * parent (네 방식)
			DirectX::XMStoreFloat4x4(&m_GlobalPose[i], global);
		}
		else
		{
			DirectX::XMStoreFloat4x4(&m_GlobalPose[i], local);
		}

		const auto global = DirectX::XMLoadFloat4x4(&m_GlobalPose[i]);
		const auto invBind = DirectX::XMLoadFloat4x4(&skeleton.bones[i].inverseBindPose);

		// 핵심: root 보정 포함
		const auto globalFixed = MathUtils::Mul(global, globalInvRoot);   // global * globalInvRoot
		const auto skin = MathUtils::Mul(invBind, globalFixed);    // invBind * globalFixed

		DirectX::XMStoreFloat4x4(&m_SkinningPalette[i], skin);
	}

#ifdef _DEBUG
	// for (const auto& paletteMatrix : m_SkinningPalette)
	// {
	// 	if (MatrixHasNonFinite(paletteMatrix))
	// 	{
	// 		AppendSkinningPaletteDebug(GetOwner(), skeleton, ResolveClip(), "non_finite_palette_matrix");
	// 		break;
	// 	}
	// }
#endif
}

void AnimationComponent::BlendLocalPoses(
	const std::vector<LocalPose>& fromPoses,
	const std::vector<LocalPose>& toPoses,
	float alpha,
	std::vector<LocalPose>& blended) const
{
	const size_t boneCount = max(fromPoses.size(), toPoses.size());
	blended.resize(boneCount);

	const float clampedAlpha = clamp(alpha, 0.0f, 1.0f);

	for (size_t i = 0; i < boneCount; ++i)
	{
		const LocalPose& fromPose = (i < fromPoses.size()) ? fromPoses[i] : MakeIdentityLocalPose();
		const LocalPose& toPose = (i < toPoses.size()) ? toPoses[i] : MakeIdentityLocalPose();

		const float mask = (i < m_BoneMaskWeights.size()) ? m_BoneMaskWeights[i] : 1.0f;
		const float weightedAlpha = std::clamp(clampedAlpha * mask, 0.0f, 1.0f);

		blended[i] = MakeIdentityLocalPose();
		blended[i].translation = MathUtils::Lerp3(fromPose.translation, toPose.translation, weightedAlpha);
		blended[i].scale = MathUtils::Lerp3(fromPose.scale, toPose.scale, weightedAlpha);
		blended[i].rotation = MathUtils::Slerp4(fromPose.rotation, toPose.rotation, weightedAlpha);

		const auto q = XMQuaternionNormalize(XMLoadFloat4(&blended[i].rotation));
		XMStoreFloat4(&blended[i].rotation, q);
	}
}

void AnimationComponent::ApplyRetargetOffsets(std::vector<LocalPose>& localPoses) const
{
	if (m_RetargetOffsets.empty())
		return;

	const size_t boneCount = min(localPoses.size(), m_RetargetOffsets.size());
	for (size_t i = 0; i < boneCount; ++i)
	{
		const LocalPose& offset = m_RetargetOffsets[i];

		localPoses[i].translation.x += offset.translation.x;
		localPoses[i].translation.y += offset.translation.y;
		localPoses[i].translation.z += offset.translation.z;

		const auto baseRotation = DirectX::XMLoadFloat4(&localPoses[i].rotation);
		const auto offsetRotation = DirectX::XMLoadFloat4(&offset.rotation);
		const auto combined = DirectX::XMQuaternionNormalize(DirectX::XMQuaternionMultiply(offsetRotation, baseRotation));
		DirectX::XMStoreFloat4(&localPoses[i].rotation, combined);

		localPoses[i].scale.x *= offset.scale.x;
		localPoses[i].scale.y *= offset.scale.y;
		localPoses[i].scale.z *= offset.scale.z;
	}
}

void AnimationComponent::EnsureAutoBoneMask(const RenderData::Skeleton& skeleton)
{
	if (m_BoneMaskSource == BoneMaskSource::None)
		return;

	// FIX: skeleton이 바뀌어서 bone count가 달라졌는데 AutoApplied=true라서 갱신 안되는 케이스 방지
	if (m_AutoBoneMaskApplied && m_BoneMaskWeights.size() == skeleton.bones.size())
		return;

	const auto& indices = (m_BoneMaskSource == BoneMaskSource::UpperBody)
		? skeleton.upperBodyBones
		: skeleton.lowerBodyBones;

	if (indices.empty())
		return;

	SetBoneMaskFromIndices(skeleton.bones.size(), indices, m_BoneMaskWeight, m_BoneMaskDefaultWeight);
	m_AutoBoneMaskApplied = true;
}
