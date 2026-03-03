#pragma once
#include "json.hpp"
#include <cstdint>
#include "assimp/anim.h"
#include "MathHelper.h"

using nlohmann::json;

template<typename T>
inline int FindKeyIndex(const T* keys, int keyCount, double time)
{
	// keys[i].mTime : 오름차순
	if (keyCount <= 0) return -1;
	if (time <= keys[0].mTime) return 0;	// clamp
	if (time >= keys[keyCount - 1].mTime) return keyCount - 1;

	int lo = 0, hi = keyCount - 1;
	while (lo + 1 < hi)
	{
		int mid = (lo + hi) / 2;
		if (keys[mid].mTime <= time) lo = mid;
		else hi = mid;
	}
	return lo;	// lo ~ lo + 1 구간
}

inline XMFLOAT3 SamplePos(const aiNodeAnim* anim, double tick)
{
	if (!anim || anim->mNumPositionKeys == 0) return XMFLOAT3(0, 0, 0);
	int i = FindKeyIndex(anim->mPositionKeys, static_cast<int>(anim->mNumPositionKeys), tick);
	if (i < 0) return XMFLOAT3(0, 0, 0);

	if (i == static_cast<int>(anim->mNumPositionKeys) - 1)
	{
		const auto& v = anim->mPositionKeys[i].mValue;
		return XMFLOAT3(v.x, v.y, v.z);
	}

	const auto& key0 = anim->mPositionKeys[i];
	const auto& key1 = anim->mPositionKeys[i + 1];

	double deltaTime = key1.mTime - key0.mTime;
	float a = (deltaTime > 0.0) ? static_cast<float>((tick - key0.mTime) / deltaTime) : 0.0f;

	XMFLOAT3 p0((float)key0.mValue.x, (float)key0.mValue.y, (float)key0.mValue.z);
	XMFLOAT3 p1((float)key1.mValue.x, (float)key1.mValue.y, (float)key1.mValue.z);
	return MathUtils::Lerp3(p0, p1, a);
}

inline XMFLOAT3 SampleScale(const aiNodeAnim* anim, double tick)
{
	if (!anim || anim->mNumScalingKeys == 0) return XMFLOAT3(0, 0, 0);
	int i = FindKeyIndex(anim->mScalingKeys, static_cast<int>(anim->mNumScalingKeys), tick);
	if (i < 0) return XMFLOAT3(0, 0, 0);

	if (i == static_cast<int>(anim->mNumScalingKeys) - 1)
	{
		const auto& v = anim->mScalingKeys[i].mValue;
		return XMFLOAT3(v.x, v.y, v.z);
	}

	const auto& key0 = anim->mScalingKeys[i];
	const auto& key1 = anim->mScalingKeys[i + 1];

	double deltaTime = key1.mTime - key0.mTime;
	float a = (deltaTime > 0.0) ? static_cast<float>((tick - key0.mTime) / deltaTime) : 0.0f;

	XMFLOAT3 s0((float)key0.mValue.x, (float)key0.mValue.y, (float)key0.mValue.z);
	XMFLOAT3 s1((float)key1.mValue.x, (float)key1.mValue.y, (float)key1.mValue.z);
	return MathUtils::Lerp3(s0, s1, a);
}

inline XMFLOAT4 SampleRot(const aiNodeAnim* anim, double tick)
{
	if (!anim || anim->mNumRotationKeys == 0) return XMFLOAT4(0, 0, 0, 1);
	int i = FindKeyIndex(anim->mRotationKeys, static_cast<int>(anim->mNumRotationKeys), tick);
	if (i < 0) return XMFLOAT4(0, 0, 0, 1);

	if (i == static_cast<int>(anim->mNumRotationKeys) - 1)
	{
		const auto& q = anim->mRotationKeys[i].mValue;
		return XMFLOAT4((float)q.x, (float)q.y, (float)q.z, (float)q.w);
	}

	const auto& key0 = anim->mRotationKeys[i];
	const auto& key1 = anim->mRotationKeys[i + 1];

	double deltaTime = key1.mTime - key0.mTime;
	float a = (deltaTime > 0.0) ? static_cast<float>((tick - key0.mTime) / deltaTime) : 0.0f;

	XMFLOAT4 r0((float)key0.mValue.x, (float)key0.mValue.y, (float)key0.mValue.z, (float)key0.mValue.w);
	XMFLOAT4 r1((float)key1.mValue.x, (float)key1.mValue.y, (float)key1.mValue.z, (float)key1.mValue.w);
	XMFLOAT4 out = MathUtils::Slerp4(r0, r1, a);
	XMVECTOR q = XMLoadFloat4(&out);
	q = XMQuaternionNormalize(q);
	XMStoreFloat4(&out, q);
	return out;
}

struct AnimationKeyFrame
{
	float time = 0.0f;
	XMFLOAT3 translation{ 0.0f, 0.0f, 0.0f };
	XMFLOAT4 rotation{ 0.0f, 0.0f, 0.0f, 0.0f };
	XMFLOAT3 scale{ 1.0f,1.0f,1.0f };
};

struct AnimationTrack
{
	int                            boneIndex = -1;
	std::vector<AnimationKeyFrame> keyFrames;
};

struct AnimationClip
{
	std::string name;
	float duration = 0.0f;
	float ticksPerSecond = 0.0f;
	std::vector<AnimationTrack> tracks;
};

struct aiScene;
inline std::string SanitizeName(const std::string& s);
bool ImportFBXToAnimJson(
	const aiScene* scene,
	const std::string& outDir,
	const std::string& baseName,
	const std::unordered_map<std::string, uint32_t>& boneNameToIndex,
	std::vector<std::string>& outAnimFiles);
