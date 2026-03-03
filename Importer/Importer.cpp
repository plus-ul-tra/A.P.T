#include "pch.h"
#include "Importer.h"
#include "iostream"
#include <filesystem>
#include <assimp/config.h>
namespace fs = std::filesystem;

static std::string ToGenericString(const fs::path& p)
{
	// JSON/툴링에서 슬래시 통일용
	return p.generic_string();
}

static void EnsureDir(const fs::path& p)
{
	std::error_code ec;
	fs::create_directories(p, ec);
	// ec 무시할지 assert할지 정책에 맞게 처리
}

static std::string RelToBase(const fs::path& baseDir, const fs::path& fullPath)
{
	std::error_code ec;
	fs::path rel = fs::relative(fullPath, baseDir, ec);
	if (ec) return ToGenericString(fullPath); // fallback
	return ToGenericString(rel);
}


static std::string EmbeddedTextureExtension(const aiTexture* texture)
{
	if (!texture) return "png";
	return (texture->mHeight == 0) ? "png" : "rgba";
}

static std::string TextureTypeSuffix(aiTextureType type)
{
	switch (type)
	{
	case aiTextureType_BASE_COLOR:
	case aiTextureType_DIFFUSE:
		return "albedo";
	case aiTextureType_NORMALS:
	case aiTextureType_HEIGHT:
		return "normal";
	case aiTextureType_SPECULAR:
		return "specular";
	case aiTextureType_SHININESS:
		return "glossiness";
	case aiTextureType_METALNESS:
		return "metallic";
	case aiTextureType_DIFFUSE_ROUGHNESS:
		return "roughness";
	case aiTextureType_AMBIENT_OCCLUSION:
		return "ao";
	case aiTextureType_EMISSIVE:
		return "emissive";
	}

	return "type_" + std::to_string(static_cast<int>(type));
}

static std::string TextureTypeLabel(aiTextureType type)
{
	switch (type)
	{
	case aiTextureType_BASE_COLOR:
	case aiTextureType_DIFFUSE:
		return "albedo";
	case aiTextureType_NORMALS:
	case aiTextureType_HEIGHT:
		return "normal";
	case aiTextureType_SPECULAR:
		return "specular";
	case aiTextureType_SHININESS:
		return "glossiness";
	case aiTextureType_METALNESS:
		return "metallic";
	case aiTextureType_DIFFUSE_ROUGHNESS:
		return "roughness";
	case aiTextureType_AMBIENT_OCCLUSION:
		return "ao";
	case aiTextureType_EMISSIVE:
		return "emissive";
	}

	return "type_" + std::to_string(static_cast<int>(type));
}

static std::string TextureTypeInfo(aiTextureType type)
{
	return TextureTypeLabel(type) + "(" + TextureTypeSuffix(type) + ")";
}

static std::string BaseNameFromPath(const std::string& path)
{
	const size_t pos = path.find_last_of("/\\");
	if (pos == std::string::npos) return path;
	return path.substr(pos + 1);
}

static bool TryParseEmbeddedIndex(const aiString& path, uint32_t& outIndex)
{
	const std::string rawPath = path.C_Str();
	if (rawPath.empty()) return false;

	const char* parseStart = nullptr;
	if (rawPath[0] == '*')
	{
		parseStart = rawPath.c_str() + 1;
	}
	else if (rawPath.rfind("embedded_", 0) == 0)
	{
		parseStart = rawPath.c_str() + std::strlen("embedded_");
	}
	else if (std::isdigit(static_cast<unsigned char>(rawPath[0])) != 0)
	{
		parseStart = rawPath.c_str();
	}
	else
	{
		return false;
	}

	char* endPtr = nullptr;
	const unsigned long parsed = std::strtoul(parseStart, &endPtr, 10);
	if (endPtr == parseStart) return false;

	outIndex = static_cast<uint32_t>(parsed);
	return true;
}

