#pragma once

#include "json.hpp"
#include <unordered_set>

#pragma pack(push, 1)
struct SkelBinHeader
{
	uint32_t magic                  = 0x534B454C; // "SKEL"
	uint16_t version				= 4;
	uint16_t boneCount              = 0;
	uint32_t stringTableBytes       = 0;
							        
	uint32_t upperCount		        = 0;
	uint32_t lowerCount		        = 0;

	int32_t  equipmentBoneIndex		= -1;
	float    equipmentBindPose[16]	= {};
};

struct BoneBin
{
	uint32_t nameOffset  = 0;
	int32_t  parentIndex = -1;
	float	 inverseBindPose[16]; // Row-Major
	float	 localBind[16];	      // aiNode local transform
};

#pragma pack(pop)

struct SkeletonBuildResult
{
	std::vector<BoneBin> bones;
	std::string stringTable;
	std::unordered_map<std::string, uint32_t> boneNameToIndex;
};

struct aiScene;
SkeletonBuildResult BuildSkeletonFromScene(const aiScene* scene, const std::unordered_set<std::string>& extraBoneNames); 
bool ImportFBXToSkelBin(
	const aiScene* scene,
	const std::string& outSkelBin,
	std::unordered_map<std::string, uint32_t>& outBoneNameToIndex,
	nlohmann::json& skeletonMeta);