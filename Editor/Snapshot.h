#pragma once
#include "json.hpp"
#include <filesystem>
#include <string>
#include <array>
#include "GameObject.h"
#include <memory>
#include "Scene.h"
#include <fstream>

namespace fs = std::filesystem;

struct ObjectSnapshot
{
	nlohmann::json data;
	bool isOpaque;
};

struct SceneStateSnapshot
{
	bool hasScene = false;
	nlohmann::json data;
	fs::path currentPath;
	fs::path selectedPath;
	std::string selectedObjectName;
	std::string lastSelectedObjectName;
	std::array<char, 256> objectNameBuffer;
	std::string lastSceneName;
	std::array<char, 256> sceneNameBuffer;
};

struct SceneFileSnapshot
{
	fs::path path;
	bool existed = false;
	std::string contents;
};

inline std::shared_ptr<GameObject> FindSceneObject(Scene* scene, const std::string& name)
{
	if (!scene)
	{
		return nullptr;
	}

	const auto& gameObjects = scene->GetGameObjects();
	if (auto it = gameObjects.find(name); it != gameObjects.end())
	{
		return it->second;
	}

	return nullptr;
}

inline std::shared_ptr<GameObject> EnsureSceneObjects(Scene* scene, const ObjectSnapshot& snapshot)
{
	if (!scene)
		return nullptr;

	const std::string name = snapshot.data.value("name", "");
	if (name.empty())
	{
		return nullptr;
	}

	if (auto existing = FindSceneObject(scene, name))
	{
		return existing;
	}

	auto created = std::make_shared<GameObject>(scene->GetEventDispatcher());
	created->Deserialize(snapshot.data);
	scene->AddGameObject(created);
	return created;
}

inline void ApplySnapshot(Scene* scene, const ObjectSnapshot& snapshot)
{
	auto target = EnsureSceneObjects(scene, snapshot);
	if (!target)
		return;

	target->Deserialize(snapshot.data);
}

inline size_t MakePropertyKey(const Component* component, const std::string& propertyName)
{
	const size_t pointerHash = std::hash<const void*>{}(component);
	const size_t nameHash = std::hash<std::string>{}(propertyName);
	return pointerHash ^ (nameHash + 0x9e3779b97f4a7c15ULL + (pointerHash << 6) + (pointerHash >> 2));
}

inline SceneFileSnapshot CaptureFileSnapshot(const fs::path& path)
{
	SceneFileSnapshot snapshot;
	snapshot.path = path;
	if (path.empty())
		return snapshot;

	std::ifstream ifs(path);
	if (!ifs.is_open())
	{
		return snapshot;
	}

	snapshot.existed = true;
	snapshot.contents.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
	return snapshot;
}

inline void RestoreFileSnapshot(const SceneFileSnapshot& snapshot)
{
	if(snapshot.path.empty())
	{
		return;
	}

	if (!snapshot.existed)
	{
		std::error_code error;
		fs::remove(snapshot.path, error);
		return;
	}

	std::ofstream ofs(snapshot.path);
	if (!ofs.is_open())
	{
		return;
	}
	ofs << snapshot.contents;
}