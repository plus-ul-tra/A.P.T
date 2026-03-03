#pragma once
#include "Component.h"
#include "MathHelper.h"
#include "CameraSettings.h"

bool SceneHasObjectName(const Scene& scene, const std::string& name);
std::string MakeUniqueObjectName(const Scene& scene, const std::string& baseName);
void CopyStringToBuffer(const std::string& value, std::array<char, 256>& buffer);
bool DrawSubMeshOverridesEditor(MeshComponent& meshComponent, AssetLoader& assetLoader);

struct PropertyEditResult
{
	bool updated     = false;
	bool activated   = false;
	bool deactivated = false;
};

PropertyEditResult DrawComponentPropertyEditor(Component* component, const Property& property, AssetLoader& assetLoader);
bool ProjectToViewport( const XMFLOAT3& world, const XMMATRIX& viewProj, const ImVec2& rectMin,const ImVec2& rectMax, ImVec2& out);
