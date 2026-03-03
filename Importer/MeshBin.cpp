#include "pch.h"
#include "MeshBin.h"
#include "BinHelper.h"
#include "iostream"

#include <filesystem>
#include <fstream>

#include "json.hpp"
using nlohmann::json;
namespace fs = std::filesystem;

struct Influence
{
	uint32_t bone = 0;
	float weight = 0.0f;
};

static std::string JoinPath(const std::string& dir, const std::string& file)
{
	if (dir.empty()) return file;
	const char last = dir.back();
	if (last == '/' || last == '\\') return dir + file;
	return dir + "/" + file;
}

static void AABBInit(AABBf& bounds)
{
	bounds.min[0] = bounds.min[1] = bounds.min[2] = +FLT_MAX;
	bounds.max[0] = bounds.max[1] = bounds.max[2] = -FLT_MAX;
}

static void AABBExpand(AABBf& bounds, float x, float y, float z)
{
	bounds.min[0] = std::min(bounds.min[0], x); bounds.max[0] = std::max(bounds.max[0], x);
	bounds.min[1] = std::min(bounds.min[1], y); bounds.max[1] = std::max(bounds.max[1], y);
	bounds.min[2] = std::min(bounds.min[2], z); bounds.max[2] = std::max(bounds.max[2], z);
}

static void NormalizeTop4(std::vector<Influence>& v)	//weight 상위 4개로 Normalize
{
	std::sort(v.begin(), v.end(), [](const Influence& a, const Influence& b) { return a.weight > b.weight; });
	if (v.size() > 4) v.resize(4);

	float sum = 0.0f;
	for (auto& x : v) sum += x.weight;
	if (sum <= 0.0f) { v.clear(); return; }

	for (auto& x : v) x.weight /= sum;
}

static uint16_t ToU16Norm(float weight)
{
	if (weight < 0.0f) weight = 0.0f;
	if (weight > 1.0f) weight = 1.0f;
	return static_cast<uint16_t>(weight * 65535.0f + 0.5f);
}

static void CollectMeshTransforms_DFS(
	const aiNode* node,
	const aiMatrix4x4& parentGlobal,
	std::vector<std::vector<aiMatrix4x4>>& out // [meshIndex] -> transforms
)
{
	if (!node) return;

	aiMatrix4x4 global = parentGlobal * node->mTransformation;

	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
	{
		uint32_t meshIndex = node->mMeshes[i];
		if (meshIndex < out.size())
			out[meshIndex].push_back(global);
	}

	for (uint32_t c = 0; c < node->mNumChildren; ++c)
		CollectMeshTransforms_DFS(node->mChildren[c], global, out);
}

// static void CollectMeshInstances_DFS(
// 	const aiNode* node,
// 	const aiMatrix4x4& parentGlobal,
// 	std::vector<MeshInstance>& outInstances
// )
// {
// 	if (!node) return;
// 
// 	aiMatrix4x4 global = parentGlobal * node->mTransformation;
// 	const std::string nodeName = node->mName.C_Str();
// 
// 	// 이 노드가 참조하는 mesh들
// 	for (uint32_t i = 0; i < node->mNumMeshes; ++i)
// 	{
// 		MeshInstance meshInst;
// 		meshInst.meshIndex = node->mMeshes[i];
// 		meshInst.global = global;
// 		meshInst.nodeName = nodeName;
// 		outInstances.push_back(std::move(meshInst));
// 	}
// 
// 	// 자식으로 내려감
// 	for (uint32_t c = 0; c < node->mNumChildren; ++c)
// 	{
// 		CollectMeshInstances_DFS(node->mChildren[c], global, outInstances);
// 	}
// }

