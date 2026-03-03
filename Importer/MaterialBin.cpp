#include "pch.h"
#include "MaterialBin.h"
#include "BinHelper.h"

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

static std::string BaseNameFromPath(const std::string& path)
{
	const size_t pos = path.find_last_of("/\\");
	if (pos == std::string::npos) return path;
	return path.substr(pos + 1);
}

static bool TryParseEmbeddedIndex(const std::string& rawPath, uint32_t& outIndex)
{
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

static bool TryResolveEmbeddedIndex(const aiScene* scene, const std::string& rawPath, uint32_t& outIndex)
{
	if (TryParseEmbeddedIndex(rawPath, outIndex)) return true;
	if (!scene || scene->mNumTextures == 0) return false;
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

static bool EmbeddedTextureHasType(
	const std::unordered_map<uint32_t, std::vector<aiTextureType>>& usage,
	uint32_t index,
	aiTextureType type)
{
	const auto it = usage.find(index);
	if (it == usage.end()) return false;
	const auto& types = it->second;
	return std::find(types.begin(), types.end(), type) != types.end();
}

static std::string ResolveExternalTexturePath(
	const std::unordered_map<std::string, std::string>& externalTextureRemap,
	const std::string& rawPath)
{
	//항상 파일명만 사용하도록 바꿈
	if (rawPath.empty()) return {};

	auto it = externalTextureRemap.find(rawPath);
	if (it != externalTextureRemap.end()) return it->second;

	const std::string baseName = BaseNameFromPath(rawPath);
	if (!baseName.empty())
	{
		it = externalTextureRemap.find(baseName);
		if (it != externalTextureRemap.end()) return it->second;
		return baseName;  
	}

	return {};
}


static std::string GetTexPath(
	const aiScene* scene,
	const aiMaterial* mat,
	aiTextureType type,
	const std::unordered_map<uint32_t, std::vector<aiTextureType>>& embeddedUsage,
	const std::unordered_map<std::string, std::string>& externalTextureRemap)
{
	if (!mat) return {};
	if (mat->GetTextureCount(type) <= 0) return {};

	aiString path;
	if (mat->GetTexture(type, 0, &path) != AI_SUCCESS) return {};

	const std::string rawPath = path.C_Str();
	uint32_t index = 0;
	if (TryResolveEmbeddedIndex(scene, rawPath, index))
	{
		const aiTexture* texture = (scene && index < scene->mNumTextures) ? scene->mTextures[index] : nullptr;
		const std::string extension = EmbeddedTextureExtension(texture);
		if (!EmbeddedTextureHasType(embeddedUsage, index, type))
		{
			return "embedded_" + std::to_string(index) + "_texture." + extension;
		}

		std::string suffix = TextureTypeSuffix(type);
		if (!suffix.empty())
		{
			suffix[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(suffix[0])));
		}
		return "embedded_" + std::to_string(index) + "_" + suffix + "." + extension;
	}

	return ResolveExternalTexturePath(externalTextureRemap, rawPath);
}

static std::string ResolveFallbackTexture(
	ETextureType slot,
	const std::unordered_map<ETextureType, std::string>& fallbackTextureMap)
{
	const auto it = fallbackTextureMap.find(slot);
	if (it == fallbackTextureMap.end())
	{
		return {};
	}

	return it->second;
}

bool ImportFBXToMaterialBin(
	const aiScene* scene,
	const std::string& outMaterialBin,
	const std::unordered_map<uint32_t, std::vector<aiTextureType>>& embeddedUsage,
	const std::unordered_map<std::string, std::string>& externalTextureRemap,
	const std::unordered_map<ETextureType, std::string>& fallbackTextureMap)
{
	if (!scene || !scene->HasMaterials()) return false;

	std::vector<MatData> materials;
	materials.reserve(scene->mNumMaterials);

	std::string stringTable;
	stringTable.reserve(4096);
	stringTable.push_back('\0');

	for (uint32_t m = 0; m < scene->mNumMaterials; ++m)
	{
		const aiMaterial* mat = scene->mMaterials[m];
		if (!mat) continue;

		MatData out{};

		aiString name;
		if (mat->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
			out.materialNameOffset = AddString(stringTable, name.C_Str());

		// baseColor (PBR 우선, 없으면 diffuse fallback)
		aiColor4D baseColor(1, 1, 1, 1);
		if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) != AI_SUCCESS)
		{
			mat->Get(AI_MATKEY_COLOR_DIFFUSE, baseColor);
		}

		out.baseColor[0] = baseColor.r;
		out.baseColor[1] = baseColor.g;
		out.baseColor[2] = baseColor.b;
		out.baseColor[3] = baseColor.a;

		// metallic
		float metallic = 0.0f;
		if (mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS)
		{
			out.metallic = metallic;
		}

		// roughness
		float roughness = 0.0f;
		if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS)
		{
			out.roughness = roughness;
		}
		else
		{
			// glossiness만 있으면 roughness로 변환(있을 때만)
			float gloss = 0.0f;
			if (mat->Get(AI_MATKEY_GLOSSINESS_FACTOR, gloss) == AI_SUCCESS)
				out.roughness = 1.0f - gloss;
		}

		float opacity = 1.0f;
		if (mat->Get(AI_MATKEY_OPACITY, opacity) == AI_SUCCESS)
			out.opacity = opacity;
		
		int twoSided = 0;
		if (mat->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
			out.doubleSided = (twoSided != 0) ? 1 : 0;

		// alphaMode / alphaCutoff
		// FBX는 표준 alphaMode 키가 안정적이지 않아서 "프로젝트 규칙"으로 결정하는 게 보통임.
		// 여기서는 "opacity<1이면 Blend" 정도만 예시로 둠.
		{
			out.alphaMode = 0; // Opaque
			out.alphaCutOff = 0.5f;

			if (out.opacity < 1.0f)
				out.alphaMode = 2; // Blend

			// glTF 경로로 들어온 경우에만 들어올 수 있는 키(있으면)
#ifdef AI_MATKEY_GLTF_ALPHACUTOFF
			float cutoff = 0.5f;
			if (mat->Get(AI_MATKEY_GLTF_ALPHACUTOFF, cutoff) == AI_SUCCESS)
				out.alphaCutOff = cutoff;
#endif
		}


		std::string albedo = GetTexPath(scene, mat, aiTextureType_BASE_COLOR, embeddedUsage, externalTextureRemap);
		if (albedo.empty()) albedo = GetTexPath(scene, mat, aiTextureType_DIFFUSE, embeddedUsage, externalTextureRemap);
		if (albedo.empty()) albedo = ResolveFallbackTexture(ETextureType::ALBEDO, fallbackTextureMap);
		out.texPathOffset[(size_t)ETextureType::ALBEDO] = AddString(stringTable, albedo);

		std::string normal = GetTexPath(scene, mat, aiTextureType_NORMALS, embeddedUsage, externalTextureRemap);
		if (normal.empty()) normal = GetTexPath(scene, mat, aiTextureType_HEIGHT, embeddedUsage, externalTextureRemap);
		if (normal.empty()) normal = ResolveFallbackTexture(ETextureType::NORMAL, fallbackTextureMap);
		out.texPathOffset[(size_t)ETextureType::NORMAL] = AddString(stringTable, normal);

		// Metallic / Roughness / AO / Emissive
		{
			std::string metallic = GetTexPath(scene, mat, aiTextureType_METALNESS, embeddedUsage, externalTextureRemap);
			if (metallic.empty()) metallic = ResolveFallbackTexture(ETextureType::METALLIC, fallbackTextureMap);
			out.texPathOffset[(size_t)ETextureType::METALLIC] = AddString(stringTable, metallic);

			std::string roughness = GetTexPath(scene, mat, aiTextureType_DIFFUSE_ROUGHNESS, embeddedUsage, externalTextureRemap);
			if (roughness.empty()) roughness = ResolveFallbackTexture(ETextureType::ROUGHNESS, fallbackTextureMap);
			out.texPathOffset[(size_t)ETextureType::ROUGHNESS] = AddString(stringTable, roughness);

			std::string ao = GetTexPath(scene, mat, aiTextureType_AMBIENT_OCCLUSION, embeddedUsage, externalTextureRemap);
			if (ao.empty()) ao = ResolveFallbackTexture(ETextureType::AO, fallbackTextureMap);
			out.texPathOffset[(size_t)ETextureType::AO] = AddString(stringTable, ao);

			std::string emissive = GetTexPath(scene, mat, aiTextureType_EMISSIVE, embeddedUsage, externalTextureRemap);
			if (emissive.empty()) emissive = ResolveFallbackTexture(ETextureType::EMISSIVE, fallbackTextureMap);
			out.texPathOffset[(size_t)ETextureType::EMISSIVE] = AddString(stringTable, emissive);
		}

		materials.push_back(out);
	}

	MatBinHeader header{};
	header.materialCount    = static_cast<uint16_t>(materials.size());
	header.stringTableBytes = static_cast<uint32_t>(stringTable.size());

	std::ofstream ofs(outMaterialBin, std::ios::binary);
	if (!ofs) return false;

	ofs.write(reinterpret_cast<const char*>(&header), sizeof(header));

	if (!materials.empty())
	{
		ofs.write(reinterpret_cast<const char*>(materials.data()), sizeof(MatData) * materials.size());
	}
	
	if (!stringTable.empty())
		ofs.write((const char*)stringTable.data(), stringTable.size());

	return true;
}