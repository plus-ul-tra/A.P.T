#include "AssetLoader.h"

#include <array>
#include <filesystem>
#include <fstream>
#include <cstdint>
#include <unordered_map>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace
{
	enum class ETextureType : uint8_t { ALBEDO, NORMAL, METALLIC, ROUGHNESS, AO, EMISSIVE, MAX };
	static constexpr size_t TEX_MAX = static_cast<size_t>(ETextureType::MAX);

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
		uint16_t version = 0;
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
		uint32_t indexStart;
		uint32_t indexCount;
		uint32_t materialNameOffset;
		uint32_t nameOffset;
		AABBf    bounds{};
		uint32_t instanceStart = 0;      // InstanceTransform 배열 시작 인덱스
		uint32_t instanceCount = 0;      // 이 submesh의 인스턴스 개수
	};

	struct InstanceTransformBin
	{
		float m[16]; // XMFLOAT4X4 그대로 memcpy 가능(로우메이저 전제)
	};

	struct Vertex
	{
		float px, py, pz;
		float nx, ny, nz;
		float u, v;
		float tx, ty, tz, handedness;
	};

	struct VertexSkinned : Vertex
	{
		uint16_t boneIndex [4] = { 0, 0, 0, 0 };
		uint16_t boneWeight[4] = { 0, 0, 0, 0 };
	};

	struct MatBinHeader
	{
		uint32_t magic = 0x4D41544C; // "MATL"
		uint16_t version = 1;
		uint16_t materialCount = 0;
		uint32_t stringTableBytes = 0;
	};

	struct MatData
	{
		uint32_t materialNameOffset = 0;   // string table offset
		float baseColor[4];
		float metallic = 0.0f;
		float roughness = 1.0f;
		uint8_t alphaMode = 0;             // 0 : Opaque 1 : Mask 2 : Blend
		float alphaCutOff = 0.5f;
		float opacity = 1.0f;
		uint8_t doubleSided = 0;
		uint8_t pad[3] = { 0, 0, 0 };
		uint32_t texPathOffset[TEX_MAX] = {}; // 0이면 없음
	};

	struct SkelBinHeader
	{
		uint32_t magic            = 0x534B454C; // "SKEL"
		uint16_t version          = 3;
		uint16_t boneCount        = 0;
		uint32_t stringTableBytes = 0;

		uint32_t upperCount		  = 0;
		uint32_t lowerCount		  = 0;
	};

	struct SkelBinEquipmentData
	{
		int32_t equipmentBoneIndex = -1;
		float   equipmentBindPose[16] = {};
	};

	struct BoneBin
	{
		uint32_t nameOffset = 0;
		int32_t  parentIndex = -1;
		float	 inverseBindPose[16]; // Row-Major
		float	 localBind[16];
	};
