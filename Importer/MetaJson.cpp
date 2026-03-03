#include "pch.h"
#include "MetaJson.h"

bool WriteAssetMetaJson(
    const std::string& outPath,
    const std::string& assetName, 
    const std::string& baseDir, 
    const std::string& textureDir, 
    const std::string& matBin,
    const std::string& skelBinOrEmpty,
    const std::vector<MetaMeshItem>& meshes, 
    const std::vector<MetaAnimItem>& anims
)
{
    return WriteAssetMetaJson(outPath, assetName, baseDir, textureDir, matBin, skelBinOrEmpty, meshes, anims, json::object());
}

bool WriteAssetMetaJson(
	const std::string& outPath, 
	const std::string& assetName, 
	const std::string& baseDir, 
	const std::string& textureDir, 
	const std::string& matBin, 
	const std::string& skelBinOrEmpty, 
	const std::vector<MetaMeshItem>& meshes, 
	const std::vector<MetaAnimItem>& anims, 
	const json& skeletonMeta)
{
	json j;

	j["name"] = assetName;

	j["path"] = {
		{"baseDir",baseDir},
		{"textureDir", textureDir }
	};

	j["files"] =
	{
		{"materials", matBin},
		{"skeleton", skelBinOrEmpty}
	};

	j["meshes"] = json::array();
	for (auto& mesh : meshes)
	{
		j["meshes"].push_back({ { "file", mesh.file }, { "isSkinned", mesh.isSkinned } });
	}

	j["animations"] = json::array();
	for (auto& anim : anims)
	{
		j["animations"].push_back({ {"name", anim.name}, {"file", anim.file} });
	}

	if (!skeletonMeta.is_null() && !skeletonMeta.empty())
	{
		j["skeletonMeta"] = skeletonMeta;
	}

	std::ofstream ofs(outPath);
	if (!ofs) return false;

	ofs << j.dump(2);
	return true;
}