static bool TryResolveEmbeddedIndex(const aiScene* scene, const aiString& path, uint32_t& outIndex)
{
	if (TryParseEmbeddedIndex(path, outIndex)) return true;
	if (!scene || scene->mNumTextures == 0) return false;

	const std::string rawPath = path.C_Str();
	if (rawPath.empty()) return false;

	const std::string rawBase = BaseNameFromPath(rawPath);

	for (uint32_t i = 0; i < scene->mNumTextures; ++i)
	{
		const aiTexture* texture = scene->mTextures[i];
		if (!texture) continue;

		const std::string texName = texture->mFilename.C_Str();
		if (texName.empty()) continue;

		const std::string texBase = BaseNameFromPath(texName);

		if (rawPath == texName || rawPath == texBase || rawBase == texName || rawBase == texBase)
		{
			outIndex = i;
			return true;
		}
	}

	return false;
}

static std::unordered_map<uint32_t, std::vector<aiTextureType>> CollectEmbeddedTextureUsage(const aiScene* scene)
{
	std::unordered_map<uint32_t, std::vector<aiTextureType>> usage;
	if (!scene || !scene->HasMaterials()) return usage;

	static const aiTextureType kTypes[] = {
		aiTextureType_BASE_COLOR,
		aiTextureType_DIFFUSE,
		aiTextureType_NORMALS,
		aiTextureType_HEIGHT,
		aiTextureType_SPECULAR,
		aiTextureType_SHININESS,
		aiTextureType_METALNESS,
		aiTextureType_DIFFUSE_ROUGHNESS,
		aiTextureType_AMBIENT_OCCLUSION,
		aiTextureType_EMISSIVE
	};

	std::unordered_set<uint64_t> seen;
	for (uint32_t m = 0; m < scene->mNumMaterials; ++m)
	{
		const aiMaterial* mat = scene->mMaterials[m];
		if (!mat) continue;

		for (aiTextureType type : kTypes)
		{
			const unsigned int textureCount = mat->GetTextureCount(type);
			for (unsigned int t = 0; t < textureCount; ++t)
			{
				aiString path;
				if (mat->GetTexture(type, t, &path) != AI_SUCCESS) continue;

				uint32_t index = 0;
				if (!TryResolveEmbeddedIndex(scene, path, index)) continue;

				const uint64_t key = (static_cast<uint64_t>(index) << 32) | static_cast<uint32_t>(type);
				if (!seen.insert(key).second) continue;

				usage[index].push_back(type);
			}
		}
	}

	return usage;
}

static void WriteEmbeddedTextures(
	const aiScene* scene,
	const fs::path& texDir,
	const std::unordered_map<uint32_t, std::vector<aiTextureType>>& usage)
{
	if (!scene || scene->mNumTextures == 0) return;

	for (uint32_t i = 0; i < scene->mNumTextures; ++i)
	{
		const aiTexture* texture = scene->mTextures[i];
		if (!texture) continue;

		const std::string extension = EmbeddedTextureExtension(texture);
		std::vector<fs::path> outPaths;

		auto it = usage.find(i);
		if (it == usage.end() || it->second.empty())
		{
			outPaths.push_back(texDir / ("embedded_" + std::to_string(i) + "_texture." + extension));
		}
		else
		{
			for (aiTextureType type : it->second)
			{
				std::string suffix = TextureTypeSuffix(type);
				if (!suffix.empty())
				{
					suffix[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(suffix[0])));
				}
				outPaths.push_back(texDir / ("embedded_" + std::to_string(i) + "_" + suffix + "." + extension));
			}
		}

		for (const auto& outPath : outPaths)
		{
			std::ofstream ofs(outPath, std::ios::binary);
			if (!ofs) continue;

			if (texture->mHeight == 0)
			{
				const size_t byteCount = static_cast<size_t>(texture->mWidth);
				ofs.write(reinterpret_cast<const char*>(texture->pcData), byteCount);
			}
			else
			{
				const size_t pixelCount = static_cast<size_t>(texture->mWidth) * texture->mHeight;
				std::vector<unsigned char> rgba(pixelCount * 4);
				for (size_t p = 0; p < pixelCount; ++p)
				{
					const aiTexel& texel = texture->pcData[p];
					rgba[p * 4 + 0] = texel.r;
					rgba[p * 4 + 1] = texel.g;
					rgba[p * 4 + 2] = texel.b;
					rgba[p * 4 + 3] = texel.a;
				}

				ofs.write(reinterpret_cast<const char*>(rgba.data()), rgba.size());
			}
		}
	}
}