#pragma pack(pop)

	constexpr uint32_t kMeshMagic = 0x4D455348; // "MESH"
	constexpr uint32_t kMatMagic  = 0x4D41544C; // "MATL"
	constexpr uint32_t kSkelMagic = 0x534B454C; // "SKEL"

	void ExpandBounds(DirectX::XMFLOAT3& minOut, DirectX::XMFLOAT3& maxOut, const DirectX::XMFLOAT3& point)
	{
		minOut.x = min(minOut.x, point.x);
		minOut.y = min(minOut.y, point.y);
		minOut.z = min(minOut.z, point.z);

		maxOut.x = max(maxOut.x, point.x);
		maxOut.y = max(maxOut.y, point.y);
		maxOut.z = max(maxOut.z, point.z);
	}

	void RebuildMeshBoundsFromSubMeshes(RenderData::MeshData& meshData)
	{
		if (meshData.subMeshes.empty())
		{
			return;
		}

		DirectX::XMFLOAT3 minOut{ FLT_MAX, FLT_MAX, FLT_MAX };
		DirectX::XMFLOAT3 maxOut{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

		for (const auto& subMesh : meshData.subMeshes)
		{
			const DirectX::XMFLOAT3 min = subMesh.boundsMin;
			const DirectX::XMFLOAT3 max = subMesh.boundsMax;

			const DirectX::XMFLOAT3 corners[8] = {
				{ min.x, min.y, min.z },
				{ max.x, min.y, min.z },
				{ max.x, max.y, min.z },
				{ min.x, max.y, min.z },
				{ min.x, min.y, max.z },
				{ max.x, min.y, max.z },
				{ max.x, max.y, max.z },
				{ min.x, max.y, max.z }
			};

			const auto localToWorld = DirectX::XMLoadFloat4x4(&subMesh.localToWorld);
			for (const auto& corner : corners)
			{
				const auto v = DirectX::XMLoadFloat3(&corner);
				const auto transformed = DirectX::XMVector3TransformCoord(v, localToWorld);
				DirectX::XMFLOAT3 worldCorner{};
				DirectX::XMStoreFloat3(&worldCorner, transformed);
				ExpandBounds(minOut, maxOut, worldCorner);
			}
		}

		meshData.boundsMin = minOut;
		meshData.boundsMax = maxOut;
	}


	std::string ReadStringAtOffset(const std::string& table, uint32_t offset)
	{
		if (offset >= table.size())
			return {};

		const char* ptr = table.data() + offset;
		return std::string(ptr);
	}

	fs::path ResolvePath(const fs::path& baseDir, const std::string& relative)
	{
		fs::path path(relative);
		if (path.is_absolute())
		{
			return path.lexically_normal();
		}

		return (baseDir / path).lexically_normal();
	}

	std::string ToLowerCopy(std::string value)
	{
		std::transform(value.begin(), value.end(), value.begin(),
			[](unsigned char c)
			{
				return static_cast<char>(std::tolower(c));
			});
		return value;
	}

	bool ContainsTokenIgnoreCase(const std::string& value, const std::string& token)
	{
		const std::string lowerValue = ToLowerCopy(value);
		const std::string lowerToken = ToLowerCopy(token);
		return lowerValue.find(lowerToken) != std::string::npos;
	}

	void LogMaterialBinTextures(
		const std::string& materialBinPath,
		const std::vector<MatData>& mats,
		const std::string& stringTable)
	{
		for (size_t i = 0; i < mats.size(); ++i)
		{
			const MatData& mat = mats[i];
			std::string materialName = ReadStringAtOffset(stringTable, mat.materialNameOffset);
			if (materialName.empty())
			{
				materialName = "Material_" + std::to_string(i);
			}

			for (size_t t = 0; t < TEX_MAX; ++t)
			{
				const std::string texPathRaw = ReadStringAtOffset(stringTable, mat.texPathOffset[t]);
				if (texPathRaw.empty())
				{
					continue;
				}
			}
		}
	}

	bool PushUnique(std::vector<TextureHandle>& out, TextureHandle handle)
	{
		for (const auto& existing : out)
		{
			if (existing == handle)
			{
				return false;
			}
		}

		out.push_back(handle);
		return true;
	}

	RenderData::MaterialTextureSlot ToMaterialSlot(ETextureType type, bool& outValid)
	{
		outValid = true;
		switch (type)
		{
		case ETextureType::ALBEDO:
			return RenderData::MaterialTextureSlot::Albedo;
		case ETextureType::NORMAL:
			return RenderData::MaterialTextureSlot::Normal;
		case ETextureType::METALLIC:
			return RenderData::MaterialTextureSlot::Metallic;
		case ETextureType::ROUGHNESS:
			return RenderData::MaterialTextureSlot::Roughness;
		case ETextureType::AO:
			return RenderData::MaterialTextureSlot::AO;
		case ETextureType::EMISSIVE:
			return RenderData::MaterialTextureSlot::Emissive;
		default:
			outValid = false;
			return RenderData::MaterialTextureSlot::Albedo;
		}
	}

	template <typename HandleType>
	uint64_t MakeHandleKey(const HandleType& handle)
	{
		return (static_cast<uint64_t>(handle.generation) << 32) | handle.id;
	}

	template <typename HandleType>
	void StoreReferenceIfMissing(
		std::unordered_map<uint64_t, AssetRef>& out,
		const HandleType& handle,
		const std::string& assetMetaPath,
		UINT32 index
	)
	{
		const uint64_t key = MakeHandleKey(handle);
		if (out.find(key) != out.end())
		{
			return;
		}
		
		out.emplace(key, AssetRef{ assetMetaPath, index });
	}

	RenderData::Vertex ToRenderVertex(const Vertex& in)
	{
		RenderData::Vertex out{};
		out.position = { in.px, in.py, in.pz };
		out.normal   = { in.nx, in.ny, in.nz };
		out.uv       = { in.u, in.v };
		out.tangent  = { in.tx, in.ty, in.tz, in.handedness };
		return out;
	}

	RenderData::Vertex ToRenderVertex(const VertexSkinned& in)
	{
		RenderData::Vertex out = ToRenderVertex(static_cast<const Vertex&>(in));
		out.boneIndices = { in.boneIndex[0], in.boneIndex[1], in.boneIndex[2], in.boneIndex[3] };
		const float invWeight = 1.0f / 65535.0f;
		out.boneWeights = {
			static_cast<float>(in.boneWeight[0]) * invWeight,
			static_cast<float>(in.boneWeight[1]) * invWeight,
			static_cast<float>(in.boneWeight[2]) * invWeight,
			static_cast<float>(in.boneWeight[3]) * invWeight
		};

		const float weightSum = out.boneWeights[0] + out.boneWeights[1] + out.boneWeights[2] + out.boneWeights[3];
		if (weightSum > 0.0f)
		{
			const float invSum = 1.0f / weightSum;
			out.boneWeights[0] *= invSum;
			out.boneWeights[1] *= invSum;
			out.boneWeights[2] *= invSum;
			out.boneWeights[3] *= invSum;
		}
		else
		{
			out.boneIndices = { 0, 0, 0, 0 };
			out.boneWeights = { 1.0f, 0.0f, 0.0f, 0.0f };
		}

		return out;
	}

#ifdef _DEBUG
	struct SkinningStats
	{
		size_t zeroWeightVertexCount = 0;
		uint16_t maxBoneIndex = 0;
		std::vector<double> boneWeightSums;
		std::vector<size_t> boneWeightVertexCounts;
	};

	SkinningStats GatherSkinningStats(const RenderData::MeshData& meshData)
	{
		SkinningStats stats{};

		for (const auto& v : meshData.vertices)
		{
			const float weightSum = v.boneWeights[0] + v.boneWeights[1] + v.boneWeights[2] + v.boneWeights[3];
			if (weightSum <= 0.0f)
			{
				++stats.zeroWeightVertexCount;
			}

			for (size_t i = 0; i < v.boneIndices.size(); ++i)
			{
				const float weight = v.boneWeights[i];
				if (weight <= 0.0f)
					continue;

				const uint16_t boneIndex = v.boneIndices[i];
				if (boneIndex > stats.maxBoneIndex)
				{
					stats.maxBoneIndex = boneIndex;
				}

				if (stats.boneWeightSums.size() <= boneIndex)
				{
					stats.boneWeightSums.resize(boneIndex + 1, 0.0);
					stats.boneWeightVertexCounts.resize(boneIndex + 1, 0);
				}

				stats.boneWeightSums[boneIndex] += weight;
				++stats.boneWeightVertexCounts[boneIndex];
			}
		}

		return stats;
	}
#endif

#ifdef _DEBUG
	void WriteMeshBinLoadDebugJson(
		const fs::path& meshPath,
		const MeshBinHeader& header,
		const std::vector<SubMeshBin>& subMeshes,
		const RenderData::MeshData& meshData)
	{
		json root;
		root["path"] = meshPath.generic_string();
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

		root["isSkinned"] = meshData.hasSkinning ? true : false;
		if (meshData.hasSkinning)
		{
			const auto stats = GatherSkinningStats(meshData);
			root["skinningStats"] = {
				{"zeroWeightVertexCount", stats.zeroWeightVertexCount},
				{"maxBoneIndex", stats.maxBoneIndex},
				{"boneWeightSums", stats.boneWeightSums},
				{"boneWeightVertexCounts", stats.boneWeightVertexCounts}
			};
		}

		root["vertices"] = json::array();
		for (const auto& v : meshData.vertices)
		{
			root["vertices"].push_back({
				{"pos", { v.position.x, v.position.y, v.position.z }},
				{"normal", { v.normal.x, v.normal.y, v.normal.z }},
				{"uv", { v.uv.x, v.uv.y }},
				{"tangent", { v.tangent.x, v.tangent.y, v.tangent.z, v.tangent.w }},
				{"boneIndex", { v.boneIndices[0], v.boneIndices[1], v.boneIndices[2], v.boneIndices[3] }},
				{"boneWeight", { v.boneWeights[0], v.boneWeights[1], v.boneWeights[2], v.boneWeights[3] }}
				});
		}

		root["indices"] = meshData.indices;

		fs::path debugPath = meshPath;
		debugPath += ".load.debug.json";
		std::ofstream ofs(debugPath);
		if (ofs)
		{
			ofs << root.dump(2);
		}
	}
#endif

	RenderData::AnimationClip ParseAnimationJson(const json& j)
	{
		RenderData::AnimationClip clip{};
		clip.name = j.value("name", "");
		clip.duration = j.value("duration", 0.0f);
		clip.ticksPerSecond = j.value("ticksPerSecond", 0.0f);

		if (!j.contains("tracks") || !j["tracks"].is_array())
		{
			return clip;
		}

		for (const auto& trackJson : j["tracks"])
		{
			RenderData::AnimationTrack track{};
			track.boneIndex = trackJson.value("boneIndex", -1);

			if (trackJson.contains("keys") && trackJson["keys"].is_array())
			{
				for (const auto& keyJson : trackJson["keys"])
				{
					RenderData::AnimationKeyFrame key{};
					key.time = keyJson.value("time", 0.0f);

					const auto& pos = keyJson.value("position", std::vector<float>{});
					if (pos.size() >= 3)
					{
						key.translation = { pos[0], pos[1], pos[2] };
					}

					const auto& rot = keyJson.value("rotation", std::vector<float>{});
					if (rot.size() >= 4)
					{
						key.rotation = { rot[0], rot[1], rot[2], rot[3] };
					}

					const auto& scale = keyJson.value("scale", std::vector<float>{});
					if (scale.size() >= 3)
					{
						key.scale = { scale[0], scale[1], scale[2] };
					}

					track.keyFrames.push_back(key);
				}
			}

			clip.tracks.push_back(std::move(track));
		}

		return clip;
	}
}

