#pragma once
#include <string>
#include <windows.h>

struct AssetRef
{
	std::string assetPath;
	UINT32      assetIndex = 0;
};

using MeshRef         = AssetRef;
using MaterialRef     = AssetRef;
using TextureRef	  = AssetRef;
using ShaderAssetRef  = AssetRef;
using VertexShaderRef = AssetRef;
using PixelShaderRef  = AssetRef;
using SkeletonRef     = AssetRef;
using AnimationRef    = AssetRef;