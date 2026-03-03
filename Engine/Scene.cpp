#include "pch.h"
#include "Scene.h"
#include "json.hpp"
#include "Object.h"
#include "GameObject.h"
#include "UIObject.h"
#include "GameManager.h"
#include <unordered_set>
#include "ServiceRegistry.h"
#include "EventDispatcher.h"
#include "AssetLoader.h"
#include "CameraComponent.h"
#include "MeshRenderer.h"
#include "MaterialComponent.h"
#include "LightComponent.h"
#include "DirectionalLightComponent.h"
#include "PointLightComponent.h"
#include "SpotLightComponent.h"
#include "TransformComponent.h"
#include "SkeletalMeshComponent.h"
#include "AnimationComponent.h"
#include "SkeletalMeshRenderer.h"
#include "CameraObject.h"
#include <fstream>
#include "EnemyComponent.h"
#include "EnemyMovementComponent.h"
#include "EnemyStatComponent.h"
#include "EnemyControllerComponent.h"
#include "MeshComponent.h"
#include "PlayerCombatFSMComponent.h"
#include "PlayerComponent.h"
#include "PlayerDoorFSMComponent.h"
#include "PlayerInventoryFSMComponent.h"
#include "PlayerMoveFSMComponent.h"
#include "PlayerMovementComponent.h"
#include "PlayerPushFSMComponent.h"
#include "PlayerShopFSMComponent.h"
#include "PlayerFSMComponent.h"
#include "PlayerStatComponent.h"
#include "NodeComponent.h"
#include "SkinningAnimationComponent.h"
#include "AnimFSMComponent.h"
#include <type_traits>

#ifdef _DEBUG
namespace
{
	void AppendSkinningPaletteFrameDebug(
		const Object* owner,
		const RenderData::Skeleton* skeleton,
		size_t paletteSize,
		const char* reason)
	{
		std::ofstream ofs("skinning_palette_frame.debug.txt", std::ios::app);
		if (!ofs)
			return;

		const std::string objectName = owner ? owner->GetName() : std::string("unknown");
		const size_t boneCount = skeleton ? skeleton->bones.size() : 0u;

		ofs << "object=" << objectName
			<< " reason=" << (reason ? reason : "unknown")
			<< " bones=" << boneCount
			<< " paletteSize=" << paletteSize
			<< "\n";
	}
}
#endif
Scene::Scene(ServiceRegistry& serviceRegistry) : m_Services(serviceRegistry) 
{ 
	m_AssetLoader = &m_Services.Get<AssetLoader>(); 
}


Scene::~Scene()
{
	m_GameObjects.clear();
	//m_Camera = nullptr; // 필요 시
}

// Light, Camera, Fog 등등 GameObject 외의 Scene 구성된 것들 Update
void Scene::StateUpdate(float deltaTime)
{
	// Camera는 BuildFromData를 하면서 자동으로 갱신이 되고있음
	// Light의 경우 LightObject가 생기고 PointLight 같은 애의 위치가 바뀌면 만들수있을것같음?

	if (!m_Pause)
	{
		return;
	}

	if (m_GameCamera)
	{
		m_GameCamera->Update(deltaTime);
	}
}

void Scene::Enter()
{
	for (auto& [name, obj] : m_GameObjects)
	{
		obj->Start();
	}
}

void Scene::Render(RenderData::FrameData& frameData) const
{
	BuildFrameData(frameData);
}

void Scene::AddGameObject(std::shared_ptr<GameObject> gameObject)
{
	if (!gameObject) {
		return;
	}
	gameObject->SetScene(this);

	if (gameObject->m_Name == "Main Camera")
	{
		//SetMainCamera(gameObject);
	}

	m_GameObjects[gameObject->m_Name] = std::move(gameObject);
}

void Scene::QueueGameObjectRemoval(const std::string& name)
{
	if (name.empty())
	{
		return;
	}

	m_PendingRemovalNames.push_back(name);
}

void Scene::ProcessPendingRemovals()
{
	if (m_PendingRemovalNames.empty())
	{
		return;
	}

	std::vector<std::string> pending;
	pending.swap(m_PendingRemovalNames);
	for (const auto& name : pending)
	{
		RemoveGameObjectByName(name);
	}
}