#ifdef _DEBUG
bool IsZeroMatrix(const float* m)
{
	for (int i = 0; i < 16; ++i)
	{
		if (m[i] != 0.0f)
			return false;
	}
	return true;
}

void WriteSkeletonLoadDebug(const fs::path& skelPath, const RenderData::Skeleton& skeleton)
{
	std::ofstream ofs(skelPath.string() + ".load.debug.txt");
	if (!ofs)
		return;

	ofs << "boneCount=" << skeleton.bones.size() << "\n";
	for (size_t i = 0; i < skeleton.bones.size(); ++i)
	{
		const auto& bone = skeleton.bones[i];
		ofs << "bone[" << i << "] name=" << bone.name
			<< " parentIndex=" << bone.parentIndex
			<< " bindPoseZero=" << (IsZeroMatrix(reinterpret_cast<const float*>(&bone.bindPose)) ? "true" : "false")
			<< " inverseBindPoseZero=" << (IsZeroMatrix(reinterpret_cast<const float*>(&bone.inverseBindPose)) ? "true" : "false")
			<< "\n";
		ofs << "  bindPose=";
		const float* bind = reinterpret_cast<const float*>(&bone.bindPose);
		for (int m = 0; m < 16; ++m)
			ofs << bind[m] << (m == 15 ? "\n" : ",");
		ofs << "  inverseBindPose=";
		const float* inv = reinterpret_cast<const float*>(&bone.inverseBindPose);
		for (int m = 0; m < 16; ++m)
			ofs << inv[m] << (m == 15 ? "\n" : ",");
	}
}

void WriteAnimationLoadDebug(const fs::path& animPath, const RenderData::AnimationClip& clip)
{
	std::ofstream ofs(animPath.string() + ".load.debug.txt");
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

	for (size_t i = 0; i < clip.tracks.size(); ++i)
	{
		const auto& track = clip.tracks[i];
		ofs << "track[" << i << "] boneIndex=" << track.boneIndex
			<< " keys=" << track.keyFrames.size() << "\n";
	}
}
#endif

AssetLoader::AssetLoader()
{
	SetActive(this);
}

AssetLoader::~AssetLoader()
{
	m_Meshes.Clear();
	m_Materials.Clear();
	m_Textures.Clear();
	m_Skeletons.Clear();
	m_Animations.Clear();
	m_AssetsByPath.clear();
	m_MeshRefs.clear();
	m_MaterialRefs.clear();
	m_TextureRefs.clear();
	m_SkeletonRefs.clear();
	m_AnimationRefs.clear();
	m_BGMPaths.clear();
	m_SFXPaths.clear();
}

void AssetLoader::SetActive(AssetLoader* loader)
{
	s_ActiveLoader = loader;
}

AssetLoader* AssetLoader::GetActive()
{
	return s_ActiveLoader;
}

void AssetLoader::LoadAll()
{
	m_BGMPaths.clear();
	m_SFXPaths.clear();

	const fs::path assetRoot = "../ResourceOutput";
	if (fs::exists(assetRoot) && fs::is_directory(assetRoot))
	{
		for (const auto& dirEntry : fs::directory_iterator(assetRoot))
		{
			if (!dirEntry.is_directory())
				continue;

			fs::path metaDir = dirEntry.path() / "Meta";
			if (!fs::exists(metaDir) || !fs::is_directory(metaDir))
				continue;

			for (const auto& fileEntry : fs::directory_iterator(metaDir))
			{
				if (!fileEntry.is_regular_file())
					continue;

				const fs::path& path = fileEntry.path();
				if (path.extension() != ".json")
					continue;

				const std::string filename = path.filename().string();
				if (filename.find(".shader.json") != std::string::npos)
				{
					LoadShaderAsset(path.string());
					continue;
				}

				if (filename.find(".asset.json") == std::string::npos)
					continue;

				LoadAsset(path.string());
			}
		}
	}

	const fs::path shaderRoot = "../MRenderer/fx";
	LoadShaderSources(shaderRoot);

	const fs::path uiTextureRoot = "../Resources/UI";
	LoadLooseTextures(uiTextureRoot, true, "UI");

	LoadSoundResources("../Resources/Sound/BGM", "../Resources/Sound/SFX");

}