static void SaveVertex(const aiMesh* mesh, uint32_t i, Vertex& v)
{
	v.px = mesh->mVertices[i].x;
	v.py = mesh->mVertices[i].y;
	v.pz = mesh->mVertices[i].z;


	if (mesh->HasNormals())
	{
		v.nx = mesh->mNormals[i].x;
		v.ny = mesh->mNormals[i].y;
		v.nz = mesh->mNormals[i].z;
	}

	if (mesh->HasTextureCoords(0))
	{
		v.u = mesh->mTextureCoords[0][i].x;
		v.v = mesh->mTextureCoords[0][i].y;
	}

	// tangent + handedness
	if (mesh->HasTangentsAndBitangents() && mesh->HasNormals())
	{
		const aiVector3D& T = mesh->mTangents[i];
		const aiVector3D& B = mesh->mBitangents[i];
		const aiVector3D& N = mesh->mNormals[i];

		v.tx = T.x; v.ty = T.y; v.tz = T.z;

		// handedness sign: +1 or -1
		const aiVector3D c = Cross3(N, T);
		const float det = Dot3(c, B);
		v.handedness = (det < 0.0f) ? -1.0f : 1.0f;
	}
	else
	{
		// tangent space가 없으면 기본값
		v.tx = 0.0f; v.ty = 0.0f; v.tz = 0.0f;
		v.handedness = 1.0f;
	}
}



#ifdef _DEBUG
static void WriteMeshBinDebugJson(
	const fs::path& outPath,
	const MeshBinHeader& header,
	const std::vector<SubMeshBin>& subMeshes,
	const std::vector<Vertex>& vertices,
	const std::vector<VertexSkinned>& verticesSkinned,
	const std::vector<uint32_t>& indices,
	bool isSkinned)
{
	json root;
	root["path"] = outPath.generic_string();
	root["header"] = {
		{"magic", header.magic},
		{"version", header.version},
		{"flags", header.flags},
		{"vertexCount", header.vertexCount},
		{"indexCount", header.indexCount},
		{"subMeshCount", header.subMeshCount},
		{"stringTableBytes", header.stringTableBytes},
		{"bounds", {
			{"min", { header.bounds.min[0], header.bounds.min[1], header.bounds.min[2] }},
			{"max", { header.bounds.max[0], header.bounds.max[1], header.bounds.max[2] }}
		}}
	};

	root["subMeshes"] = json::array();
	for (const auto& subMesh : subMeshes)
	{
		root["subMeshes"].push_back({
		{"indexStart", subMesh.indexStart},
		{"indexCount", subMesh.indexCount},
		{"materialNameOffset", subMesh.materialNameOffset},
		{"nameOffset", subMesh.nameOffset},
		{"bounds", {
			{"min", { subMesh.bounds.min[0], subMesh.bounds.min[1], subMesh.bounds.min[2] }},
			{"max", { subMesh.bounds.max[0], subMesh.bounds.max[1], subMesh.bounds.max[2] }}
		}}
			});
	}

	root["isSkinned"] = isSkinned;
	root["vertices"] = json::array();
	if (isSkinned)
	{
		for (const auto& v : verticesSkinned)
		{
			root["vertices"].push_back({
				{"pos", { v.px, v.py, v.pz }},
				{"normal", { v.nx, v.ny, v.nz }},
				{"uv", { v.u, v.v }},
				{"tangent", { v.tx, v.ty, v.tz, v.handedness }},
				{"boneIndex", { v.boneIndex[0], v.boneIndex[1], v.boneIndex[2], v.boneIndex[3] }},
				{"boneWeight", { v.boneWeight[0], v.boneWeight[1], v.boneWeight[2], v.boneWeight[3] }}
			});
		}
	}
	else
	{
		for (const auto& v : vertices)
		{
			root["vertices"].push_back({
				{"pos", { v.px, v.py, v.pz }},
				{"normal", { v.nx, v.ny, v.nz }},
				{"uv", { v.u, v.v }},
				{"tangent", { v.tx, v.ty, v.tz, v.handedness }}
			});
		}
	}

	root["indices"] = indices;

	fs::path debugPath = outPath;
	debugPath += ".write.debug.json";
	std::ofstream ofs(debugPath);
	if (ofs)
	{
		ofs << root.dump(2);
	}
}
#endif