static std::string CopyExternalTexture(
	const fs::path& fbxDir,
	const fs::path& texDir,
	const std::string& rawPath
)
{
	if (rawPath.empty()) return{};

	fs::path sourcePath = fs::u8path(rawPath);
	if (!sourcePath.is_absolute())
	{
		sourcePath = fbxDir / sourcePath;
	}

	std::error_code ec;
	fs::path normalizedSource = fs::weakly_canonical(sourcePath, ec);
	if (ec)
	{
		normalizedSource = sourcePath;
	}

	if (!fs::exists(normalizedSource))
	{
		return {};
	}

	const fs::path destPath = texDir / normalizedSource.filename();
	fs::copy_file(normalizedSource, destPath, fs::copy_options::overwrite_existing, ec);
	if (ec)
	{
		return {};
	}

	return ToGenericString(destPath.filename());
}

static std::unordered_map<std::string, std::string> CollectExternalTextureRemap(
	const aiScene* scene,
	const fs::path& fbxDir,
	const fs::path& texDir)
{
	std::unordered_map<std::string, std::string> remap;
	if (!scene || !scene->HasMaterials()) return remap;

	static const aiTextureType kTypes[] = {
		aiTextureType_BASE_COLOR,
		aiTextureType_DIFFUSE,
		aiTextureType_NORMALS,
		aiTextureType_HEIGHT,
		aiTextureType_METALNESS,
		aiTextureType_DIFFUSE_ROUGHNESS,
		aiTextureType_AMBIENT_OCCLUSION,
		aiTextureType_EMISSIVE
	};

	for (uint32_t m = 0; m < scene->mNumMaterials; ++m)
	{
		const aiMaterial* mat = scene->mMaterials[m];
		if (!mat) continue;

		for (aiTextureType type : kTypes)
		{
			const unsigned int textureCount = mat->GetTextureCount(type);
			for (unsigned int t = 0; t < textureCount; ++t)
			{
				aiString path;
				if (mat->GetTexture(type, t, &path) != AI_SUCCESS) continue;

				uint32_t index = 0;
				if (TryResolveEmbeddedIndex(scene, path, index)) continue;

				const std::string rawPath = path.C_Str();
				if (rawPath.empty()) continue;

				if (remap.find(rawPath) != remap.end()) continue;

				const std::string resolved = CopyExternalTexture(fbxDir, texDir, rawPath);
				if (resolved.empty()) continue;

				remap.emplace(rawPath, resolved);

				const std::string baseName = BaseNameFromPath(rawPath);
				if (!baseName.empty())
				{
					remap.emplace(baseName, resolved);
				}
			}
		}
	}

	return remap;
}

static bool HasKeyword(const std::string& value, const std::string& keyword)
{
	const auto it = std::search(
		value.begin(),
		value.end(),
		keyword.begin(),
		keyword.end(),
		[](char a, char b)
		{
			return std::tolower(static_cast<unsigned char>(a)) ==
				std::tolower(static_cast<unsigned char>(b));
		});
	return it != value.end();
}

static void TryAssignFallbackTexture(
	std::unordered_map<ETextureType, std::string>& out,
	ETextureType slot,
	const fs::path& filePath)
{
	if (out.find(slot) != out.end())
	{
		return;
	}

	out.emplace(slot, ToGenericString(filePath.filename()));
}