void Scene::RemoveGameObject(std::shared_ptr<GameObject> gameObject)
{
	if (!gameObject) return;


	if (auto* trans = gameObject->GetComponent<TransformComponent>())
	{
		const auto children = trans->GetChildrens();
		for (auto* childTransform : children)
		{
			if (!childTransform)
			{
				continue;
			}
			auto* childOwner = dynamic_cast<GameObject*>(childTransform->GetOwner());
			if (!childOwner)
			{
				continue;
			}
			RemoveGameObjectByName(childOwner->GetName());
		}
		trans->DetachFromParent();
	}

	auto it = m_GameObjects.find(gameObject->m_Name);
	if (it != m_GameObjects.end())
	{
		if (auto* trans = gameObject->GetComponent<TransformComponent>())
		{
			trans->DetachFromParent();
		}
		gameObject->SetScene(nullptr);
		m_GameObjects.erase(gameObject->m_Name);
	}
}

std::shared_ptr<GameObject> Scene::CreateGameObject(const std::string& name)
{
	auto gameObject = std::make_shared<GameObject>(GetEventDispatcher());
	gameObject->SetName(name);

	AddGameObject(gameObject);
	return gameObject;
}

bool Scene::RemoveGameObjectByName(const std::string& name)
{
	auto it = m_GameObjects.find(name);
	if (it != m_GameObjects.end())
	{
		RemoveGameObject(it->second);
		return true;
	}

	return false;
}

bool Scene::RenameGameObject(const std::string& currentName, const std::string& newName)
{
	if (currentName == newName || newName.empty())
	{
		return false;
	}

	if (HasGameObjectName(newName))
	{
		return false;
	}

	auto node = m_GameObjects.extract(currentName);
	if (!node.empty())
	{
		node.key() = newName;
		node.mapped()->SetName(newName);
		m_GameObjects.insert(std::move(node));
		return true;
	}

	return false;
}

bool Scene::HasGameObjectName(const std::string& name) const
{
	return m_GameObjects.find(name) != m_GameObjects.end();
}




void Scene::SetGameCamera(std::shared_ptr<CameraObject> cameraObject)
{
	m_GameCamera = cameraObject;
}

void Scene::SetEditorCamera(std::shared_ptr<CameraObject> cameraObject)
{
	m_EditorCamera = cameraObject;
}


// Opaque, Transparent, UI  ex) ["gameObjects"]["opaque"]
void Scene::Serialize(nlohmann::json& j) const
{	
	//
	// Scene의 Editor 설정 값들
	// Editor 카메라 설정값기억(editor에서만 사용)
	j["editor"] = nlohmann::json::object();
	if (m_EditorCamera)
	{
		auto writeVec3 = [](nlohmann::json& target, const XMFLOAT3& value)
			{
				target["x"] = value.x;
				target["y"] = value.y;
				target["z"] = value.z;
			};
		nlohmann::json editorCameraJson;
		writeVec3(editorCameraJson["eye"], m_EditorCamera->GetEye());
		writeVec3(editorCameraJson["look"], m_EditorCamera->GetLook());
		writeVec3(editorCameraJson["up"], m_EditorCamera->GetUp());

		j["editor"]["EditorCamera"] = editorCameraJson;
	}
	//
	//	GameObject 영역
	//
	j["gameObjects"] = nlohmann::json::array();

	// map -> vector 복사
	std::vector<std::pair<std::string, std::shared_ptr<GameObject>>> gameObjects(
		m_GameObjects.begin(), m_GameObjects.end());

	//UI는 나중에

	auto sortPred = [](const auto& a, const auto& b) {
		auto extractNameAndNumber = [](const std::string& s) -> std::pair<std::string, int> {
			size_t pos = s.find_first_of("0123456789");
			if (pos != std::string::npos) {
				return { s.substr(0, pos), std::stoi(s.substr(pos)) };
			}
			return { s, -1 };
			};

		auto [nameA, numA] = extractNameAndNumber(a.first);
		auto [nameB, numB] = extractNameAndNumber(b.first);

		if (nameA == nameB) {
			// 같은 종류일 경우 숫자 비교
			return numA < numB;
		}
		// 이름(문자) 기준 비교
		return nameA < nameB;
		};

	// 정렬 (숫자 포함 이름 기준)
	std::sort(gameObjects.begin(), gameObjects.end(), sortPred);

	// 정렬된 순서대로 JSON에 저장
	for (const auto& gameObject : gameObjects)
	{
		nlohmann::json gameObjectJson;
		gameObject.second->Serialize(gameObjectJson);
		j["gameObjects"].push_back(gameObjectJson);
	}

	//UI
}

