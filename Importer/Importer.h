#pragma once
#include "MeshBin.h"
#include "MaterialBin.h"
#include "AnimJson.h"
#include "SkeletonBin.h"
#include "MetaJson.h"

void ImportAll();

void ImportFBX(const std::string& FBXPath, const std::string& outDir);