static std::unordered_map<ETextureType, std::string> CollectFallbackTextures(const fs::path& texDir)
{
	std::unordered_map<ETextureType, std::string> fallback;
	if (!fs::exists(texDir))
	{
		return fallback;
	}

	for (const auto& entry : fs::directory_iterator(texDir))
	{
		if (!entry.is_regular_file())
		{
			continue;
		}

		const std::string filename = entry.path().filename().string();

		if (HasKeyword(filename, "BaseColor") || HasKeyword(filename, "Albedo") || HasKeyword(filename, "Diffuse"))
		{
			TryAssignFallbackTexture(fallback, ETextureType::ALBEDO, entry.path());
		}
		else if (HasKeyword(filename, "Normal"))
		{
			TryAssignFallbackTexture(fallback, ETextureType::NORMAL, entry.path());
		}
		else if (HasKeyword(filename, "Metal"))
		{
			TryAssignFallbackTexture(fallback, ETextureType::METALLIC, entry.path());
		}
		else if (HasKeyword(filename, "Rough"))
		{
			TryAssignFallbackTexture(fallback, ETextureType::ROUGHNESS, entry.path());
		}
		else if (HasKeyword(filename, "AO") || HasKeyword(filename, "Occlusion"))
		{
			TryAssignFallbackTexture(fallback, ETextureType::AO, entry.path());
		}
		else if (HasKeyword(filename, "Emissive") || HasKeyword(filename, "Emission"))
		{
			TryAssignFallbackTexture(fallback, ETextureType::EMISSIVE, entry.path());
		}
	}

	return fallback;
}



// file: "Hero_Idle.anim.json" or ".../Hero_Idle.anim.json"
// baseName: "Hero"  -> returns "Idle"
static std::string ExtractClipName(const std::string& filePath, const std::string& baseName)
{
	fs::path p = fs::u8path(filePath);
	std::string filename = p.filename().u8string();

	const std::string suffix = ".anim.json";
	if (filename.size() >= suffix.size() &&
		filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0)
	{
		filename.erase(filename.size() - suffix.size()); // remove ".anim.json"
	}
	else
	{
		// fallback: 그냥 stem
		filename = p.stem().u8string();
	}

	const std::string prefix = baseName + "_";
	if (filename.rfind(prefix, 0) == 0) // starts_with
	{
		filename.erase(0, prefix.size()); // remove "Hero_"
	}

	// SanitizeName는 파일명용이니까, "이름"에는 굳이 안 해도 되지만
	// 프로젝트 정책상 통일 원하면 여기서 적용
	return filename;
}

static void DebugDumpAssimpScene(const aiScene* scene)
{
	if (!scene || !scene->mRootNode)
	{
		printf("[ASSIMP] scene or root null\n");
		return;
	}

	printf("[ASSIMP] meshes=%u animations=%u root=%s\n",
		scene->mNumMeshes,
		scene->mNumAnimations,
		scene->mRootNode->mName.C_Str());

	for (unsigned m = 0; m < scene->mNumMeshes; ++m)
	{
		const aiMesh* mesh = scene->mMeshes[m];
		if (!mesh) continue;
		printf("  mesh[%u] bones=%u verts=%u\n", m, mesh->mNumBones, mesh->mNumVertices);
	}

	for (unsigned a = 0; a < scene->mNumAnimations; ++a)
	{
		const aiAnimation* anim = scene->mAnimations[a];
		if (!anim) continue;
		printf("  anim[%u] name=%s channels=%u duration=%f ticks=%f\n",
			a,
			anim->mName.C_Str(),
			anim->mNumChannels,
			(float)anim->mDuration,
			(float)anim->mTicksPerSecond);

		const unsigned dumpCount = std::min(5u, anim->mNumChannels);
		for (unsigned c = 0; c < dumpCount; ++c)
		{
			const aiNodeAnim* ch = anim->mChannels[c];
			if (!ch) continue;
			printf("    ch[%u] node=%s posKeys=%u rotKeys=%u scaleKeys=%u\n",
				c,
				ch->mNodeName.C_Str(),
				ch->mNumPositionKeys,
				ch->mNumRotationKeys,
				ch->mNumScalingKeys);
		}
	}

	// node name dump (limited)
	struct StackItem
	{
		const aiNode* n;
		int depth;
		aiMatrix4x4 parent;
	};

	std::vector<StackItem> st;
	aiMatrix4x4 identity;
	st.push_back({ scene->mRootNode, 0, identity });

	unsigned printed = 0;
	while (!st.empty() && printed < 200)
	{
		auto [n, d, parent] = st.back();
		st.pop_back();
		if (!n) continue;

		aiVector3D localScale{};
		aiQuaternion localRot{};
		aiVector3D localTranslate{};
		n->mTransformation.Decompose(localScale, localRot, localTranslate);

		const aiMatrix4x4 global = parent * n->mTransformation;
		aiVector3D globalScale{};
		aiQuaternion globalRot{};
		aiVector3D globalTranslate{};
		global.Decompose(globalScale, globalRot, globalTranslate);

		printf("  node d=%d name=%s children=%u localScale=(%.3f, %.3f, %.3f) globalScale=(%.3f, %.3f, %.3f) localPos=(%.3f, %.3f, %.3f) globalPos=(%.3f, %.3f, %.3f)\n",
			d,
			n->mName.C_Str(),
			n->mNumChildren,
			localScale.x, localScale.y, localScale.z,
			globalScale.x, globalScale.y, globalScale.z,
			localTranslate.x, localTranslate.y, localTranslate.z,
			globalTranslate.x, globalTranslate.y, globalTranslate.z);

		for (int i = (int)n->mNumChildren - 1; i >= 0; --i)
			st.push_back({ n->mChildren[i], d + 1, global });
	}
}

