#include "pch.h"
#include "assimp/scene.h"
#include "AnimJson.h"

inline std::string SanitizeName(const std::string& s)
{
	if (s.empty()) return "Anim";
	std::string out = s;
	for (char& c : out)
	{
		if (c == ':' || c == '/' || c == '\\' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|')
			c = '_';
	}
	return out;
}

static void CollectUnionTimesTicks(const aiNodeAnim* anim, std::vector<double>& outTimes)
{
	outTimes.clear();

	if (!anim) return;

	outTimes.reserve(anim->mNumPositionKeys + anim->mNumRotationKeys + anim->mNumScalingKeys);

	for (uint32_t i = 0; i < anim->mNumPositionKeys; ++i) outTimes.push_back(anim->mPositionKeys[i].mTime);
	for (uint32_t i = 0; i < anim->mNumRotationKeys; ++i) outTimes.push_back(anim->mRotationKeys[i].mTime);
	for (uint32_t i = 0; i < anim->mNumScalingKeys; ++i) outTimes.push_back(anim->mScalingKeys[i].mTime);

	std::sort(outTimes.begin(), outTimes.end());
	outTimes.erase(std::unique(outTimes.begin(), outTimes.end()), outTimes.end());
}

static json KeyFrameToJson(const AnimationKeyFrame& keyFrame)
{
	return json{
		{"time",	  keyFrame.time},
		{"position", {keyFrame.translation.x, keyFrame.translation.y, keyFrame.translation.z}},
		{"rotation", {keyFrame.rotation.x,    keyFrame.rotation.y,	  keyFrame.rotation.z,     keyFrame.rotation.w}},
		{"scale",	 {keyFrame.scale.x,       keyFrame.scale.y,		  keyFrame.scale.z}}
	};
}

static json TrackToJson(const AnimationTrack& track)
{
	json keys = json::array();

	for (const auto& keyFrame : track.keyFrames) keys.push_back(KeyFrameToJson(keyFrame));

	return json{
		{"boneIndex", track.boneIndex},
		{"keys",	  keys}
	};
}

static json ClipToJson(const AnimationClip& clip)
{
	json tracks = json::array();

	for (const auto& track : clip.tracks) tracks.push_back(TrackToJson(track));

	return json{
		{"name",		   clip.name},
		{"duration",	   clip.duration},
		{"ticksPerSecond", clip.ticksPerSecond},
		{"tracks",		   tracks}
	};
}


#ifdef _DEBUG
static void WriteAnimationDebug(
	const std::string& outPath,
	const AnimationClip& clip,
	const std::vector<std::string>& missingBones,
	uint32_t skippedEmptyTracks)
{
	std::ofstream ofs(outPath + ".debug.txt");
	if (!ofs)
		return;

	size_t totalKeys = 0;
	for (const auto& track : clip.tracks)
		totalKeys += track.keyFrames.size();

	ofs << "name=" << clip.name << "\n";
	ofs << "duration=" << clip.duration << "\n";
	ofs << "ticksPerSecond=" << clip.ticksPerSecond << "\n";
	ofs << "tracks=" << clip.tracks.size() << "\n";
	ofs << "totalKeys=" << totalKeys << "\n";
	ofs << "skippedEmptyTracks=" << skippedEmptyTracks << "\n";

	for (size_t i = 0; i < clip.tracks.size(); ++i)
	{
		const auto& track = clip.tracks[i];
		ofs << "track[" << i << "] boneIndex=" << track.boneIndex
			<< " keys=" << track.keyFrames.size() << "\n";
	}

	if (!missingBones.empty())
	{
		ofs << "missingBones=" << missingBones.size() << "\n";
		for (const auto& name : missingBones)
			ofs << "  " << name << "\n";
	}
}
#endif


bool ImportFBXToAnimJson(const aiScene* scene, const std::string& outDir, const std::string& baseName, const std::unordered_map<std::string, uint32_t>& boneNameToIndex, std::vector<std::string>& outAnimFiles)
{
	outAnimFiles.clear();
	if (!scene || scene->mNumAnimations == 0) return true;


	for (uint32_t a = 0; a < scene->mNumAnimations; ++a)
	{
		const aiAnimation* anim = scene->mAnimations[a];
		if (!anim) continue;

		double tps = anim->mTicksPerSecond;
		if (tps <= 0.0) tps = 30.0;

		AnimationClip clip{};
		std::string name = anim->mName.C_Str();
		if (name.empty()) name = "Anim" + std::to_string(a);
		clip.name = name;
		clip.ticksPerSecond = static_cast<float>(tps);
		clip.duration = static_cast<float>(anim->mDuration / tps); //seconds

		clip.tracks.reserve(anim->mNumChannels);

		uint32_t skippedEmptyTracks = 0;
		std::unordered_set<std::string> missingBoneNames;

		for (uint32_t c = 0; c < anim->mNumChannels; ++c)
		{
			const aiNodeAnim* nodeAnim = anim->mChannels[c];
			if (!nodeAnim) continue;

			const std::string nodeName = nodeAnim->mNodeName.C_Str();
			auto it = boneNameToIndex.find(nodeName);
			if (it == boneNameToIndex.end())
			{
				missingBoneNames.insert(nodeName);
				continue;	// 스켈레톤에 없으면 스킵
			}

			AnimationTrack track{};
			track.boneIndex = static_cast<int32_t>(it->second);

			std::vector<double> timesTick;
			CollectUnionTimesTicks(nodeAnim, timesTick);
			if (timesTick.empty())
			{
				++skippedEmptyTracks;
				continue;
			}


			track.keyFrames.reserve(timesTick.size());

			for (double timeTick : timesTick)
			{
				AnimationKeyFrame keyFrame{};
				keyFrame.time = static_cast<float>(timeTick / tps);	// seconds
				XMFLOAT3 p = SamplePos(nodeAnim, timeTick);
				XMFLOAT4 r = SampleRot(nodeAnim, timeTick);
				XMFLOAT3 s = SampleScale(nodeAnim, timeTick);

				keyFrame.translation = p;
				keyFrame.rotation = r;
				keyFrame.scale = s;

				track.keyFrames.push_back(keyFrame);
			}

			// 안정성: 시간 정렬 보장(이미 정렬된 times 사용)
			clip.tracks.push_back(std::move(track));
		}

		const std::string safeName = SanitizeName(clip.name);
		const std::string fileName = baseName + "_" + safeName + "_" + std::to_string(a) + ".anim.json";
		const std::string outPath = outDir + "/" + fileName;

		json j = ClipToJson(clip);

		std::ofstream ofs(outPath);
		if (!ofs) return false;
		ofs << j.dump(2);

#ifdef _DEBUG
		//std::vector<std::string> missingBones(missingBoneNames.begin(), missingBoneNames.end());
		//std::sort(missingBones.begin(), missingBones.end());
		//WriteAnimationDebug(outPath, clip, missingBones, skippedEmptyTracks);
		//std::vector<std::string> missingBones(missingBoneNames.begin(), missingBoneNames.end());
		//std::sort(missingBones.begin(), missingBones.end());
		//WriteAnimationDebug(outPath, clip, missingBones, skippedEmptyTracks);
#endif

		outAnimFiles.push_back(fileName);
	}

	return true;
}