void AssetLoader::LoadLooseTextures(const fs::path& rootDir, bool sRGB, const std::string& displayPrefix)
{
	if (!fs::exists(rootDir) || !fs::is_directory(rootDir))
	{
		return;
	}

	auto isTextureFile = [](const fs::path& path)
		{
			std::string ext = path.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
				{
					return static_cast<char>(std::tolower(c));
				});
			return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".dds";
		};

	for (const auto& entry : fs::recursive_directory_iterator(rootDir))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const fs::path filePath = entry.path();
		if (!isTextureFile(filePath))
		{
			continue;
		}

		const fs::path normalizedPath = filePath.lexically_normal();
		const std::string textureKey = normalizedPath.generic_string();
		TextureHandle textureHandle = m_Textures.Load(textureKey, [normalizedPath, sRGB]()
			{
				auto tex = std::make_unique<RenderData::TextureData>();
				tex->path = normalizedPath.generic_string();
				tex->sRGB = sRGB;
				return tex;
			});

		AssetLoadResult result{};
		result.textures.push_back(textureHandle);
		m_AssetsByPath[textureKey] = result;
		StoreReferenceIfMissing(m_TextureRefs, textureHandle, textureKey, 0u);

		fs::path displayPath = fs::relative(normalizedPath, rootDir).lexically_normal();
		displayPath.replace_extension();
		std::string displayName = displayPath.generic_string();
		if (!displayPrefix.empty())
		{
			displayName = displayPrefix + "/" + displayName;
		}
		m_Textures.SetDisplayName(textureHandle, displayName);
	}
}

void AssetLoader::LoadSoundResources(const fs::path& bgmDir, const fs::path& sfxDir)
{
	auto isAudioFile = [](const fs::path& path)
		{
			std::string ext = path.extension().string();
			std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c)
				{
					return static_cast<char>(std::tolower(c));
				});
			return ext == ".wav" || ext == ".ogg" || ext == ".mp3";
		};

	auto loadGroup = [&](const fs::path& directory, std::unordered_map<std::wstring, fs::path>& outMap)
		{
			if (!fs::exists(directory) || !fs::is_directory(directory))
			{
				return;
			}

			for (const auto& entry : fs::directory_iterator(directory))
			{
				if (!entry.is_regular_file())
				{
					continue;
				}

				const fs::path filePath = entry.path();
				if (!isAudioFile(filePath))
				{
					continue;
				}

				outMap[filePath.stem().wstring()] = filePath;
			}
		};

	loadGroup(bgmDir, m_BGMPaths);
	loadGroup(sfxDir, m_SFXPaths);
}

void AssetLoader::LoadShaderSources(const fs::path& shaderDir)
{
	if (!fs::exists(shaderDir) || !fs::is_directory(shaderDir))
	{
		return;
	}

	for (const auto& fileEntry : fs::directory_iterator(shaderDir))
	{
		if (!fileEntry.is_regular_file())
		{
			continue;
		}

		const fs::path& path = fileEntry.path();
		if (path.extension() != ".cso") 
		{
			continue;
		}

		const std::string stem = path.stem().string();
		const bool isVertexShader = ContainsTokenIgnoreCase(stem, "_vs");
		const bool isPixelShader = ContainsTokenIgnoreCase(stem, "_ps");
		if (!isVertexShader && !isPixelShader)
		{
			continue;
		}

		if (isVertexShader)
		{
			VertexShaderHandle handle = m_VertexShaders.Load(path.generic_string(), [path]()
				{
					auto data = std::make_unique<RenderData::VertexShaderData>();
					data->path = path.generic_string();
					return data;
				});
			if (!stem.empty())
			{
				m_VertexShaders.SetDisplayName(handle, stem);
			}
			StoreReferenceIfMissing(m_VertexShaderRefs, handle, path.generic_string(), 0u);
		}

		if (isPixelShader)
		{
			PixelShaderHandle handle = m_PixelShaders.Load(path.generic_string(), [path]()
				{
					auto data = std::make_unique<RenderData::PixelShaderData>();
					data->path = path.generic_string();
					return data;
				});
			if (!stem.empty())
			{
				m_PixelShaders.SetDisplayName(handle, stem);
			}
			StoreReferenceIfMissing(m_PixelShaderRefs, handle, path.generic_string(), 0u);
		}
	}
}

const AssetLoader::AssetLoadResult* AssetLoader::GetAsset(const std::string& assetMetaPath) const
{
	auto it = m_AssetsByPath.find(assetMetaPath);
	if (it == m_AssetsByPath.end())
	{
		return nullptr;
	}

	return &it->second;
}

MaterialHandle AssetLoader::ResolveMaterial(const std::string& assetMetaPath, UINT32 index) const
{
	if (assetMetaPath.empty())
	{
		return MaterialHandle::Invalid();
	}

	const auto* asset = GetAsset(assetMetaPath);
	if (!asset)
	{
		return MaterialHandle::Invalid();
	}

	if (index >= asset->materials.size())
	{
		return MaterialHandle::Invalid();
	}

	return asset->materials[index];
}

MeshHandle AssetLoader::ResolveMesh(const std::string& assetMetaPath, UINT32 index) const
{
	if (assetMetaPath.empty())
	{
		return MeshHandle::Invalid();
	}

	const auto* asset = GetAsset(assetMetaPath);
	if (!asset)
	{
		return MeshHandle::Invalid();
	}

	if (index >= asset->meshes.size())
	{
		return MeshHandle::Invalid();
	}

	return asset->meshes[index];
}

TextureHandle AssetLoader::ResolveTexture(const std::string& assetMetaPath, UINT32 index) const
{
	if (assetMetaPath.empty())
	{
		return TextureHandle::Invalid();
	}

	const auto* asset = GetAsset(assetMetaPath);
	if (!asset)
	{
		return TextureHandle::Invalid();
	}

	if (index >= asset->textures.size())
	{
		return TextureHandle::Invalid();
	}

	return asset->textures[index];
}

SkeletonHandle AssetLoader::ResolveSkeleton(const std::string& assetMetaPath, UINT32 index) const
{
	if (assetMetaPath.empty())
	{
		return SkeletonHandle::Invalid();
	}

	const auto* asset = GetAsset(assetMetaPath);
	if (!asset)
	{
		return SkeletonHandle::Invalid();
	}

	if (index != 0u)
	{
		return SkeletonHandle::Invalid();
	}

	return asset->skeleton;
}

AnimationHandle AssetLoader::ResolveAnimation(const std::string& assetMetaPath, UINT32 index) const
{
	if (assetMetaPath.empty())
	{
		return AnimationHandle::Invalid();
	}

	const auto* asset = GetAsset(assetMetaPath);
	if (!asset)
	{
		return AnimationHandle::Invalid();
	}

	if (index >= asset->animations.size())
	{
		return AnimationHandle::Invalid();
	}

	return asset->animations[index];
}