void ImportFBX(const std::string& FBXPath, const std::string& outDir)
{
	// ----- 1) 이름/경로 규칙: 단일 진실원천 -----
	const fs::path fbxPath = fs::u8path(FBXPath);
	const std::string assetName = SanitizeName(fbxPath.stem().u8string());

	const fs::path baseDir     = fs::u8path(outDir) / assetName; // asset 폴더(진짜 루트)
	const fs::path meshesDir   = baseDir / "Meshes";
	const fs::path animsDir    = baseDir / "Anims";
	const fs::path matsDir     = baseDir / "Mats";
	const fs::path texDir      = baseDir / "Textures";
	const fs::path skelDir     = baseDir / "Skels";
	const fs::path metaDir     = baseDir / "Meta";
	const fs::path skelMetaDir = baseDir / "SkelMeta";

	EnsureDir(meshesDir);
	EnsureDir(animsDir);
	EnsureDir(matsDir);
	EnsureDir(texDir);
	EnsureDir(skelDir);
	EnsureDir(metaDir);
	EnsureDir(skelMetaDir);

	const fs::path skelBinPath  = skelDir     / (assetName + ".skelbin");
	const fs::path matBinPath   = matsDir     / (assetName + ".matbin");
	const fs::path metaPath     = metaDir     / (assetName + ".asset.json");
	const fs::path skelMetaPath = skelMetaDir / (assetName + ".skelmeta.json");


	// ----- 2) Assimp load -----
	Assimp::Importer importer;

	// DX11 / LH 기준
	const aiScene* scene = importer.ReadFile(
		FBXPath,
		aiProcess_Triangulate				|		// 모든 면을 삼각형으로 반환
		aiProcess_GenNormals				|		// 노멀 생성
		aiProcess_CalcTangentSpace			|		// 탄젠트 공간 계산
		aiProcess_GlobalScale				|		// CM화
		aiProcess_JoinIdenticalVertices		|       // 중복 정점 제거
		aiProcess_ConvertToLeftHanded				// LH 변환
	);
	if (!scene) {
		printf("Assimp error: %s\n", importer.GetErrorString());
	}
	if (!scene) return ;
#ifdef _DEBUG
	//DebugDumpAssimpScene(scene);   // ← 여기
#endif
	
	// ----- 3) Import pipeline -----
	const auto embeddedUsage = CollectEmbeddedTextureUsage(scene);
	WriteEmbeddedTextures(scene, texDir, embeddedUsage);
	const auto externalTextureRemap = CollectExternalTextureRemap(scene, fbxPath.parent_path(), texDir);
	const auto fallbackTextureMap = CollectFallbackTextures(texDir);

	nlohmann::json skeletonMeta = nlohmann::json::object();
	if (fs::exists(skelMetaPath))
	{
		std::ifstream skelMetaStream(skelMetaPath);
		if (skelMetaStream)
		{
			try
			{
				skelMetaStream >> skeletonMeta;
			}
			catch (const std::exception&)
			{
				skeletonMeta = nlohmann::json::object();
			}
		}
	}

	std::unordered_map<std::string, uint32_t> boneMap;
	// Skeleton (없으면 boneMap 비거나 skel 파일 안 만들어도 됨)
	if (!ImportFBXToSkelBin(scene, ToGenericString(skelBinPath), boneMap, skeletonMeta))
	{
		std::cout << "No Skel" << std::endl;
	}

	std::vector<std::string> meshFiles;
	if (!ImportFBXToMeshBin(scene, ToGenericString(meshesDir), assetName, boneMap, meshFiles))
	{
		std::cout << "No Mesh" << std::endl;
	}

	std::vector<MetaMeshItem> metaMeshes;

	for (auto& file : meshFiles)
	{
		const bool isSkinned = (file.find("_skinned") != std::string::npos);

		// ImportFBXToMeshBin이 fileName만 반환한다고 가정 -> "Meshes/<fileName>" 형태로 meta에 기록
		fs::path fn = fs::u8path(file).filename();
		std::string rel = ToGenericString(fs::path("Meshes") / fn);

		metaMeshes.push_back({ rel, isSkinned });
	}

	std::vector<std::string> animFiles;
	if (!ImportFBXToAnimJson(scene, ToGenericString(animsDir), assetName, boneMap, animFiles))
	{
		std::cout << "No Anim" << std::endl;
	}

	std::vector<MetaAnimItem> metaAnims;

	for (auto& file : animFiles)
	{
		// file이 파일명만 와도 되고, 전체 경로가 와도 됨 -> filename만 뽑아서 규칙으로 처리
		fs::path fn = fs::u8path(file).filename();
		std::string rel = ToGenericString(fs::path("Anims") / fn);

		std::string clipName = ExtractClipName(fn.u8string(), assetName);

		metaAnims.push_back({ SanitizeName(clipName), rel });
	}

	if (!ImportFBXToMaterialBin(scene, ToGenericString(matBinPath), embeddedUsage, externalTextureRemap, fallbackTextureMap))
	{
		std::cout << "No Mat" << std::endl;
	}

	// ----- 4) Write meta -----
	// meta는 Meta 폴더에 있으므로, 런타임 기준(metaDir)에서 asset 루트(baseDir)로 올라가게 baseDir=".."
	// textureDir은 asset 루트 기준 "Textures"로 통일
	const std::string metaBaseDir = "..";
	const std::string metaTextureDir = "Textures";

	if (!WriteAssetMetaJson(
		ToGenericString(metaPath),			// outPath
		assetName,							// assetName
		metaBaseDir,						// baseDir (meta 파일 기준)
		metaTextureDir,                     // textureDir (asset 루트 기준)
		RelToBase(baseDir, matBinPath),		// matBin	상대경로
		RelToBase(baseDir, skelBinPath),	// skelBin	상대경로
		metaMeshes,
		metaAnims,
		skeletonMeta))
	{
		std::cout << "No Meta" << std::endl;
	}

	// Debug용 skeletonMeta
	if (!skeletonMeta.is_null() && !skeletonMeta.empty())
	{
		std::ofstream skelMetaOut(skelMetaPath);
		if (skelMetaOut)
		{
			skelMetaOut << skeletonMeta.dump(2);
		}
	}
}

void ImportAll()
{
	const fs::path fbxRoot    = "../Resources/FBX";
	const fs::path outputRoot = "../ResourceOutput";

	if (!fs::exists(fbxRoot) || !fs::is_directory(fbxRoot))
		return;

	for (const auto& entry : fs::directory_iterator(fbxRoot))
	{
		if (!entry.is_regular_file())
			continue;

		const fs::path& path = entry.path();

		if (path.extension() != ".fbx")
			continue;

#ifdef _DEBUG
	//	std::cout << path.string() << std::endl;
#endif
		ImportFBX(path.string(), outputRoot.string());
	}
}