template<typename ObjectContainer>
void ProcessWithErase(
	const nlohmann::json& arr,
	ObjectContainer& objContainer,
	EventDispatcher& dispatcher,
	Scene* ownerScene)
{
	std::unordered_set<std::string> names;
	for (const auto& gameObjectJson : arr)
	{
		if (!gameObjectJson.is_object() || !gameObjectJson.contains("name") || !gameObjectJson.at("name").is_string())
		{
			continue;
		}
		names.insert(gameObjectJson.at("name").get<std::string>());
	}

	for (const auto& gameObjectJson : arr)
	{
		if (!gameObjectJson.is_object() || !gameObjectJson.contains("name") || !gameObjectJson.at("name").is_string())
		{
			continue;
		}
		std::string name = gameObjectJson.at("name").get<std::string>();

		auto it = objContainer.find(name);
		if (it != objContainer.end())
		{	
			it->second->SetScene(ownerScene);
			it->second->Deserialize(gameObjectJson);
		}
		else
		{
			std::shared_ptr<GameObject> gameObject;
			// 없으면 새로 생성 후 추가
			gameObject = std::make_shared<GameObject>(dispatcher);
			gameObject->SetScene(ownerScene);
			gameObject->Deserialize(gameObjectJson);
			objContainer[name] = std::move(gameObject);
		}
	}

	for (auto it = objContainer.begin(); it != objContainer.end(); )
	{
		if (!names.contains(it->first)) {
			it->second->SetScene(nullptr);
			it = objContainer.erase(it);
		}
		else ++it;
	}
}

void Scene::Deserialize(const nlohmann::json& j)
{
	const auto& goRoot = j.at("gameObjects"); //GameObject Root
	nlohmann::json mergedGameObjects = nlohmann::json::array();
	auto appendGameObjectArray = [&](const nlohmann::json& arr)
		{
			for (const auto& gameObjectJson : arr)
			{
				mergedGameObjects.push_back(gameObjectJson);
			}
		};
	if (goRoot.is_array())
	{
		appendGameObjectArray(goRoot);
	}
	else if (goRoot.is_object())
	{
		for (const auto& [key, value] : goRoot.items())
		{
			if (value.is_array())
			{
				appendGameObjectArray(value);
			}
		}
	}

	const nlohmann::json* editorRoot = nullptr;


	//editor 카메라 셋팅값 저장( 게임에서는 안씀)
	if (j.contains("editor"))
	{
		editorRoot = &j.at("editor");
	}
	if (editorRoot && editorRoot->contains("EditorCamera"))
	{
		if (m_EditorCamera)
		{
			if (auto* cameraComp = m_EditorCamera->GetComponent<CameraComponent>())
			{
				const auto& editorCameraJson = editorRoot->at("EditorCamera");
				auto readVec3 = [](const nlohmann::json& source, const XMFLOAT3& fallback)
					{
						XMFLOAT3 result = fallback;
						result.x = source.value("x", result.x);
						result.y = source.value("y", result.y);
						result.z = source.value("z", result.z);
						return result;
					};

				XMFLOAT3 eye = m_EditorCamera->GetEye();
				XMFLOAT3 look = m_EditorCamera->GetLook();
				XMFLOAT3 up = m_EditorCamera->GetUp();
				if (editorCameraJson.contains("eye"))
				{
					eye = readVec3(editorCameraJson.at("eye"), eye);
				}
				if (editorCameraJson.contains("look"))
				{
					look = readVec3(editorCameraJson.at("look"), look);
				}
				if (editorCameraJson.contains("up"))
				{
					up = readVec3(editorCameraJson.at("up"), up);
				}
				cameraComp->SetEyeLookUp(eye, look, up);
			}
		}
	}

	// 오브젝트 자동 역직렬화
	ProcessWithErase(mergedGameObjects, m_GameObjects, GetEventDispatcher(), this);

	std::unordered_map<std::string, std::shared_ptr<GameObject>> objectLookup;
	for (const auto& [name, object] : m_GameObjects)
	{
		objectLookup[name] = object;
	}

	auto applyParentLinks = [&](const nlohmann::json& arr)
		{
			for (const auto& gameObjectJson : arr)
			{
				if (!gameObjectJson.is_object() || !gameObjectJson.contains("name") || !gameObjectJson.at("name").is_string())
				{
					continue;
				}
				const std::string name = gameObjectJson.at("name").get<std::string>();
				auto childIt = objectLookup.find(name);
				if (childIt == objectLookup.end())
				{
					continue;
				}
				auto* childTransform = childIt->second->GetComponent<TransformComponent>();
				if (!childTransform)
				{
					continue;
				}

				const std::string parentName = gameObjectJson.value("parent", "");
				if (parentName.empty())
				{
					if (childTransform->GetParent())
					{
						//childTransform->DetachFromParent();
						childTransform->DetachFromParentKeepLocal();
					}
					continue;
				}

				auto parentIt = objectLookup.find(parentName);
				if (parentIt == objectLookup.end())
				{
					if (childTransform->GetParent())
					{
						//childTransform->DetachFromParent();
						childTransform->DetachFromParentKeepLocal();
					}
					continue;
				}

				auto* parentTransform = parentIt->second->GetComponent<TransformComponent>();
				if (!parentTransform)
				{
					continue;
				}

				if (childTransform->GetParent() != parentTransform)
				{
					if (childTransform->GetParent())
					{
						//childTransform->DetachFromParent();
						childTransform->DetachFromParentKeepLocal();
					}
					//childTransform->SetParent(parentTransform);
					childTransform->SetParentKeepLocal(parentTransform);
				}
			}
		};
	applyParentLinks(mergedGameObjects);
	//UI
}