ShaderAssetHandle AssetLoader::ResolveShaderAsset(const std::string& shaderMetaPath, UINT32 index)
{
	(void)index;
	return LoadShaderAsset(shaderMetaPath);
}

VertexShaderHandle AssetLoader::ResolveVertexShader(const VertexShaderRef& ref)
{
	if (ref.assetPath.empty())
	{
		return VertexShaderHandle::Invalid();
	}

	const fs::path shaderPath = ref.assetPath;
	VertexShaderHandle handle = m_VertexShaders.Load(shaderPath.generic_string(), [shaderPath]()
		{
			auto data = std::make_unique<RenderData::VertexShaderData>();
			data->path = shaderPath.generic_string();
			return data;
		});

	StoreReferenceIfMissing(m_VertexShaderRefs, handle, shaderPath.generic_string(), 0u);
	return handle;
}

PixelShaderHandle AssetLoader::ResolvePixelShader(const PixelShaderRef& ref)
{
	if (ref.assetPath.empty())
	{
		return PixelShaderHandle::Invalid();
	}

	const fs::path shaderPath = ref.assetPath;
	PixelShaderHandle handle = m_PixelShaders.Load(shaderPath.generic_string(), [shaderPath]()
		{
			auto data = std::make_unique<RenderData::PixelShaderData>();
			data->path = shaderPath.generic_string();
			return data;
		});

	StoreReferenceIfMissing(m_PixelShaderRefs, handle, shaderPath.generic_string(), 0u);
	return handle;
}

