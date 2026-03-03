#pragma once
#include "json.hpp"
#include <fstream>
#include <string>
#include <vector>
using nlohmann::json;

struct MetaMeshItem { std::string file; bool isSkinned; };
struct MetaAnimItem { std::string name; std::string file; };

bool WriteAssetMetaJson(
    const std::string& outPath,
    const std::string& assetName,
    const std::string& baseDir,
    const std::string& textureDir,
    const std::string& matBin,
    const std::string& skelBinOrEmpty,
    const std::vector<MetaMeshItem>& meshes,
    const std::vector<MetaAnimItem>& anims
);

bool WriteAssetMetaJson(
	const std::string& outPath,
	const std::string& assetName,
	const std::string& baseDir,
	const std::string& textureDir,
	const std::string& matBin,
	const std::string& skelBinOrEmpty,
	const std::vector<MetaMeshItem>& meshes,
	const std::vector<MetaAnimItem>& anims,
	const json& skeletonMeta
);