template<typename PushFn>
static void EmitSubMeshes(
	const RenderData::MeshData& meshData,
	const RenderData::RenderItem& baseItem,
	const std::vector<MeshComponent::SubMeshMaterialOverride>* overrides,
	const AssetLoader& assetLoader,
	PushFn&& push)
{
	if (!meshData.subMeshes.empty())
	{
		size_t subMeshIndex = 0;
		for (const auto& subMesh : meshData.subMeshes)
		{
			RenderData::RenderItem item = baseItem;

			item.useSubMesh   = true;
			item.indexStart   = subMesh.indexStart;
			item.indexCount   = subMesh.indexCount;
			item.localToWorld = subMesh.localToWorld;
			item.boundsMin    = subMesh.boundsMin;
			item.boundsMax    = subMesh.boundsMax;
			item.hasBounds    = true;

			// submesh material override + 에디터에서 수정하면 갱신되도록
			if (!item.material.IsValid() && subMesh.material.IsValid())
				item.material = subMesh.material;

			if (overrides && subMeshIndex < overrides->size())
			{
				const auto& overrideData = overrides->at(subMeshIndex);
				if (overrideData.HasMaterialOverride())
				{
					const MaterialHandle overrideHandle = assetLoader.ResolveMaterial(
						overrideData.material.assetPath,
						overrideData.material.assetIndex);
					if (overrideHandle.IsValid())
					{
						item.material = overrideHandle;
						item.materialOverrides = RenderData::MaterialData{};
						item.useMaterialOverrides = false;
					}
				}

				if (overrideData.HasShaderOverrides())
				{
					RenderData::MaterialData materialOverrides{};
					const RenderData::MaterialData* sourceMaterial = nullptr;
					if (item.useMaterialOverrides)
					{
						sourceMaterial = &item.materialOverrides;
					}
					else if (item.material.IsValid())
					{
						sourceMaterial = assetLoader.GetMaterials().Get(item.material);
					}
					if (sourceMaterial)
					{
						materialOverrides = *sourceMaterial;
					}

					if (overrideData.shaderAsset.IsValid())
					{
						materialOverrides.shaderAsset = overrideData.shaderAsset;
					}
					else
					{
						materialOverrides.shaderAsset = ShaderAssetHandle::Invalid();
					}

					if (overrideData.vertexShader.IsValid())
					{
						materialOverrides.vertexShader = overrideData.vertexShader;
					}
					if (overrideData.pixelShader.IsValid())
					{
						materialOverrides.pixelShader = overrideData.pixelShader;
					}

					item.materialOverrides = materialOverrides;
					item.useMaterialOverrides = true;
				}
			}

			push(std::move(item));
			++subMeshIndex;
		}
	}
	else
	{
		RenderData::RenderItem item = baseItem;
		item.useSubMesh   = false;
		item.indexStart   = 0;
		item.indexCount   = static_cast<UINT32>(meshData.indices.size());
		item.localToWorld = Identity();
		item.boundsMin    = meshData.boundsMin;
		item.boundsMax    = meshData.boundsMax;
		item.hasBounds    = true;
		if (overrides && !overrides->empty())
		{
			const auto& overrideData = overrides->front();
			if (overrideData.HasMaterialOverride())
			{
				const MaterialHandle overrideHandle = assetLoader.ResolveMaterial(
					overrideData.material.assetPath,
					overrideData.material.assetIndex);
				if (overrideHandle.IsValid())
				{
					item.material = overrideHandle;
					item.materialOverrides = RenderData::MaterialData{};
					item.useMaterialOverrides = false;
				}
			}
			if (overrideData.HasShaderOverrides())
			{
				RenderData::MaterialData materialOverrides{};
				const RenderData::MaterialData* sourceMaterial = nullptr;
				if (item.useMaterialOverrides)
				{
					sourceMaterial = &item.materialOverrides;
				}
				else if (item.material.IsValid())
				{
					sourceMaterial = assetLoader.GetMaterials().Get(item.material);
				}
				if (sourceMaterial)
				{
					materialOverrides = *sourceMaterial;
				}

				if (overrideData.shaderAsset.IsValid())
				{
					materialOverrides.shaderAsset = overrideData.shaderAsset;
				}
				else
				{
					materialOverrides.shaderAsset = ShaderAssetHandle::Invalid();
				}

				if (overrideData.vertexShader.IsValid())
				{
					materialOverrides.vertexShader = overrideData.vertexShader;
				}
				if (overrideData.pixelShader.IsValid())
				{
					materialOverrides.pixelShader = overrideData.pixelShader;
				}

				item.materialOverrides = materialOverrides;
				item.useMaterialOverrides = true;
			}
		}


		push(std::move(item));
	}
}

