#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>

#include "RenderData.h"
#include "ResourceStore.h"
#include "ResourceRefs.h"

#include "json.hpp"
using nlohmann::json;
namespace fs = std::filesystem;

class AssetLoader
{
public:
	AssetLoader();
	~AssetLoader();

	static void SetActive(AssetLoader* loader);
	static AssetLoader* GetActive();

	struct AssetLoadResult
	{
		std::vector<MeshHandle>		    meshes;
		std::vector<MaterialHandle>     materials;
		std::vector<TextureHandle>	    textures;
		SkeletonHandle				    skeleton = SkeletonHandle::Invalid();
		std::vector<AnimationHandle>    animations;
	};

	struct ShaderLoadResult
	{
		std::vector<VertexShaderHandle> vertexShaders;
		std::vector<PixelShaderHandle>  pixelShaders;
	};

	void LoadAll();
	const std::unordered_map<std::wstring, std::filesystem::path>& GetBGMPaths() const { return m_BGMPaths; }
	const std::unordered_map<std::wstring, std::filesystem::path>& GetSFXPaths() const { return m_SFXPaths; }
	

	const AssetLoadResult* GetAsset		  (const std::string& assetMetaPath) const;
	MaterialHandle     ResolveMaterial    (const std::string& assetMetaPath, UINT32 index) const;
	MeshHandle		   ResolveMesh	      (const std::string& assetMetaPath, UINT32 index) const;
	TextureHandle      ResolveTexture     (const std::string& assetMetaPath, UINT32 index) const;
	SkeletonHandle     ResolveSkeleton    (const std::string& assetMetaPath, UINT32 index) const;
	AnimationHandle    ResolveAnimation   (const std::string& assetMetaPath, UINT32 index) const;

	ShaderAssetHandle  ResolveShaderAsset (const std::string& shaderMetaPath, UINT32 index);
	VertexShaderHandle ResolveVertexShader(const VertexShaderRef& ref);
	PixelShaderHandle  ResolvePixelShader (const PixelShaderRef& ref);

	bool GetMaterialAssetReference    (MaterialHandle handle,     std::string& outPath, UINT32& outIndex) const;
	bool GetMeshAssetReference        (MeshHandle handle,         std::string& outPath, UINT32& outIndex) const;
	bool GetTextureAssetReference     (TextureHandle handle,      std::string& outPath, UINT32& outIndex) const;
	bool GetShaderAssetReference	  (ShaderAssetHandle handle, std::string& outPath, UINT32& outIndex) const;
	bool GetVertexShaderAssetReference(VertexShaderHandle handle, std::string& outPath, UINT32& outIndex) const;
	bool GetPixelShaderAssetReference (PixelShaderHandle handle,  std::string& outPath, UINT32& outIndex) const;
	bool GetSkeletonAssetReference    (SkeletonHandle handle,     std::string& outPath, UINT32& outIndex) const;
	bool GetAnimationAssetReference   (AnimationHandle handle,    std::string& outPath, UINT32& outIndex) const;

	const ResourceStore<RenderData::MeshData, MeshHandle>& GetMeshes() const { return m_Meshes; }
	ResourceStore<RenderData::MeshData, MeshHandle>& GetMeshes() { return m_Meshes; }
	const ResourceStore<RenderData::MaterialData, MaterialHandle>& GetMaterials() const { return m_Materials; }
	ResourceStore<RenderData::MaterialData, MaterialHandle>& GetMaterials() { return m_Materials; }
	ResourceStore<RenderData::TextureData, TextureHandle>& GetTextures() { return m_Textures; }
	ResourceStore<RenderData::ShaderAssetData, ShaderAssetHandle>& GetShaderAssets() { return m_ShaderAssets; }
	ResourceStore<RenderData::VertexShaderData, VertexShaderHandle>& GetVertexShaders() { return m_VertexShaders; }
	ResourceStore<RenderData::PixelShaderData, PixelShaderHandle>& GetPixelShaders() { return m_PixelShaders; }
	ResourceStore<RenderData::Skeleton, SkeletonHandle>& GetSkeletons() { return m_Skeletons; }
	ResourceStore<RenderData::AnimationClip, AnimationHandle>& GetAnimations() { return m_Animations; }

	ShaderAssetHandle LoadShaderAsset(const std::string& shaderMetaPath);
	ShaderAssetHandle EnsureShaderAssetForStages(VertexShaderHandle vertexShader, PixelShaderHandle pixelShader);

	void LoadMeshes(json& meta,
		const fs::path& baseDir,
		AssetLoadResult& result,
		std::vector<MaterialHandle>& materialHandles,
		std::unordered_map<std::string, MaterialHandle>& materialByName,
		const std::string& assetMetaPath);

	void LoadMaterials(json& meta,
		const fs::path& baseDir,
		const fs::path& textureDir,
		AssetLoadResult& result,
		std::vector<MaterialHandle>& materialHandles,
		std::unordered_map<std::string, MaterialHandle>& materialByName,
		const std::string& assetMetaPath);

	void LoadSkeletons(json& meta, 
		const fs::path& baseDir, 
		AssetLoadResult& result,
		const std::string& assetMetaPath);

	void LoadAnimations(json& meta, 
		const fs::path& baseDir,
		AssetLoadResult& result, 
		const std::string& assetMetaPath);
private:
	AssetLoadResult	  LoadAsset		 (const std::string& assetMetaPath);
	void LoadShaderSources(const fs::path& shaderDir);
	void LoadLooseTextures(const fs::path& rootDir, bool sRGB, const std::string& displayPrefix);
	void LoadSoundResources(const fs::path& bgmDir, const fs::path& sfxDir);
	
	ResourceStore<RenderData::MeshData,			MeshHandle>         m_Meshes;
	ResourceStore<RenderData::MaterialData,		MaterialHandle>     m_Materials;
	ResourceStore<RenderData::TextureData,		TextureHandle>      m_Textures;
	ResourceStore<RenderData::ShaderAssetData,  ShaderAssetHandle>  m_ShaderAssets;
	ResourceStore<RenderData::VertexShaderData, VertexShaderHandle> m_VertexShaders;
	ResourceStore<RenderData::PixelShaderData,  PixelShaderHandle>	m_PixelShaders;
	ResourceStore<RenderData::Skeleton,			SkeletonHandle>     m_Skeletons;
	ResourceStore<RenderData::AnimationClip,	AnimationHandle>    m_Animations;


	std::unordered_map<std::string, AssetLoadResult> m_AssetsByPath;
	std::unordered_map<uint64_t,    MeshRef>		 m_MeshRefs;
	std::unordered_map<uint64_t,    MaterialRef>	 m_MaterialRefs;
	std::unordered_map<uint64_t,    TextureRef>		 m_TextureRefs;
	std::unordered_map<uint64_t,    ShaderAssetRef>  m_ShaderAssetRefs;
	std::unordered_map<uint64_t,    VertexShaderRef> m_VertexShaderRefs;
	std::unordered_map<uint64_t,    PixelShaderRef>  m_PixelShaderRefs;
	std::unordered_map<uint64_t,    SkeletonRef>	 m_SkeletonRefs;
	std::unordered_map<uint64_t,    AnimationRef>	 m_AnimationRefs;
	std::unordered_map<std::wstring, std::filesystem::path> m_BGMPaths;
	std::unordered_map<std::wstring, std::filesystem::path> m_SFXPaths;

	inline static AssetLoader* s_ActiveLoader = nullptr;
};

