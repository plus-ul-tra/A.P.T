#pragma once
#include "ImporterMathHelper.h"

enum MeshFlags : uint16_t
{
    MESH_HAS_SKINNING = 1 << 0,
};

#pragma pack(push, 1)
struct AABBf
{
    float min[3];
    float max[3];
};

struct MeshBinHeader
{
    uint32_t magic = 0x4D455348; // "MESH"
    uint16_t version = 3;
    uint16_t flags = 0;          // bit0: hasSkinning
    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t subMeshCount = 0;
    uint32_t instanceCount = 0; 
    uint32_t stringTableBytes = 0;
    AABBf    bounds{};
};

struct SubMeshBin
{
    uint32_t            indexStart;
    uint32_t            indexCount;
    uint32_t            materialNameOffset;
    uint32_t            nameOffset;
    AABBf               bounds{};
	uint32_t            instanceStart = 0;      // InstanceTransform 배열 시작 인덱스
	uint32_t            instanceCount = 0;      // 이 submesh의 인스턴스 개수
};

struct InstanceTransformBin
{
	XMFLOAT4X4 localToWorld;
};

struct MeshInstance
{
	uint32_t    meshIndex = 0;   // scene->mMeshes[meshIndex]
	aiMatrix4x4 global;          // parent 누적된 global transform
	std::string nodeName;        // submesh 이름 만들 때 사용(선택)
};

struct Vertex
{
    float px, py, pz;
    float nx, ny, nz;
    float  u,  v;
    float tx, ty, tz, handedness;
};

struct VertexSkinned : Vertex
{
    uint16_t boneIndex[4] = { 0,0,0,0 };
    uint16_t boneWeight[4] = { 0,0,0,0 }; // normalized to 0..65535
};
#pragma pack(pop)

struct aiScene;

bool ImportFBXToMeshBin(
    const aiScene* scene, 
    const std::string& outDir,
    const std::string& baseName,
    const std::unordered_map<std::string, uint32_t>& boneNameToIndex, //skinned
    std::vector<std::string>& outMeshFiles
);