bool AssetLoader::GetMaterialAssetReference(MaterialHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_MaterialRefs.find(MakeHandleKey(handle));
	if (it == m_MaterialRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetMeshAssetReference(MeshHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_MeshRefs.find(MakeHandleKey(handle));
	if (it == m_MeshRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetTextureAssetReference(TextureHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_TextureRefs.find(MakeHandleKey(handle));
	if (it == m_TextureRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetShaderAssetReference(ShaderAssetHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_ShaderAssetRefs.find(MakeHandleKey(handle));
	if (it == m_ShaderAssetRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetVertexShaderAssetReference(VertexShaderHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_VertexShaderRefs.find(MakeHandleKey(handle));
	if (it == m_VertexShaderRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetPixelShaderAssetReference(PixelShaderHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_PixelShaderRefs.find(MakeHandleKey(handle));
	if (it == m_PixelShaderRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetSkeletonAssetReference(SkeletonHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_SkeletonRefs.find(MakeHandleKey(handle));
	if (it == m_SkeletonRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}

bool AssetLoader::GetAnimationAssetReference(AnimationHandle handle, std::string& outPath, UINT32& outIndex) const
{
	auto it = m_AnimationRefs.find(MakeHandleKey(handle));
	if (it == m_AnimationRefs.end())
	{
		return false;
	}

	outPath = it->second.assetPath;
	outIndex = it->second.assetIndex;
	return true;
}


AssetLoader::AssetLoadResult AssetLoader::LoadAsset(const std::string& assetMetaPath)
{
	AssetLoadResult result{};
	std::ifstream ifs(assetMetaPath);
	if (!ifs)
	{
		return result;
	}

	json meta;
	ifs >> meta;

	const fs::path metaPath(assetMetaPath);
	const fs::path metaDir = metaPath.parent_path();
	const json pathJson = meta.value("path", json::object());
	const fs::path baseDir = (metaDir / pathJson.value("baseDir", "")).lexically_normal();
	const fs::path textureDir = (baseDir / pathJson.value("textureDir", "")).lexically_normal();

	std::vector<MaterialHandle> materialHandles;
	std::unordered_map<std::string, MaterialHandle> materialByName;
	
	LoadMaterials(meta, baseDir, textureDir, result, materialHandles, materialByName, assetMetaPath);

	LoadSkeletons(meta, baseDir, result, assetMetaPath);

	LoadMeshes(meta, baseDir, result, materialHandles, materialByName, assetMetaPath);

	LoadAnimations(meta, baseDir, result, assetMetaPath);

	m_AssetsByPath[assetMetaPath] = result;
	return result;
}

ShaderAssetHandle AssetLoader::LoadShaderAsset(const std::string& shaderMetaPath)
{
	if (shaderMetaPath.empty())
	{
		return ShaderAssetHandle::Invalid();
	}

	json meta = json::object();
	std::string shaderName;
	std::string vsFile;
	std::string psFile;
	const fs::path metaPath(shaderMetaPath);
	const fs::path baseDir = metaPath.parent_path();
	{
		std::ifstream ifs(shaderMetaPath);
		if (ifs)
		{
			ifs >> meta;
			shaderName = meta.value("name", "");
			vsFile = meta.value("vs", "");
			psFile = meta.value("ps", "");
		}
	}
	
	const std::string shaderKey = shaderMetaPath;
	ShaderAssetHandle handle = m_ShaderAssets.Load(shaderKey, [vsFile, psFile, baseDir, this]()
		{
			auto data = std::make_unique<RenderData::ShaderAssetData>();

			if (!vsFile.empty())
			{
				const fs::path vsPath = ResolvePath(baseDir, vsFile);
				VertexShaderHandle vsHandle = m_VertexShaders.Load(vsPath.generic_string(), [vsPath]()
					{
						auto vsData = std::make_unique<RenderData::VertexShaderData>();
						vsData->path = vsPath.generic_string();
						return vsData;
					});
				data->vertexShader = vsHandle;
				StoreReferenceIfMissing(m_VertexShaderRefs, vsHandle, vsPath.generic_string(), 0u);
			}

			if (!psFile.empty())
			{
				const fs::path psPath = ResolvePath(baseDir, psFile);
				PixelShaderHandle psHandle = m_PixelShaders.Load(psPath.generic_string(), [psPath]()
					{
						auto psData = std::make_unique<RenderData::PixelShaderData>();
						psData->path = psPath.generic_string();
						return psData;
					});
				data->pixelShader = psHandle;
				StoreReferenceIfMissing(m_PixelShaderRefs, psHandle, psPath.generic_string(), 0u);
			}

			return data;
		});

	if (!shaderName.empty())
	{
		m_ShaderAssets.SetDisplayName(handle, shaderName);
	}

	StoreReferenceIfMissing(m_ShaderAssetRefs, handle, shaderMetaPath, 0u);
	return handle;
}

static std::string HashShaderKey(const std::string& key)
{
	const size_t hashValue = std::hash<std::string>{}(key);
	std::ostringstream oss;
	oss << std::hex << hashValue;
	return oss.str();
}

ShaderAssetHandle AssetLoader::EnsureShaderAssetForStages(VertexShaderHandle vertexShader, PixelShaderHandle pixelShader)
{
	if (!vertexShader.IsValid() || !pixelShader.IsValid())
	{
		return ShaderAssetHandle::Invalid();
	}

	std::string vsPath;
	std::string psPath;
	UINT32 vsIndex = 0;
	UINT32 psIndex = 0;
	if (!GetVertexShaderAssetReference(vertexShader, vsPath, vsIndex))
	{
		return ShaderAssetHandle::Invalid();
	}

	if (!GetPixelShaderAssetReference(pixelShader, psPath, psIndex))
	{
		return ShaderAssetHandle::Invalid();
	}

	const std::string key = vsPath + "|" + psPath;
	const std::string hash = HashShaderKey(key);
	const std::string assetName = "AutoShader_" + hash;

	const fs::path metaRoot = fs::path("../ResourceOutput") / "ShaderAssets" / "Meta";
	const fs::path metaPath = metaRoot / (assetName + ".shader.json");
	if (fs::exists(metaPath))
	{
		return LoadShaderAsset(metaPath.string());
	}

	fs::create_directories(metaRoot);
	json meta = json::object();
	meta["name"] = assetName;
	meta["vs"] = vsPath;
	meta["ps"] = psPath;

	std::ofstream out(metaPath);
	if (!out)
	{
		return ShaderAssetHandle::Invalid();
	}

	out << meta.dump(4);
	out.close();
	return LoadShaderAsset(metaPath.string());
}


void AssetLoader::LoadMeshes(
	json& meta, 
	const fs::path& baseDir, 
	AssetLoadResult& result, 
	std::vector<MaterialHandle>& materialHandles, 
	std::unordered_map<std::string, MaterialHandle>& materialByName,
	const std::string& assetMetaPath
)
{
	if (meta.contains("meshes") && meta["meshes"].is_array())
	{
		UINT32 meshIndex = 0;
		for (const auto& meshJson : meta["meshes"])
		{
			const std::string meshFile = meshJson.value("file", "");
			if (meshFile.empty())
			{
				++meshIndex;
				continue;
			}

			const fs::path meshPath = ResolvePath(baseDir, meshFile);
			std::ifstream meshStream(meshPath, std::ios::binary);
			if (!meshStream)
			{
				++meshIndex;
				continue;
			}

			MeshBinHeader header{};
			meshStream.read(reinterpret_cast<char*>(&header), sizeof(header));
			if (header.magic != kMeshMagic)
			{
				++meshIndex;
				continue;
			}

			std::vector<SubMeshBin> subMeshes(header.subMeshCount);
			meshStream.read(reinterpret_cast<char*>(subMeshes.data()), sizeof(SubMeshBin) * subMeshes.size());

			//instanceTransforms 읽기
			std::vector<InstanceTransformBin> instanceTransforms;
			if (header.version >= 3 && header.instanceCount > 0)
			{
				instanceTransforms.resize(header.instanceCount);
				meshStream.read(reinterpret_cast<char*>(instanceTransforms.data()),
					sizeof(InstanceTransformBin) * instanceTransforms.size());
			}

			RenderData::MeshData meshData{};
			meshData.hasSkinning = (header.flags & MESH_HAS_SKINNING) != 0;
			meshData.boundsMin = { header.bounds.min[0], header.bounds.min[1], header.bounds.min[2] };
			meshData.boundsMax = { header.bounds.max[0], header.bounds.max[1], header.bounds.max[2] };

			meshData.vertices.reserve(header.vertexCount);
			if (meshData.hasSkinning)
			{
				std::vector<VertexSkinned> vertices(header.vertexCount);
				meshStream.read(reinterpret_cast<char*>(vertices.data()), sizeof(VertexSkinned) * vertices.size());
				for (const auto& v : vertices)
				{
					meshData.vertices.push_back(ToRenderVertex(v));
				}
			}
			else
			{
				std::vector<Vertex> vertices(header.vertexCount);
				meshStream.read(reinterpret_cast<char*>(vertices.data()), sizeof(Vertex) * vertices.size());
				for (const auto& v : vertices)
				{
					meshData.vertices.push_back(ToRenderVertex(v));
				}
			}

			meshData.indices.resize(header.indexCount);
			meshStream.read(reinterpret_cast<char*>(meshData.indices.data()), sizeof(uint32_t) * meshData.indices.size());

#ifdef _DEBUG
			if (meshData.hasSkinning)
			{
				const auto stats = GatherSkinningStats(meshData);
				if (stats.zeroWeightVertexCount > 0)
				{
					std::cout << "[Skinning] mesh=" << meshPath.filename().string()
						<< " zeroWeightVertices=" << stats.zeroWeightVertexCount
						<< " maxBoneIndex=" << stats.maxBoneIndex
						<< std::endl;
				}
			}
#endif

#ifdef _DEBUG
			// 			std::cout << "[MeshBin] load mesh=" << meshPath.filename().string()
			// 				<< " verts=" << meshData.vertices.size()
			// 				<< " indices=" << meshData.indices.size()
			// 				<< " skinned=" << meshData.hasSkinning
			// 				<< std::endl;
			// 			for (auto i = 0; i < meshData.vertices.size(); ++i)
			// 			{
			// 				const auto& v = meshData.vertices[i];
			// 				std::cout << "[MeshBin] v" << i
			// 					<< " pos=(" << v.position.x << "," << v.position.y << "," << v.position.z << ")"
			// 					<< " n=(" << v.normal.x << "," << v.normal.y << "," << v.normal.z << ")"
			// 					<< " uv=(" << v.uv.x << "," << v.uv.y << ")"
			// 					<< " t=(" << v.tangent.x << "," << v.tangent.y << "," << v.tangent.z << "," << v.tangent.w << ")"
			// 					<< std::endl;
			// 			}
			// 			for (size_t i = 0; i + 2 < meshData.indices.size(); i += 3)
			// 			{
			// 				std::cout << "[MeshBin] tri" << (i / 3)
			// 					<< " idx=(" << meshData.indices[i] << "," << meshData.indices[i + 1] << "," << meshData.indices[i + 2] << ")"
			// 					<< std::endl;
			// 			}
#endif

			std::string stringTable;
			if (header.stringTableBytes > 0)
			{
				stringTable.resize(header.stringTableBytes);
				meshStream.read(stringTable.data(), header.stringTableBytes);
			}

#ifdef _DEBUG
			//WriteMeshBinLoadDebugJson(meshPath, header, subMeshes, meshData);
#endif

			meshData.subMeshes.reserve(subMeshes.size());
			for (const auto& subMesh : subMeshes)
			{
				const std::string materialName = ReadStringAtOffset(stringTable, subMesh.materialNameOffset);
				const std::string subName = ReadStringAtOffset(stringTable, subMesh.nameOffset);

				auto resolveMat = [&]() -> MaterialHandle
					{
						if (!materialName.empty())
						{
							auto it = materialByName.find(materialName);
							if (it != materialByName.end()) return it->second;
						}
						if (materialHandles.size() == 1) return materialHandles.front();
						return MaterialHandle::Invalid();
					};

				const MaterialHandle mat = resolveMat();

				// v2 (또는 v3인데 instanceTransforms 없음) -> 1개만, identity
				if (header.version < 3 || instanceTransforms.empty() || subMesh.instanceCount == 0)
				{
					RenderData::MeshData::SubMesh out{};
					out.indexStart = subMesh.indexStart;
					out.indexCount = subMesh.indexCount;
					out.material = mat;
					out.name = subName;
					out.boundsMin = { subMesh.bounds.min[0], subMesh.bounds.min[1], subMesh.bounds.min[2] };
					out.boundsMax = { subMesh.bounds.max[0], subMesh.bounds.max[1], subMesh.bounds.max[2] };

					DirectX::XMFLOAT4X4 id{};
					id._11 = id._22 = id._33 = id._44 = 1.0f;
					out.localToWorld = id;

					meshData.subMeshes.push_back(std::move(out));
					continue;
				}

				// v3 -> instanceCount 만큼 풀어서 push
				const uint32_t start = subMesh.instanceStart;
				const uint32_t count = subMesh.instanceCount;

				for (uint32_t i = 0; i < count; ++i)
				{
					const uint32_t idx = start + i;
					if (idx >= (uint32_t)instanceTransforms.size()) break;

					RenderData::MeshData::SubMesh out{};
					out.indexStart = subMesh.indexStart;
					out.indexCount = subMesh.indexCount;
					out.material = mat;
					out.name = subName;
					out.boundsMin = { subMesh.bounds.min[0], subMesh.bounds.min[1], subMesh.bounds.min[2] };
					out.boundsMax = { subMesh.bounds.max[0], subMesh.bounds.max[1], subMesh.bounds.max[2] };
					out.localToWorld._11 = instanceTransforms[idx].m[0];
					out.localToWorld._12 = instanceTransforms[idx].m[1];
					out.localToWorld._13 = instanceTransforms[idx].m[2];
					out.localToWorld._14 = instanceTransforms[idx].m[3];
					out.localToWorld._21 = instanceTransforms[idx].m[4];
					out.localToWorld._22 = instanceTransforms[idx].m[5];
					out.localToWorld._23 = instanceTransforms[idx].m[6];
					out.localToWorld._24 = instanceTransforms[idx].m[7];
					out.localToWorld._31 = instanceTransforms[idx].m[8];
					out.localToWorld._32 = instanceTransforms[idx].m[9];
					out.localToWorld._33 = instanceTransforms[idx].m[10];
					out.localToWorld._34 = instanceTransforms[idx].m[11];
					out.localToWorld._41 = instanceTransforms[idx].m[12];
					out.localToWorld._42 = instanceTransforms[idx].m[13];
					out.localToWorld._43 = instanceTransforms[idx].m[14];
					out.localToWorld._44 = instanceTransforms[idx].m[15];

					meshData.subMeshes.push_back(std::move(out));
				}
			}

			RebuildMeshBoundsFromSubMeshes(meshData);

			MeshHandle handle = m_Meshes.Load(meshPath.generic_string(), [meshData]()
				{
					return std::make_unique<RenderData::MeshData>(meshData);
				});
			result.meshes.push_back(handle);

			if (handle.IsValid())
			{
				StoreReferenceIfMissing(m_MeshRefs, handle, assetMetaPath, meshIndex);
			}
			++meshIndex;
		}
	}
}

void AssetLoader::LoadMaterials(json& meta, const fs::path& baseDir, const fs::path& textureDir, AssetLoadResult& result, std::vector<MaterialHandle>& materialHandles, std::unordered_map<std::string, MaterialHandle>& materialByName, const std::string& assetMetaPath)
{
	const json filesJson = meta.value("files", json::object());
	const std::string materialsFile = filesJson.value("materials", "");

	if (!materialsFile.empty())
	{
		const fs::path materialPath = ResolvePath(baseDir, materialsFile);
		std::ifstream matStream(materialPath, std::ios::binary);
		if (matStream)
		{
			MatBinHeader header{};
			matStream.read(reinterpret_cast<char*>(&header), sizeof(header));
			if (header.magic == kMatMagic && header.materialCount > 0)
			{
				std::vector<MatData> mats(header.materialCount);
				matStream.read(reinterpret_cast<char*>(mats.data()), sizeof(MatData) * mats.size());

				std::string stringTable;
				if (header.stringTableBytes > 0)
				{
					stringTable.resize(header.stringTableBytes);
					matStream.read(stringTable.data(), header.stringTableBytes);
				}

				materialHandles.reserve(mats.size());
				materialByName.reserve(mats.size());
				for (size_t i = 0; i < mats.size(); ++i)
				{
					const MatData& mat = mats[i];
					RenderData::MaterialData material{};
					material.baseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
					material.metallic = mat.metallic;
					material.roughness = mat.roughness;

					std::string materialName = ReadStringAtOffset(stringTable, mat.materialNameOffset);
					if (materialName.empty())
					{
						materialName = "Material_" + std::to_string(i);
					}

					for (size_t t = 0; t < TEX_MAX; ++t)
					{
						const std::string texPathRaw = ReadStringAtOffset(stringTable, mat.texPathOffset[t]);
						if (texPathRaw.empty())
						{
							continue;
						}

						bool slotValid = false;
						const auto slot = ToMaterialSlot(static_cast<ETextureType>(t), slotValid);
						if (!slotValid)
						{
							continue;
						}

						const fs::path texPath = ResolvePath(textureDir, texPathRaw);
						const bool isSRGB = (slot == RenderData::MaterialTextureSlot::Albedo)
							|| (slot == RenderData::MaterialTextureSlot::Emissive);

						TextureHandle textureHandle = m_Textures.Load(
							texPath.generic_string(),
							[texPath, isSRGB]()
							{
								auto tex = std::make_unique<RenderData::TextureData>();
								tex->path = texPath.generic_string();
								tex->sRGB = isSRGB;
								return tex;
							});
						material.textures[static_cast<size_t>(slot)] = textureHandle;
						if (PushUnique(result.textures, textureHandle))
						{
							const UINT32 textureIndex = static_cast<UINT32>(result.textures.size() - 1);
							StoreReferenceIfMissing(m_TextureRefs, textureHandle, assetMetaPath, textureIndex);
						}
					}

					const std::string materialKey = materialPath.generic_string() + ":" + materialName;
					MaterialHandle handle = m_Materials.Load(materialKey, [material]()
						{
							return std::make_unique<RenderData::MaterialData>(material);
						});


					materialHandles.push_back(handle);
					materialByName.emplace(materialName, handle);
					result.materials.push_back(handle);
					StoreReferenceIfMissing(m_MaterialRefs, handle, assetMetaPath, static_cast<UINT32>(i));
				}
			}
		}
	}
}

void AssetLoader::LoadSkeletons(json& meta, const fs::path& baseDir, AssetLoadResult& result, const std::string& assetMetaPath)
{
	const json filesJson = meta.value("files", json::object());
	const std::string skeletonFile = filesJson.value("skeleton", "");
	
	if (!skeletonFile.empty())
	{
		const fs::path skelPath = ResolvePath(baseDir, skeletonFile);
		std::ifstream skelStream(skelPath, std::ios::binary);
		if (skelStream)
		{
			SkelBinHeader header{};
			skelStream.read(reinterpret_cast<char*>(&header), sizeof(header));

			if (header.magic == kSkelMagic && header.boneCount > 0)
			{
				RenderData::Skeleton skeleton{};
				if (header.version >= 4)
				{
					SkelBinEquipmentData equipmentData{};
					skelStream.read(reinterpret_cast<char*>(&equipmentData), sizeof(equipmentData));
					skeleton.equipmentBoneIndex = equipmentData.equipmentBoneIndex;
					std::memcpy(&skeleton.equipmentBindPose, equipmentData.equipmentBindPose, sizeof(float) * 16);
				}
				else
				{
					skeleton.equipmentBoneIndex = -1;
					DirectX::XMStoreFloat4x4(&skeleton.equipmentBindPose, DirectX::XMMatrixIdentity());
				}

				if (header.version >= 3)
				{
					float globalInverse[16]{};
					skelStream.read(reinterpret_cast<char*>(globalInverse), sizeof(float) * 16);
					std::memcpy(&skeleton.globalInverseTransform, globalInverse, sizeof(float) * 16);
				}
				else
				{
					DirectX::XMStoreFloat4x4(&skeleton.globalInverseTransform, DirectX::XMMatrixIdentity());
				}

				std::vector<BoneBin> bones(header.boneCount);
				skelStream.read(reinterpret_cast<char*>(bones.data()), sizeof(BoneBin) * bones.size());

				std::string stringTable;
				if (header.stringTableBytes > 0)
				{
					stringTable.resize(header.stringTableBytes);
					skelStream.read(stringTable.data(), header.stringTableBytes);
				}

				skeleton.bones.reserve(bones.size());
				const int32_t boneCount = static_cast<int32_t>(bones.size());
				for (const auto& bone : bones)
				{
					RenderData::Bone out{};
					out.name = ReadStringAtOffset(stringTable, bone.nameOffset);
					if (bone.parentIndex >= 0 && bone.parentIndex < boneCount)
					{
						out.parentIndex = bone.parentIndex;
					}
					else
					{
						out.parentIndex = -1;
					}
					std::memcpy(&out.bindPose, bone.localBind, sizeof(float) * 16);
					std::memcpy(&out.inverseBindPose, bone.inverseBindPose, sizeof(float) * 16);
					skeleton.bones.push_back(std::move(out));
				}

				if (header.upperCount > 0)
				{
					std::vector<int32_t> indices(header.upperCount);
					skelStream.read(reinterpret_cast<char*>(indices.data()), sizeof(int32_t) * indices.size());
					skeleton.upperBodyBones.assign(indices.begin(), indices.end());
				}
				if (header.lowerCount > 0)
				{
					std::vector<int32_t> indices(header.lowerCount);
					skelStream.read(reinterpret_cast<char*>(indices.data()), sizeof(int32_t) * indices.size());
					skeleton.lowerBodyBones.assign(indices.begin(), indices.end());
				}

#ifdef _DEBUG
				//WriteSkeletonLoadDebug(skelPath, skeleton);
#endif

				result.skeleton = m_Skeletons.Load(skelPath.generic_string(), [skeleton]()
					{
						return std::make_unique<RenderData::Skeleton>(skeleton);
					});

				if (result.skeleton.IsValid())
				{
					StoreReferenceIfMissing(m_SkeletonRefs, result.skeleton, assetMetaPath, 0u);
				}
			}
		}
	}
}

void AssetLoader::LoadAnimations(json& meta, const fs::path& baseDir, AssetLoadResult& result, const std::string& assetMetaPath)
{
	if (meta.contains("animations") && meta["animations"].is_array())
	{
		UINT32 animationIndex = 0;
		for (const auto& animJson : meta["animations"])
		{
			const std::string animFile = animJson.value("file", "");
			if (animFile.empty())
			{
				++animationIndex;
				continue;
			}

			const fs::path animPath = ResolvePath(baseDir, animFile);
			std::ifstream animStream(animPath);
			if (!animStream)
			{
				++animationIndex;
				continue;
			}

			json animData;
			animStream >> animData;
			RenderData::AnimationClip clip = ParseAnimationJson(animData);

#ifdef _DEBUG
			//WriteAnimationLoadDebug(animPath, clip);
#endif

			AnimationHandle handle = m_Animations.Load(animPath.generic_string(), [clip]()
				{
					return std::make_unique<RenderData::AnimationClip>(clip);
				});

			result.animations.push_back(handle);
			if (handle.IsValid())
			{
				StoreReferenceIfMissing(m_AnimationRefs, handle, assetMetaPath, animationIndex);
			}
			++animationIndex;
		}
	}
}