static bool BuildStaticBaseItem(
	const Object& obj,
	MeshRenderer& renderer,
	RenderData::RenderLayer layer,
	RenderData::RenderItem& outItem,
	const MeshComponent*& outMeshComponent
)
{
	RenderData::RenderItem item{};

	if (!renderer.BuildRenderItem(item))
		return false;

	// mesh: MeshComponent
	const auto* meshComp = obj.GetComponent<MeshComponent>();
	if (!meshComp) return false;

	item.mesh = meshComp->GetMeshHandle();
	if (!item.mesh.IsValid()) return false;

	// material: renderer가 준 값이 없으면 MaterialComponent에서
	if (!item.material.IsValid())
	{
		if (const auto* matComp = obj.GetComponent<MaterialComponent>())
			item.material = matComp->GetMaterialHandle();
	}

	if (const auto* matComp = obj.GetComponent<MaterialComponent>())
	{
		if (matComp->HasOverrides())
		{
			item.materialOverrides = matComp->GetOverrides();
			item.useMaterialOverrides = true;
		}
	}

	outItem = std::move(item);
	outMeshComponent = meshComp;
	return true;
}

static void AppendSkinningPaletteIfAny(
	const SkeletalMeshComponent& skelComp,
	RenderData::FrameData& frameData,
	UINT32& outOffset,
	UINT32& outCount
)
{
	outOffset = 0;
	outCount  = 0;

	const auto& palette = skelComp.GetSkinningPalette();
#ifdef _DEBUG
// 	const RenderData::Skeleton* skeleton = nullptr;
// 	if (auto* loader = AssetLoader::GetActive())
// 	{
// 		skeleton = loader->GetSkeletons().Get(skelComp.GetSkeletonHandle());
// 	}
// 
// 	if (!skeleton)
// 	{
// 		AppendSkinningPaletteFrameDebug(skelComp.GetOwner(), nullptr, palette.size(), "skeleton_invalid");
// 	}
#endif

	if (palette.empty())
	{
#ifdef _DEBUG
		//AppendSkinningPaletteFrameDebug(skelComp.GetOwner(), skeleton, 0u, "palette_empty");
#endif
		return;
	}

#ifdef _DEBUG
// 	if (skeleton && palette.size() != skeleton->bones.size())
// 	{
// 		AppendSkinningPaletteFrameDebug(skelComp.GetOwner(), skeleton, palette.size(), "palette_bone_count_mismatch");
// 	}
#endif


	outOffset = static_cast<UINT32>(frameData.skinningPalettes.size());
	outCount  = static_cast<UINT32>(palette.size());
	frameData.skinningPalettes.insert(frameData.skinningPalettes.end(), palette.begin(), palette.end());
}