static uint32_t GetStringOffset(
	std::string& stringTable,
	std::unordered_map<std::string, uint32_t>& offsets,
	const std::string& value
)
{
	auto it = offsets.find(value);
	if (it != offsets.end())
	{
		return it->second;
	}

	const uint32_t offset = static_cast<uint32_t>(stringTable.size());
	stringTable.append(value);
	stringTable.push_back('\0');
	offsets.emplace(value, offset);

	return offset;
}

static void AppendMeshVertices(
	const aiMesh* mesh,
	const std::unordered_map<std::string, uint32_t>* boneNameToIndex,
	bool useSkinned,
	std::vector<Vertex>& vertices,
	std::vector<VertexSkinned>& verticesSkinned,
	AABBf& outBounds
)
{
	const bool isSkinned = mesh->HasBones() && mesh->mNumBones > 0;
	if (useSkinned)
	{
		std::vector<std::vector<Influence>> infl;
		if (isSkinned)
		{
			infl.resize(mesh->mNumVertices);

			for (uint32_t b = 0; b < mesh->mNumBones; ++b)
			{
				const aiBone* bone = mesh->mBones[b];
				if (!bone) continue;

				const std::string boneName = bone->mName.C_Str();

				auto it = boneNameToIndex->find(boneName);
				if (it == boneNameToIndex->end())
				{
					continue;
				}

				const uint32_t boneIndex = it->second;
				for (uint32_t w = 0; w < bone->mNumWeights; ++w)
				{
					const aiVertexWeight& vw = bone->mWeights[w];

					if (vw.mVertexId >= mesh->mNumVertices) continue;

					infl[vw.mVertexId].push_back({ boneIndex, vw.mWeight });
				}
			}
			for (auto& v : infl) NormalizeTop4(v);
		}

		verticesSkinned.reserve(verticesSkinned.size() + mesh->mNumVertices);
		for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
		{
			VertexSkinned v{};
			SaveVertex(mesh, i, static_cast<Vertex&>(v));
			AABBExpand(outBounds, v.px, v.py, v.pz);
			
			if (isSkinned && i < infl.size())
			{
				const auto& inf = infl[i];
				for (size_t k = 0; k < inf.size(); ++k)
				{
					v.boneIndex [k] = static_cast<uint16_t>(inf[k].bone);
					v.boneWeight[k] = ToU16Norm(inf[k].weight);
				}
			}

			verticesSkinned.push_back(v);
		}
	}
	else
	{
		vertices.reserve(vertices.size() + mesh->mNumVertices);

		for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
		{
			Vertex v{};
			SaveVertex(mesh, i, v);
			AABBExpand(outBounds, v.px, v.py, v.pz);
			vertices.push_back(v);
		}
	}
}

static void AABBExpandByTransformedLocalAABB(AABBf& worldBounds, const AABBf& localBounds, const aiMatrix4x4& localToWorld)
{
	// 8 corners 변환해서 world AABB 확장
	const aiVector3D c[8] =
	{
		{ localBounds.min[0], localBounds.min[1], localBounds.min[2] },
		{ localBounds.max[0], localBounds.min[1], localBounds.min[2] },
		{ localBounds.min[0], localBounds.max[1], localBounds.min[2] },
		{ localBounds.max[0], localBounds.max[1], localBounds.min[2] },
		{ localBounds.min[0], localBounds.min[1], localBounds.max[2] },
		{ localBounds.max[0], localBounds.min[1], localBounds.max[2] },
		{ localBounds.min[0], localBounds.max[1], localBounds.max[2] },
		{ localBounds.max[0], localBounds.max[1], localBounds.max[2] },
	};

	for (int i = 0; i < 8; ++i)
	{
		aiVector3D w = TransformPoint(localToWorld, c[i]);
		AABBExpand(worldBounds, w.x, w.y, w.z);
	}
}

