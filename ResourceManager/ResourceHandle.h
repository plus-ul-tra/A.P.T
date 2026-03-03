#pragma once

#include <cstdint>
#include <Windows.h>

// id + generation만 들고 있는 가벼운 참조값

struct ResourceHandle
{
	UINT32 id = 0;
	UINT32 generation = 0;

	bool IsValid() const
	{
		return id != 0;
	}

	static ResourceHandle Invalid()
	{
		return {};
	}

	bool operator==(const ResourceHandle& other) const
	{
		return id == other.id && generation == other.generation;
	}

	bool operator!=(const ResourceHandle& other) const
	{
		return !(*this == other);
	}
};

template <typename Tag>
struct TypedHandle
{
	UINT32 id = 0;
	UINT32 generation = 0;

	bool IsValid() const
	{
		return id != 0;
	}

	static TypedHandle Invalid()
	{
		return {};
	}

	bool operator==(const TypedHandle& other) const
	{
		return id == other.id && generation == other.generation;
	}

	bool operator!=(const TypedHandle& other) const
	{
		return !(*this == other);
	}
};

struct MeshTag
{
};

struct MaterialTag
{
};

struct TextureTag
{
};

struct ShaderAssetTag
{
};

struct VertexShaderTag
{
};

struct PixelShaderTag
{
};

struct SkeletonTag
{
};

struct AnimationTag
{
};

using MeshHandle		 = TypedHandle<MeshTag>;
using MaterialHandle	 = TypedHandle<MaterialTag>;
using TextureHandle		 = TypedHandle<TextureTag>;
using ShaderAssetHandle  = TypedHandle<ShaderAssetTag>;
using VertexShaderHandle = TypedHandle<VertexShaderTag>;
using PixelShaderHandle  = TypedHandle<PixelShaderTag>;
using SkeletonHandle     = TypedHandle<SkeletonTag>;
using AnimationHandle    = TypedHandle<AnimationTag>;


//핸들을 unordered_map의키로 사용할 수 있게 해주는 해시 함수
namespace std
{
	template <>
	struct hash<ResourceHandle>
	{
		size_t operator()(const ResourceHandle& handle) const noexcept
		{
			return (static_cast<size_t>(handle.id) << 32) ^ handle.generation;
		}
	};

	template <typename Tag>
	struct hash<TypedHandle<Tag>>
	{
		size_t operator()(const TypedHandle<Tag>& handle) const noexcept
		{
			return (static_cast<size_t>(handle.id) << 32) ^ handle.generation;
		}
	};
}