static void AppendGlobalPoseIfAny(
	const AnimationComponent* animComp,
	RenderData::FrameData& frameData,
	UINT32& outOffset,
	UINT32& outCount
)
{
	outOffset = 0;
	outCount = 0;

	if (!animComp)
	{
		return;
	}

	const auto& globalPose = animComp->GetGlobalPose();
	if (globalPose.empty())
	{
		return;
	}

	outOffset = static_cast<UINT32>(frameData.globalPoses.size());
	outCount = static_cast<UINT32>(globalPose.size());
	frameData.globalPoses.insert(frameData.globalPoses.end(), globalPose.begin(), globalPose.end());
}

static const AnimationComponent* FindAnimationComponent(const Object& obj)
{
	const auto anims = obj.GetComponentsDerived<AnimationComponent>();
	return anims.empty() ? nullptr : anims.front();
}

static bool BuildSkeletalBaseItem(
	const Object& obj,
	SkeletalMeshRenderer& renderer,
	RenderData::RenderLayer layer,
	RenderData::FrameData& frameData,
	RenderData::RenderItem& outItem,
	const MeshComponent*& outMeshComponent
)
{
	RenderData::RenderItem item{};
	if (!renderer.BuildRenderItem(item))
		return false;

	const auto* skelComp = obj.GetComponent<SkeletalMeshComponent>();
	if (!skelComp) return false;

	item.mesh	  = skelComp->GetMeshHandle();
	item.skeleton = skelComp->GetSkeletonHandle();
	if (!item.mesh.IsValid()) return false;

	// material resolve
	if (!item.material.IsValid())
	{
		if (const auto* matComp = obj.GetComponent<MaterialComponent>())
			item.material = matComp->GetMaterialHandle();
	}

	if (const auto* matComp = obj.GetComponent<MaterialComponent>())
	{
		if (matComp->HasOverrides())
		{
			item.materialOverrides = matComp->GetOverrides();
			item.useMaterialOverrides = true;
		}
	}

	// palette append (오브젝트 단위 1번)
	UINT32 paletteOffset = 0, paletteCount = 0;
	AppendSkinningPaletteIfAny(*skelComp, frameData, paletteOffset, paletteCount);

	item.skinningPaletteOffset = paletteOffset;
	item.skinningPaletteCount = paletteCount;

	UINT32 globalPoseOffset = 0, globalPoseCount = 0;
	const auto* animComp = FindAnimationComponent(obj);
	AppendGlobalPoseIfAny(animComp, frameData, globalPoseOffset, globalPoseCount);

	item.globalPoseOffset = globalPoseOffset;
	item.globalPoseCount = globalPoseCount;

	outItem = std::move(item);
	outMeshComponent = skelComp;
	return true;
}

static void AppendLights(const Object& obj, RenderData::FrameData& frameData)
{
	for (const auto* light : obj.GetComponentsDerived<LightComponent>())
	{
		if (!light) continue;

		frameData.lights.push_back(light->BuildLightData());
	}
}