bool ImportFBXToMeshBin(
	const aiScene* scene,
	const std::string& outDir, 
	const std::string& baseName,
	const std::unordered_map<std::string, uint32_t>& boneNameToIndex, //skinned
	std::vector<std::string>& outMeshFiles
)
{
	if (!scene || !scene->HasMeshes()) return false;


	outMeshFiles.clear();
	outMeshFiles.reserve(scene->mNumMeshes);

	bool anySkinned = false;
	for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];
		if (!mesh) continue;

		if (mesh->HasBones() && mesh->mNumBones > 0)
		{
			anySkinned = true;
			break;
		}
	}

	if (anySkinned && boneNameToIndex.empty())
	{
		return false;
	}
	
	std::vector<std::vector<aiMatrix4x4>> meshTransforms(scene->mNumMeshes);
	aiMatrix4x4 I; // identity
	CollectMeshTransforms_DFS(scene->mRootNode, I, meshTransforms);

	// 비어있으면 identity 1개라도 넣어둠(렌더 불가 방지)
	for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
	{
		if (meshTransforms[m].empty())
			meshTransforms[m].push_back(I);
	}

// 	1) 노드 기반으로 mesh 인스턴스 목록 수집
// 		std::vector<MeshInstance> instances;
// 		instances.reserve(scene->mNumMeshes);
// 	
// 		CollectMeshInstances_DFS(scene->mRootNode, I, instances);
// 	
// 		if (instances.empty())
// 		{
// 			// 노드에 mesh 참조가 없는 비정상 케이스 대비: 그냥 mesh들 1회씩 처리
// 			for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
// 			{
// 				MeshInstance inst;
// 				inst.meshIndex = m;
// 				inst.global = I;
// 				inst.nodeName = "Root";
// 				instances.push_back(std::move(inst));
// 			}
// 		}

	std::vector<Vertex>        vertices;
	std::vector<VertexSkinned> verticesSkinned;
	std::vector<uint32_t>	   indices;
	std::vector<SubMeshBin>	   subMeshes;
	subMeshes.reserve(scene->mNumMeshes);

	// 헤더용 전체 월드 bounds
	AABBf sceneWorldBounds{};
	AABBInit(sceneWorldBounds);

	std::string stringTable;
	stringTable.push_back('\0');
	std::unordered_map<std::string, uint32_t> stringOffsets;

	uint32_t vertexBase = 0;
	
	std::vector<InstanceTransformBin> instanceTransforms;
	size_t approxInst = 0;
	for (auto& v : meshTransforms) approxInst += v.size();
	instanceTransforms.reserve(approxInst);

	for (uint32_t m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];
		if (!mesh) continue;

		SubMeshBin subMesh{};
		subMesh.indexStart = static_cast<uint32_t>(indices.size());

		AABBf localBounds{};
		AABBInit(localBounds); 
		AABBf transformedBounds{};
		AABBInit(transformedBounds);

		const std::unordered_map<std::string, uint32_t>* mapPtr = anySkinned ? &boneNameToIndex : nullptr;
		AppendMeshVertices(mesh, mapPtr, anySkinned, vertices, verticesSkinned, localBounds);

		// 인덱스 복제
		for (uint32_t f = 0; f < mesh->mNumFaces; ++f)
		{
			const aiFace& face = mesh->mFaces[f];
			if (face.mNumIndices != 3) continue;

			indices.push_back(vertexBase + static_cast<uint32_t>(face.mIndices[0]));
			indices.push_back(vertexBase + static_cast<uint32_t>(face.mIndices[1]));
			indices.push_back(vertexBase + static_cast<uint32_t>(face.mIndices[2]));
		}


		subMesh.indexCount = static_cast<uint32_t>(indices.size()) - subMesh.indexStart;

		if (anySkinned)
		{
			for (const aiMatrix4x4& g : meshTransforms[m])
			{
				AABBExpandByTransformedLocalAABB(transformedBounds, localBounds, g);
			}
			subMesh.bounds = transformedBounds;
		}
		else
		{
			// subMesh.bounds <- 로컬 bounds 저장
			subMesh.bounds = localBounds;
		}

		// ===== 인스턴스 리스트 저장 =====
		subMesh.instanceStart = static_cast<uint32_t>(instanceTransforms.size());
		if (anySkinned)
		{
			subMesh.instanceCount = 1;

			InstanceTransformBin it{};
			XMFLOAT4X4 id{};
			id._11 = id._22 = id._33 = id._44 = 1.0f;
			it.localToWorld = id;
			instanceTransforms.push_back(it);

			for (const aiMatrix4x4& g : meshTransforms[m])
			{
				AABBExpandByTransformedLocalAABB(sceneWorldBounds, localBounds, g);
			}
		}
		else
		{
			subMesh.instanceCount = static_cast<uint32_t>(meshTransforms[m].size());

			for (const aiMatrix4x4& g : meshTransforms[m])
			{
				InstanceTransformBin it{};
				it.localToWorld = ToXMFLOAT4X4(g);
				instanceTransforms.push_back(it);

				// 헤더 bounds는 모든 인스턴스를 반영
				AABBExpandByTransformedLocalAABB(sceneWorldBounds, localBounds, g);
			}
		}

		const uint32_t materialIndex = mesh->mMaterialIndex;
		std::string materialName;
		if (materialIndex < scene->mNumMaterials)
		{
			const aiMaterial* material = scene->mMaterials[materialIndex];
			if (material)
			{
				aiString name;
				if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
				{
					materialName = name.C_Str();
				}
			}
		}

		if (materialName.empty())
		{
			materialName = "Material_" + std::to_string(materialIndex);
		}

		std::string meshName = mesh->mName.C_Str();
		if (meshName.empty())
		{
			meshName = "SubMesh_" + std::to_string(m);
		}

		subMesh.materialNameOffset = GetStringOffset(stringTable, stringOffsets, materialName);
		subMesh.nameOffset = GetStringOffset(stringTable, stringOffsets, meshName);
		subMeshes.push_back(subMesh);

		vertexBase += mesh->mNumVertices;
	}

	std::string file = baseName + ".meshbin";
	std::string outPath = JoinPath(outDir, file);

	MeshBinHeader header{};
	header.version = 3;
	header.flags = anySkinned ? MESH_HAS_SKINNING : 0;
	header.vertexCount      = anySkinned ? static_cast<uint32_t>(verticesSkinned.size()) : static_cast<uint32_t>(vertices.size());
	header.indexCount       = static_cast<uint32_t>(indices.size());
	header.subMeshCount     = static_cast<uint32_t>(subMeshes.size());
	header.instanceCount	= static_cast<uint32_t>(instanceTransforms.size());
	header.stringTableBytes = static_cast<uint32_t>(stringTable.size());
	header.bounds = sceneWorldBounds;