template <typename ObjectMap>
void AppendFrameDataFromObjects(
	const ObjectMap& objects,
	const AssetLoader& assetLoader,
	RenderData::FrameData& frameData)
{
	for (const auto& [name, gameObject] : objects)
	{
		if (!gameObject)
		{
			continue;
		}

		// 라이트는 무조건 처리
		AppendLights(*gameObject, frameData);

		// Skeletal 우선
		{
			auto skelRenderers = gameObject->GetComponents<SkeletalMeshRenderer>();
			if (!skelRenderers.empty())
			{
				// 오브젝트에 SkeletalMeshRenderer가 여러 개 붙는 정책이면 for로 전부 처리
				for (auto* renderer : skelRenderers)
				{
					if (!renderer) continue;

					const auto renderLayer = static_cast<RenderData::RenderLayer>(renderer->GetRenderLayer());

					RenderData::RenderItem baseItem{};
					const MeshComponent* meshComponent = nullptr;
					if (!BuildSkeletalBaseItem(*gameObject, *renderer, renderLayer, frameData, baseItem, meshComponent))
						continue;

					const auto* meshData = assetLoader.GetMeshes().Get(baseItem.mesh);
					if (!meshData) continue;

					const auto* overrides = meshComponent ? &meshComponent->GetSubMeshMaterialOverrides() : nullptr;
					EmitSubMeshes(*meshData, baseItem, overrides, assetLoader,
						[&](RenderData::RenderItem&& item)
						{
							frameData.renderItems[renderLayer].push_back(std::move(item));
						});
				}

				// Skeletal 경로 탔으면 Static은 안 탐
				continue;
			}
		}

		// (4) Static
		{
			auto meshRenderers = gameObject->GetComponents<MeshRenderer>();
			for (auto* renderer : meshRenderers)
			{
				if (!renderer) continue;

				const auto renderLayer = static_cast<RenderData::RenderLayer>(renderer->GetRenderLayer());

				RenderData::RenderItem baseItem{};
				const MeshComponent* meshComponent = nullptr;
				if (!BuildStaticBaseItem(*gameObject, *renderer, renderLayer, baseItem, meshComponent))
					continue;

				auto* meshData = assetLoader.GetMeshes().Get(baseItem.mesh);
				if (!meshData) continue;

				const auto* overrides = meshComponent ? &meshComponent->GetSubMeshMaterialOverrides() : nullptr;
				EmitSubMeshes(*meshData, baseItem, overrides, assetLoader,
					[&](RenderData::RenderItem&& item)
					{
						frameData.renderItems[renderLayer].push_back(std::move(item));
					});
			}
		}
	}
}

void BuildCameraData(const std::shared_ptr<CameraObject>& camera, RenderData::FrameData& frameData, bool isGame = true)
{
	RenderData::FrameContext& context = frameData.context;

	if(isGame)
	{
		context.gameCamera.view		 = camera->GetViewMatrix();
		context.gameCamera.proj      = camera->GetProjMatrix();
		const auto viewport			 = camera->GetViewportSize();
		context.gameCamera.width	 = static_cast<UINT32>(viewport.Width);
		context.gameCamera.height	 = static_cast<UINT32>(viewport.Height);
		context.gameCamera.cameraPos = camera->GetEye();
		context.gameCamera.camNear = camera->GetNearZ();
		context.gameCamera.camFar = camera->GetFarZ();
	
		const auto view = XMLoadFloat4x4(&context.gameCamera.view);
		const auto proj = XMLoadFloat4x4(&context.gameCamera.proj);
		const auto viewProj = XMMatrixMultiply(view, proj);
		XMStoreFloat4x4(&context.gameCamera.viewProj, viewProj);
	}
	else
	{
		context.editorCamera.view      = camera->GetViewMatrix();
		context.editorCamera.proj      = camera->GetProjMatrix();
		const auto viewport			   = camera->GetViewportSize();
		context.editorCamera.width	   = static_cast<UINT32>(viewport.Width);
		context.editorCamera.height	   = static_cast<UINT32>(viewport.Height);
		context.editorCamera.cameraPos = camera->GetEye();
	
		const auto view = XMLoadFloat4x4(&context.editorCamera.view);
		const auto proj = XMLoadFloat4x4(&context.editorCamera.proj);
		const auto viewProj = XMMatrixMultiply(view, proj);
		XMStoreFloat4x4(&context.editorCamera.viewProj, viewProj);
	}
}

void Scene::BuildFrameData(RenderData::FrameData& frameData) const
{
	if (m_Name == "Title")
	{
		frameData.currScene = 0;
	}
	else if (m_Name == "Stage1")
	{
		frameData.currScene = 1;
	}
	else if (m_Name == "Stage2")
	{
		frameData.currScene = 2;
	}
	else if (m_Name == "Ending")
	{
		frameData.currScene = 3;
	}
	else
	{
		frameData.currScene = 0;
	}

	frameData.renderItems.clear();
	frameData.lights.clear();
	frameData.skinningPalettes.clear();
	frameData.globalPoses.clear();
	frameData.combatEnemyPositions.clear();
	frameData.playerPosition = XMFLOAT3{ 0.0f, 0.0f, 0.0f };
	frameData.hasPlayerPosition = false;

	RenderData::FrameContext& context = frameData.context;
	const UINT32 frameIndex = context.frameIndex;
	const FLOAT deltaTime = context.deltaTime;
	context = RenderData::FrameContext{};
	context.frameIndex = frameIndex;
	context.deltaTime = deltaTime;

	// 게임 카메라
	if (m_GameCamera)
	{
		BuildCameraData(m_GameCamera, frameData, true);
	}

	//적 위치 정보 및 플레이어 위치 정보
	const bool isCombatPhase = m_GameManager && m_GameManager->GetPhase() == Phase::TurnBasedCombat;
	bool playerPositionSet = false;
	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (!gameObject)
			continue;

		if (!playerPositionSet)
		{
			if (gameObject->GetComponent<PlayerComponent>())
			{
				if (auto* transform = gameObject->GetComponent<TransformComponent>())
				{
					frameData.playerPosition = transform->GetWorldPos();
					frameData.hasPlayerPosition = true;
					playerPositionSet = true;
				}
			}
		}

		if (isCombatPhase && gameObject->GetComponent<EnemyComponent>())
		{
			if (auto* transform = gameObject->GetComponent<TransformComponent>())
			{
				frameData.combatEnemyPositions.push_back(transform->GetWorldPos());
			}
		}
	}

	/*
	const bool isCombatPhase = m_GameManager && m_GameManager->GetPhase() == Phase::TurnBasedCombat;
	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (!gameObject)
			continue;

		if (gameObject->GetComponent<PlayerComponent>())
		{
			if (auto* transform = gameObject->GetComponent<TransformComponent>())
			{
				frameData.playerPosition = transform->GetWorldPos();
				frameData.hasPlayerPosition = true;
				break;
			}
		}
	}

	if (isCombatPhase)
	{
		for (const auto& [name, gameObject] : m_GameObjects)
		{
			if (!gameObject)
				continue;

			if (gameObject->GetComponent<EnemyComponent>())
			{
				if (auto* transform = gameObject->GetComponent<TransformComponent>())
				{
					frameData.combatEnemyPositions.push_back(transform->GetWorldPos());
				}
			}
		}
	}*/


	//에디터 nullptr
	if (m_EditorCamera)
	{
		BuildCameraData(m_EditorCamera, frameData, false);
	}

	AppendFrameDataFromObjects(m_GameObjects, *m_AssetLoader, frameData);

	//UIManager에서 UI Data 가공예정
}

void Scene::EnsureAutoComponentsForSave()
{
	auto addIfMissing = [](GameObject& object, auto* tag) {
		using ComponentType = std::remove_pointer_t<decltype(tag)>;
		if (!object.HasComponent<ComponentType>())
		{
			object.AddComponent<ComponentType>();
		}
		};

	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (!gameObject)
			continue;

		if (gameObject->GetComponent<MeshRenderer>())
		{
			addIfMissing(*gameObject, static_cast<MeshComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<MaterialComponent*>(nullptr));
		}

		if (gameObject->GetComponent<SkeletalMeshRenderer>())
		{
			addIfMissing(*gameObject, static_cast<SkeletalMeshComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<MaterialComponent*>(nullptr));
		}

		if (gameObject->GetComponent<SkinningAnimationComponent>())
		{
			addIfMissing(*gameObject, static_cast<SkeletalMeshComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<MaterialComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<AnimFSMComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<SkeletalMeshRenderer*>(nullptr));
		}

		if (gameObject->GetComponent<PlayerComponent>())
		{
			addIfMissing(*gameObject, static_cast<PlayerStatComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<PlayerMovementComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<PlayerFSMComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<PlayerMoveFSMComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<PlayerPushFSMComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<PlayerCombatFSMComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<PlayerInventoryFSMComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<PlayerShopFSMComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<PlayerDoorFSMComponent*>(nullptr));
		}

		if (gameObject->GetComponent<EnemyComponent>())
		{
			addIfMissing(*gameObject, static_cast<EnemyStatComponent*>(nullptr));
			addIfMissing(*gameObject, static_cast<EnemyMovementComponent*>(nullptr));
			//addIfMissing(*gameObject, static_cast<EnemyControllerComponent*>(nullptr));
		}

		if (auto* node = gameObject->GetComponent<NodeComponent>())
		{
			node->ClearHighlights();
		}
	}
}
void Scene::SetGameManager(GameManager* gameManager)
{
	m_GameManager = gameManager;
}

void Scene::SetSceneManager(SceneManager* sceneManager)
{
	m_SceneManager = sceneManager;
}