#ifdef _DEBUG
	//WriteMeshBinDebugJson(fs::path(outPath), header, subMeshes, vertices, verticesSkinned, indices, anySkinned);
#endif

	//저장
	std::ofstream ofs(outPath, std::ios::binary);
	if (!ofs) return false;

	ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

	if (!subMeshes.empty())
	{
		ofs.write(reinterpret_cast<const char*>(subMeshes.data()), sizeof(SubMeshBin) * subMeshes.size());
	}
	if (!instanceTransforms.empty())
		ofs.write(reinterpret_cast<const char*>(instanceTransforms.data()), sizeof(InstanceTransformBin) * instanceTransforms.size());

	if (anySkinned)
	{
		ofs.write(reinterpret_cast<const char*>(verticesSkinned.data()), sizeof(VertexSkinned) * verticesSkinned.size());
	}
	else
	{
		ofs.write(reinterpret_cast<const char*>(vertices.data()), sizeof(Vertex) * vertices.size());
	}
	if (!indices.empty())
	{
		ofs.write(reinterpret_cast<const char*>(indices.data()), sizeof(uint32_t) * indices.size());
	}
	if (!stringTable.empty())
	{
		ofs.write(stringTable.data(), stringTable.size());
	}

	outMeshFiles.push_back(file);

	return true;
}