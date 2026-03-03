#include "PlayerComponent.h"
#include "ReflectionMacro.h"
#include "Object.h"
#include "GameObject.h"
#include "CameraObject.h"
#include "Scene.h"
#include "GridSystemComponent.h"
#include "ServiceRegistry.h"
#include "ItemComponent.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"
#include "Event.h"
#include "InputManager.h"
#include "RayHelper.h"
#include "PlayerStatComponent.h"
#include "SkeletalMeshComponent.h"
#include "TransformComponent.h"
#include "BoxColliderComponent.h"
#include "NodeComponent.h"
#include "PushNodeComponent.h"
#include <algorithm>
#include <array>
#include <unordered_map>
#include <cfloat>
#include "SkinningAnimationComponent.h"
#include "MathHelper.h"
#include <cmath>
#include "GameManager.h"
#include "CombatManager.h"
#include "DoorComponent.h"
#include "SoundManager.h"
#include "PlayerCombatFSMComponent.h"
#include "PlayerFSMComponent.h"
#include "PlayerDoorFSMComponent.h"
#include "AssetLoader.h"
#include "GameDataRepository.h"
#include "UIManager.h"
#include "UIProgressBarComponent.h"
#include "UIImageComponent.h"
#include "UIButtonComponent.h"
#include "UIFSMComponent.h"
#include "PlayerVisualPresetComponent.h"

REGISTER_COMPONENT(PlayerComponent)
REGISTER_PROPERTY_READONLY(PlayerComponent, Q)
REGISTER_PROPERTY_READONLY(PlayerComponent, R)
REGISTER_PROPERTY(PlayerComponent, MoveResource)
REGISTER_PROPERTY_READONLY(PlayerComponent, RemainMoveResource)
REGISTER_PROPERTY(PlayerComponent, ActResource)
REGISTER_PROPERTY_READONLY(PlayerComponent, RemainActResource)
REGISTER_PROPERTY(PlayerComponent, CurrentWeaponCost)
REGISTER_PROPERTY(PlayerComponent, AttackRange)
REGISTER_PROPERTY(PlayerComponent, Money)
REGISTER_PROPERTY(PlayerComponent, DebugEquipItem)
REGISTER_PROPERTY_READONLY(PlayerComponent, DebugCombatMode)


//REGISTER_PROPERTY(PlayerComponent, Item)

static int AxialDistance(int q1, int r1, int q2, int r2)
{
	const int dq = q1 - q2;
	const int dr = r1 - r2;
	const int ds = dq + dr;
	return (std::abs(dq) + std::abs(dr) + std::abs(ds)) / 2;
}

namespace
{
	std::string NormalizePath(std::string value)
	{
		std::replace(value.begin(), value.end(), '\\', '/');
		return value;
	}

	bool EndsWithPath(const std::string& value, const std::string& suffix)
	{
		if (suffix.empty() || value.size() < suffix.size())
		{
			return false;
		}

		return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
	}

	TextureHandle ResolveTextureByPath(AssetLoader& assetLoader, const std::string& iconPath) 
	{
		if (iconPath.empty())
		{
			return TextureHandle::Invalid();
		}

		const std::string normalizedPath = NormalizePath(iconPath);
		const auto& keyToHandle = assetLoader.GetTextures().GetKeyToHandle();
		auto direct = keyToHandle.find(normalizedPath);
		if (direct != keyToHandle.end())
		{
			return direct->second;
		}

		std::string trimmedPath = normalizedPath;
		while (trimmedPath.rfind("../", 0) == 0)
		{
			trimmedPath.erase(0, 3);
			auto trimmed = keyToHandle.find(trimmedPath);
			if (trimmed != keyToHandle.end())
			{
				return trimmed->second;
			}
		}

		const size_t filenameStart = normalizedPath.find_last_of('/');
		const std::string filename = (filenameStart == std::string::npos)
			? normalizedPath
			: normalizedPath.substr(filenameStart + 1);
		if (filename.empty())
		{
			return TextureHandle::Invalid();
		}

		for (const auto& [key, handle] : keyToHandle)
		{
			const std::string normalizedKey = NormalizePath(key);
			if (normalizedKey == normalizedPath || EndsWithPath(normalizedKey, normalizedPath) || EndsWithPath(normalizedKey, filename))
			{
				return handle;
			}
		}

		return TextureHandle::Invalid();
	}

	UIImageComponent* FindImageComponentOrFirstChildImage(UIManager& uiManager,
		const std::string& sceneName,
		const std::string& objectName,
		std::shared_ptr<UIObject>& outTarget)
	{
		outTarget = uiManager.FindUIObject(sceneName, objectName);
		if (!outTarget)
		{
			return nullptr;
		}

		if (auto* image = outTarget->GetComponent<UIImageComponent>())
		{
			return image;
		}

		auto& allScenes = uiManager.GetUIObjects();
		auto sceneIt = allScenes.find(sceneName);
		if (sceneIt == allScenes.end())
		{
			return nullptr;
		}

		for (auto& [childName, childObject] : sceneIt->second)
		{
			if (!childObject)
			{
				continue;
			}

			if (childObject->GetParentName() != objectName)
			{
				continue;
			}

			if (auto* image = childObject->GetComponent<UIImageComponent>())
			{
				outTarget = childObject;
				return image;
			}
		}

		return nullptr;
	}

	void UpdateSlotIcon(UIManager& uiManager,
		const std::string& sceneName,
		const std::vector<std::string>& objectNames,
		const TextureHandle& iconTexture)
	{
		static std::unordered_map<std::string, TextureHandle> s_DefaultSlotTextures;

		for (const auto& objectName : objectNames)
		{
			std::shared_ptr<UIObject> target;
			auto* image = FindImageComponentOrFirstChildImage(uiManager, sceneName, objectName, target);
			if (!image || !target)
			{
				continue;
			}

			const std::string cacheKey = sceneName + "::" + target->GetName();
			auto cacheIt = s_DefaultSlotTextures.find(cacheKey);
			if (cacheIt == s_DefaultSlotTextures.end())
			{
				const TextureHandle currentTexture = image->GetTextureHandle();
				if (currentTexture.IsValid())
				{
					s_DefaultSlotTextures.emplace(cacheKey, currentTexture);
					cacheIt = s_DefaultSlotTextures.find(cacheKey);
				}
			}

			if (iconTexture.IsValid())
			{
				image->SetTextureHandle(iconTexture);
				target->SetIsVisible(true);
				return;
			}

			if (cacheIt != s_DefaultSlotTextures.end() && cacheIt->second.IsValid())
			{
				image->SetTextureHandle(cacheIt->second);
				target->SetIsVisible(true);
				return;
			}

			auto buttonObject = uiManager.FindUIObject(sceneName, objectName);
			if (buttonObject)
			{
				if (auto* button = buttonObject->GetComponent<UIButtonComponent>())
				{
					const TextureHandle buttonTexture = button->GetCurrentTextureHandle();
					if (buttonTexture.IsValid())
					{
						image->SetTextureHandle(buttonTexture);
						target->SetIsVisible(true);
						return;
					}
				}
			}

			target->SetIsVisible(false);
			return;
		}
	}

	TextureHandle ResolveIconTextureFromItemObject(GameObject* itemObject, AssetLoader& assetLoader)
	{
		if (!itemObject)
		{
			return TextureHandle::Invalid();
		}

		auto* itemComponent = itemObject->GetComponent<ItemComponent>();
		if (!itemComponent)
		{
			return TextureHandle::Invalid();
		}

		return ResolveTextureByPath(assetLoader, itemComponent->GetIconPath());
	}

	TextureHandle ResolveInfoTextureFromItemObject(Scene* scene, GameObject* itemObject, AssetLoader& assetLoader)
	{
		if (!scene || !itemObject)
		{
			return TextureHandle::Invalid();
		}

		auto* itemComponent = itemObject->GetComponent<ItemComponent>();
		if (!itemComponent)
		{
			return TextureHandle::Invalid();
		}

		auto& services = scene->GetServices();
		if (!services.Has<GameDataRepository>())
		{
			return TextureHandle::Invalid();
		}

		auto& repository = services.Get<GameDataRepository>();
		const ItemDefinition* definition = repository.GetItem(itemComponent->GetItemIndex());
		if (!definition)
		{
			return TextureHandle::Invalid();
		}

		return ResolveTextureByPath(assetLoader, definition->infoPath);
	}


	const std::vector<std::string>& GetMeleeSlotIconNameCandidates()
	{
		static const std::vector<std::string> kMeleeSlotIconNames =
		{
			"MainWeapon",
			"Player_MeleeIcon",
			"Player_Melee_Icon",
			"MeleeIcon",
			"MeleeItemIcon",
			"UI_Player_MeleeIcon"
		};
		return kMeleeSlotIconNames;
	}

	const std::array<std::vector<std::string>, 3>& GetThrowSlotButtonNameCandidates()
	{
		static const std::array<std::vector<std::string>, 3> kThrowSlotButtonNames =
		{
			std::vector<std::string>{ "Player_Throw1", "Player_Throw_1", "SubWeapon1" },
			std::vector<std::string>{ "Player_Throw2", "Player_Throw_2", "SubWeapon2" },
			std::vector<std::string>{ "Player_Throw3", "Player_Throw_3", "SubWeapon3" }
		};
		return kThrowSlotButtonNames;
	}

	const std::vector<std::string>& GetInventoryInfoPanelNameCandidates()
	{
		static const std::vector<std::string> kInventoryInfoPanelNames =
		{
			"MainWeaponInfo",
			"SubWeapon1Info",
			"SubWeapon2Info",
			"SubWeapon3Info",

			"InventoryInfo",
			"Player_InventoryInfo",
			"ItemInfo",
			"Player_ItemInfo",
			"InventoryInfoPanel"
		};
		return kInventoryInfoPanelNames;
	}

	const std::vector<std::string>& GetMeleeInfoPanelNameCandidates()
	{
		static const std::vector<std::string> kMeleeInfoPanelNames =
		{
			"MainWeaponInfo",
			"InventoryInfo",
			"Player_InventoryInfo",
			"ItemInfo",
			"Player_ItemInfo",
			"InventoryInfoPanel"
		};
		return kMeleeInfoPanelNames;
	}

	const std::vector<std::string>& GetThrowInfoPanelNameCandidates(int slotIndex)
	{
		static const std::vector<std::string> kEmpty{};
		static const std::array<std::vector<std::string>, 3> kThrowInfoPanelNames =
		{
			std::vector<std::string>{ "SubWeapon1Info", "InventoryInfo", "Player_InventoryInfo", "ItemInfo", "Player_ItemInfo", "InventoryInfoPanel" },
			std::vector<std::string>{ "SubWeapon2Info", "InventoryInfo", "Player_InventoryInfo", "ItemInfo", "Player_ItemInfo", "InventoryInfoPanel" },
			std::vector<std::string>{ "SubWeapon3Info", "InventoryInfo", "Player_InventoryInfo", "ItemInfo", "Player_ItemInfo", "InventoryInfoPanel" }
		};

		if (slotIndex < 0 || slotIndex >= static_cast<int>(kThrowInfoPanelNames.size()))
		{
			return kEmpty;
		}

		return kThrowInfoPanelNames[slotIndex];
	}

	void UpdateInfoPanelTexture(UIManager& uiManager,
		const std::string& sceneName,
		const std::vector<std::string>& panelNames,
		const TextureHandle& infoTexture)
	{
		for (const auto& panelName : panelNames)
		{
			static std::unordered_map<std::string, TextureHandle> s_DefaultInfoPanelTextures;

			std::shared_ptr<UIObject> target;
			auto* image = FindImageComponentOrFirstChildImage(uiManager, sceneName, panelName, target);
			if (!image || !target)
			{
				continue;
			}

			const std::string cacheKey = sceneName + "::" + target->GetName();
			auto cacheIt = s_DefaultInfoPanelTextures.find(cacheKey);
			if (cacheIt == s_DefaultInfoPanelTextures.end())
			{
				const TextureHandle currentTexture = image->GetTextureHandle();
				if (currentTexture.IsValid())
				{
					s_DefaultInfoPanelTextures.emplace(cacheKey, currentTexture);
					cacheIt = s_DefaultInfoPanelTextures.find(cacheKey);
				}
			}

			if (infoTexture.IsValid())
			{
				image->SetTextureHandle(infoTexture);
				return;
			}

			if (cacheIt != s_DefaultInfoPanelTextures.end() && cacheIt->second.IsValid()) 
			{
				image->SetTextureHandle(cacheIt->second);
			}
			return;
		}
	}

	const std::vector<std::string>& GetGroundItemInfoPanelNameCandidates()
	{
		// 바닥 아이템 hover 정보 전용 패널 후보 이름.
		// 프로젝트 UI에서 아래 이름 중 하나로 패널을 배치하면 자동으로 연결된다.
		static const std::vector<std::string> kGroundItemInfoPanelNames =
		{
			"GroundItemInfo",
			"Player_GroundItemInfo",
			"WorldItemInfo",
			"HoverItemInfo"
		};
		return kGroundItemInfoPanelNames;
	}

	std::shared_ptr<UIObject> FindFirstInfoPanelObject(UIManager& uiManager,
		const std::string& sceneName,
		const std::vector<std::string>& panelNames)
	{
		for (const auto& panelName : panelNames)
		{
			auto panel = uiManager.FindUIObject(sceneName, panelName);
			if (panel)
			{
				return panel;
			}
		}

		return nullptr;
	}

	void UpdateGroundItemInfoPanel(UIManager& uiManager,
		const std::string& sceneName,
		const TextureHandle& infoTexture)
	{
		auto infoPanel = FindFirstInfoPanelObject(uiManager, sceneName, GetGroundItemInfoPanelNameCandidates());
		if (!infoPanel)
		{
			return;
		}

		std::shared_ptr<UIObject> target;
		auto* image = FindImageComponentOrFirstChildImage(uiManager, sceneName, infoPanel->GetName(), target);
		if (!image || !target)
		{
			return;
		}

		image->SetTextureHandle(infoTexture);
		target->SetIsVisible(infoTexture.IsValid());
	}

	const std::vector<std::string>& GetEnemyHoverInfoPanelNameCandidates()
	{
		static const std::vector<std::string> kEnemyHoverInfoPanelNames =
		{
			"EnemyInfo",
		};
		return kEnemyHoverInfoPanelNames;
	}

	const std::vector<std::string>& GetEnemyHoverHpBarNameCandidates()
	{
		static const std::vector<std::string> kEnemyHoverHpBarNames =
		{
			"EnemyHPBar",
		};
		return kEnemyHoverHpBarNames;
	}

	UIProgressBarComponent* FindFirstProgressBarOrFirstChild(UIManager& uiManager,
		const std::string& sceneName,
		const std::string& objectName,
		std::shared_ptr<UIObject>& outTarget)
	{
		outTarget = uiManager.FindUIObject(sceneName, objectName);
		if (!outTarget)
		{
			return nullptr;
		}

		if (auto* progress = outTarget->GetComponent<UIProgressBarComponent>())
		{
			return progress;
		}

		auto& allScenes = uiManager.GetUIObjects();
		auto sceneIt = allScenes.find(sceneName);
		if (sceneIt == allScenes.end())
		{
			return nullptr;
		}

		for (auto& [childName, childObject] : sceneIt->second)
		{
			(void)childName;
			if (!childObject)
			{
				continue;
			}

			if (childObject->GetParentName() != objectName)
			{
				continue;
			}

			if (auto* progress = childObject->GetComponent<UIProgressBarComponent>())
			{
				outTarget = childObject;
				return progress;
			}
		}

		return nullptr;
	}

	TextureHandle ResolveEnemyHoverTexture(GameObject* enemyObject)
	{
		auto toLowerCopy = [](std::string value)
			{
				std::transform(value.begin(), value.end(), value.begin(),
					[](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
				return value;
			};

		auto resolveFromMaterial = [](GameObject* object)
			{
				if (!object)
				{
					return TextureHandle::Invalid();
				}

				auto* material = object->GetComponent<MaterialComponent>();
				if (!material)
				{
					return TextureHandle::Invalid();
				}

				const auto& textures = material->GetOverrides().textures;
				const size_t albedoIndex = static_cast<size_t>(RenderData::MaterialTextureSlot::Albedo);
				if (albedoIndex >= textures.size())
				{
					return TextureHandle::Invalid();
				}

				return textures[albedoIndex];
			};

		auto resolveByTextureFileName = [](AssetLoader& assetLoader, const char* fileName)
			{
				if (!fileName || *fileName == '\0')
				{
					return TextureHandle::Invalid();
				}

				const std::string normalizedFileName = NormalizePath(fileName);
				const auto& keyToHandle = assetLoader.GetTextures().GetKeyToHandle();
				for (const auto& [key, handle] : keyToHandle)
				{
					const std::string normalizedKey = NormalizePath(key);
					if (EndsWithPath(normalizedKey, normalizedFileName))
					{
						return handle;
					}
				}

				return TextureHandle::Invalid();
			};

		auto resolveByMaterialName = [&](AssetLoader& assetLoader, GameObject* object)
			{
				auto* material = object ? object->GetComponent<MaterialComponent>() : nullptr;
				if (!material)
				{
					return TextureHandle::Invalid();
				}

				const MaterialHandle materialHandle = material->GetMaterialHandle();
				if (!materialHandle.IsValid())
				{
					return TextureHandle::Invalid();
				}

				const std::string* materialKey = assetLoader.GetMaterials().GetKey(materialHandle);
				const std::string* materialDisplayName = assetLoader.GetMaterials().GetDisplayName(materialHandle);
				const std::string keyLower = materialKey ? toLowerCopy(*materialKey) : std::string{};
				const std::string displayLower = materialDisplayName ? toLowerCopy(*materialDisplayName) : std::string{};

				auto containsToken = [&](const char* token)
					{
						if (!token || *token == '\0')
						{
							return false;
						}

						return keyLower.find(token) != std::string::npos ||
							displayLower.find(token) != std::string::npos;
					};

				const char* fileName = nullptr;
				/*if (containsToken("boss_"))
				{
					fileName = "enemyStatus_frameBoss.png";
				}
				else if (containsToken("e2_"))
				{
					fileName = "enemyStatus_frame100002Simhyung.png";
				}
				else if (containsToken("e1_"))
				{
					fileName = "enemyStatus_frame100001Umjinsik.png";
				}*/

				if (!fileName)
				{
					return TextureHandle::Invalid();
				}

				return resolveByTextureFileName(assetLoader, fileName);
			};

		if (!enemyObject)
		{
			return TextureHandle::Invalid();
		}

		if (auto* enemy = enemyObject->GetComponent<EnemyComponent>())
		{
			const TextureHandle& hoverInfoTexture = enemy->GetHoverInfoTextureHandle();
			if (hoverInfoTexture.IsValid())
			{
				return hoverInfoTexture;
			}
		}

		auto* scene = enemyObject->GetScene();
		if (!scene)
		{
			return resolveFromMaterial(enemyObject);
		}

		auto& services = scene->GetServices();
		if (!services.Has<AssetLoader>())
		{
			return resolveFromMaterial(enemyObject);
		}

		auto& assetLoader = services.Get<AssetLoader>();
		if (TextureHandle byMaterialName = resolveByMaterialName(assetLoader, enemyObject); byMaterialName.IsValid())
		{
			return byMaterialName;
		}

		static constexpr std::array<const char*, 3> kEnemyHoverFileNames =
		{
			"enemyStatus_frame100001Umjinsik.png",
			"enemyStatus_frame100002Simhyung.png",
			"enemyStatus_frameBoss.png",
		};

		auto* enemyStat = enemyObject->GetComponent<EnemyStatComponent>();
		if (enemyStat)
		{
			const int enemyType = enemyStat->GetEnemyType();
			if (enemyType > 0)
			{
				const size_t textureIndex = static_cast<size_t>(enemyType - 1);
				if (textureIndex < kEnemyHoverFileNames.size())
				{
					TextureHandle byFileName = resolveByTextureFileName(assetLoader, kEnemyHoverFileNames[textureIndex]);
					if (byFileName.IsValid())
					{
						return byFileName;
					}
				}
			}
		}

		return resolveFromMaterial(enemyObject);
	}

	void UpdateEnemyHoverInfoPanel(UIManager& uiManager,
		const std::string& sceneName,
		const TextureHandle& enemyTexture,
		float hpPercent,
		bool hasEnemy)
	{
		auto infoPanel = FindFirstInfoPanelObject(uiManager, sceneName, GetEnemyHoverInfoPanelNameCandidates());
		if (!infoPanel)
		{
			return;
		}

		std::shared_ptr<UIObject> imageTarget;
		auto* image = FindImageComponentOrFirstChildImage(uiManager, sceneName, infoPanel->GetName(), imageTarget);
		if (image)
		{
			image->SetTextureHandle(enemyTexture);
		}

		for (const auto& barName : GetEnemyHoverHpBarNameCandidates())
		{
			std::shared_ptr<UIObject> hpBarTarget;
			auto* hpBar = FindFirstProgressBarOrFirstChild(uiManager, sceneName, barName, hpBarTarget);
			if (!hpBar)
			{
				continue;
			}

			const float percent = (std::max)(0.0f, (std::min)(1.0f, hpPercent));
			hpBar->SetPercent(hasEnemy ? percent : 0.0f);
			if (hpBarTarget)
			{
				hpBarTarget->SetIsVisible(hasEnemy);
			}
			break;
		}

		infoPanel->SetIsVisible(hasEnemy);
	}

	void BindInventoryInfoHoverEvents(const std::shared_ptr<UIObject>& buttonObject,
		const std::shared_ptr<UIObject>& infoPanel,
		const std::string& showEventName,
		const std::string& hideEventName,
		const std::string& showCallbackId,
		const std::string& hideCallbackId)
	{
		if (!buttonObject || !infoPanel || showEventName.empty() || hideEventName.empty())
		{
			return;
		}

		auto* fsm = buttonObject->GetComponent<UIFSMComponent>();
		if (!fsm)
		{
			return;
		}

		auto callbacks = fsm->GetEventCallbacks();
		auto ensureCallback = [&callbacks](const std::string& eventName, const std::string& callbackId)
			{
				auto found = std::find_if(callbacks.begin(), callbacks.end(),
					[&](const UIFSMEventCallback& entry)
					{
						return entry.eventName == eventName && entry.callbackId == callbackId;
					});

				if (found == callbacks.end())
				{
					callbacks.push_back(UIFSMEventCallback{ eventName, callbackId });
				}
			};

		ensureCallback(showEventName, showCallbackId);
		ensureCallback(hideEventName, hideCallbackId);
		fsm->SetEventCallbacks(callbacks);

		auto* infoPanelFsm = infoPanel->GetComponent<UIFSMComponent>();
		std::weak_ptr<UIObject> weakInfoPanel = infoPanel;
		fsm->RegisterCallback(showCallbackId, [weakInfoPanel, infoPanelFsm, showEventName](const std::string&, const void*)
			{
				auto panel = weakInfoPanel.lock();
				if (!panel)
				{
					return;
				}

				if (infoPanelFsm)
				{
					infoPanelFsm->TriggerEventByName(showEventName);
				}
			});

		fsm->RegisterCallback(hideCallbackId, [weakInfoPanel, infoPanelFsm, hideEventName](const std::string&, const void*)
			{
				auto panel = weakInfoPanel.lock();
				if (!panel)
				{
					return;
				}

				if (infoPanelFsm)
				{
					infoPanelFsm->TriggerEventByName(hideEventName);
				}
			});
	}

	//---
	const std::vector<std::string>& GetThrowSlotIconNameCandidates(int slotIndex)
	{
		static const std::vector<std::string> kEmpty{};
		static const std::array<std::vector<std::string>, 3> kThrowSlotIconNames =
		{
			std::vector<std::string>{ "SubWeapon1" },
			std::vector<std::string>{ "SubWeapon2" },
			std::vector<std::string>{ "SubWeapon3" }
		};

		if (slotIndex < 0 || slotIndex >= static_cast<int>(kThrowSlotIconNames.size()))
		{
			return kEmpty;
		}

		return kThrowSlotIconNames[slotIndex];
	}

	float DistanceSq2D(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;
		return dx * dx + dz * dz;
	}

	int ToItemType(ItemCategory category)
	{
		switch (category)
		{
		case ItemCategory::Currency:
			return static_cast<int>(ItemType::GOLD);
		case ItemCategory::Healing:
			return static_cast<int>(ItemType::HEAL);
		case ItemCategory::Equipment:
			return static_cast<int>(ItemType::EQUIPMENT);
		case ItemCategory::Throwable:
			return static_cast<int>(ItemType::THROW);
		default:
			return static_cast<int>(ItemType::GOLD);
		}
	}

	std::string BuildEquipMeshPath(const ItemDefinition& definition)
	{
		if (!definition.equipMeshPath.empty())
		{
			return definition.equipMeshPath;
		}

		if (definition.meshPath.empty())
		{
			return {};
		}

		std::string path = definition.meshPath;
		const size_t dot = path.find_last_of('.');
		const size_t insertPos = (dot == std::string::npos) ? path.size() : dot;
		const std::string base = path.substr(0, insertPos);
		if (base.size() >= 5 && base.compare(base.size() - 5, 5, "_grab") == 0)
		{
			return path;
		}

		path.insert(insertPos, "_grab");
		return path;
	}

	void ApplyItemDefinition(ItemComponent& item, const ItemDefinition& definition, const std::string& meshPath)
	{
		item.SetItemIndex(definition.index);
		item.SetType(ToItemType(definition.category));
		item.SetIconPath(definition.iconPath);
		item.SetMeshPath(meshPath);
		item.SetPrice(definition.basePrice);
		item.SetActionPointCost(definition.actionPointCost);
		item.SetMeleeAttackRange(definition.range);
		item.SetThrowRange(definition.throwRange);
		item.SetDifficultyGroup(definition.difficultyGroup);
		item.SetHealth(definition.constitutionModifier);
		item.SetStrength(definition.strengthModifier);
		item.SetAgility(definition.agilityModifier);
		item.SetSense(definition.senseModifier);
		item.SetSkill(definition.skillModifier);
		item.SetDEF(definition.defenseBonus);
		if (definition.diceType > 0)
		{
			item.SetDiceType(definition.diceType);
		}
		if (definition.diceRoll > 0)
		{
			item.SetDiceRoll(definition.diceRoll);
		}
		if (definition.baseModifier > 0)
		{
			item.SetBaseModifier(definition.baseModifier);
		}
	}

	std::string BuildEquipObjectName(Scene& scene, const std::string& base)
	{
		const std::string root = base.empty() ? "EquippedItem" : base;
		std::string name = root + "_equip";
		int suffix = 1;
		while (scene.HasGameObjectName(name))
		{
			name = root + "_equip_" + std::to_string(suffix++);
		}
		return name;
	}

	void EnsureRenderComponents(GameObject& object, const std::string& meshPath)
	{
		auto* meshComponent = object.GetComponent<MeshComponent>();
		if (!meshComponent)
		{
			meshComponent = object.AddComponent<MeshComponent>();
		}

		auto* meshRenderer = object.GetComponent<MeshRenderer>();
		if (!meshRenderer)
		{
			meshRenderer = object.AddComponent<MeshRenderer>();
		}
		if (meshRenderer)
		{
			meshRenderer->SetRenderLayer(static_cast<UINT8>(RenderData::RenderLayer::OpaqueItems));
			meshRenderer->SetVisible(true);
		}

		auto* materialComponent = object.GetComponent<MaterialComponent>();
		if (!materialComponent)
		{
			materialComponent = object.AddComponent<MaterialComponent>();
		}

		if (meshPath.empty())
		{
			return;
		}

		auto* loader = AssetLoader::GetActive();
		if (!loader)
		{
			return;
		}

		const auto* asset = loader->GetAsset(meshPath);
		if (!asset)
		{
			return;
		}

		if (meshComponent && !asset->meshes.empty())
		{
			if (!meshComponent->GetMeshHandle().IsValid())
			{
				meshComponent->SetMeshHandle(asset->meshes.front());
			}
		}

		if (materialComponent && !asset->materials.empty())
		{
			if (!materialComponent->GetMaterialHandle().IsValid())
			{
				materialComponent->SetMaterialHandle(asset->materials.front());
			}
		}
	}

	GameObject* SpawnEquippedItem(Scene& scene, const ItemDefinition& definition)
	{
		const std::string equipName = BuildEquipObjectName(scene, definition.name);
		auto equippedObject = scene.CreateGameObject(equipName);
		if (!equippedObject)
		{
			return nullptr;
		}

		auto* itemComponent = equippedObject->AddComponent<ItemComponent>();
		if (!itemComponent)
		{
			return nullptr;
		}

		const std::string meshPath = BuildEquipMeshPath(definition);
		ApplyItemDefinition(*itemComponent, definition, meshPath);
		itemComponent->SetIsEquiped(true);
		EnsureRenderComponents(*equippedObject, meshPath);

		return equippedObject.get();
	}

	GameObject* FindGameObjectByName(Scene* scene, const std::string& name)
	{
		if (!scene || name.empty())
		{
			return nullptr;
		}

		const auto& objects = scene->GetGameObjects();
		auto it = objects.find(name);
		if (it == objects.end())
		{
			return nullptr;
		}

		return it->second.get();
	}


	NodeComponent* FindClosestNodeByPosition(GridSystemComponent* grid, const XMFLOAT3& position)
	{
		if (!grid)
		{
			return nullptr;
		}

		float closestDistSq = FLT_MAX;
		NodeComponent* closestNode = nullptr;
		for (auto* node : grid->GetNodes())
		{
			if (!node)
			{
				continue;
			}

			auto* nodeOwner = node->GetOwner();
			auto* nodeTransform = nodeOwner ? nodeOwner->GetComponent<TransformComponent>() : nullptr;
			if (!nodeTransform)
			{
				continue;
			}

			const float distSq = DistanceSq2D(nodeTransform->GetPosition(), position);
			if (distSq < closestDistSq)
			{
				closestDistSq = distSq;
				closestNode = node;
			}
		}

		return closestNode;
	}

	ItemComponent* FindClosestItemHit(Scene* scene, const Ray& ray, float& outT)
	{
		if (!scene)
		{
			return nullptr;
		}

		float closestT = FLT_MAX;
		ItemComponent* closestItem = nullptr;

		for (const auto& [name, object] : scene->GetGameObjects())
		{
			(void)name;
			if (!object)
			{
				continue;
			}

			auto* item = object->GetComponent<ItemComponent>();
			if (!item)
			{
				continue;
			}

			auto* collider = object->GetComponent<BoxColliderComponent>();
			if (!collider || !collider->HasBounds())
			{
				continue;
			}

			float hitT = 0.0f;
			if (!collider->IntersectsRay(ray.m_Pos, ray.m_Dir, hitT))
			{
				continue;
			}

			if (hitT >= 0.0f && hitT < closestT)
			{
				closestT = hitT;
				closestItem = item;
			}
		}

		if (!closestItem)
		{
			return nullptr;
		}

		outT = closestT;
		return closestItem;
	}

	EnemyComponent* FindClosestEnemyHit(Scene* scene, const Ray& ray, float& outT)
	{
		if (!scene)
		{
			return nullptr;
		}

		float closestT = FLT_MAX;
		EnemyComponent* closestEnemy = nullptr;

		for (const auto& [name, object] : scene->GetGameObjects())
		{
			(void)name;
			if (!object)
			{
				continue;
			}

			auto* enemy = object->GetComponent<EnemyComponent>();
			if (!enemy)
			{
				continue;
			}

			auto* enemyStat = object->GetComponent<EnemyStatComponent>();
			if (enemyStat && enemyStat->IsDead())
			{
				continue;
			}

			auto* collider = object->GetComponent<BoxColliderComponent>();
			if (!collider || !collider->HasBounds())
			{
				continue;
			}

			float hitT = 0.0f;
			if (!collider->IntersectsRay(ray.m_Pos, ray.m_Dir, hitT))
			{
				continue;
			}

			if (hitT >= 0.0f && hitT < closestT)
			{
				closestT = hitT;
				closestEnemy = enemy;
			}
		}

		if (!closestEnemy)
		{
			return nullptr;
		}

		outT = closestT;
		return closestEnemy;
	}

	void DispatchPlayerStateEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerFSMComponent>())
			fsm->DispatchEvent(eventName);
	}

	void DispatchCombatEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerCombatFSMComponent>())
			fsm->DispatchEvent(eventName);
	}

	void DispatchDoorEvent(Object* owner, const char* eventName)
	{
		if (!owner || !eventName) return;

		if (auto* fsm = owner->GetComponent<PlayerDoorFSMComponent>())
			fsm->DispatchEvent(eventName);
	}
}

static NodeComponent* FindClosestNodeHit(Scene* scene, const Ray& ray, float& outT)
{
	if (!scene)
	{
		return nullptr;
	}

	float closestT = FLT_MAX;
	NodeComponent* closestNode = nullptr;

	for (const auto& [name, object] : scene->GetGameObjects())
	{
		if (!object)
		{
			continue;
		}

		auto* node = object->GetComponent<NodeComponent>();
		if (!node)
		{
			continue;
		}

		auto* collider = object->GetComponent<BoxColliderComponent>();
		if (!collider || !collider->HasBounds())
		{
			continue;
		}

		float hitT = 0.0f;
		if (!collider->IntersectsRay(ray.m_Pos, ray.m_Dir, hitT))
		{
			continue;
		}

		if (hitT >= 0.0f && hitT < closestT)
		{
			closestT = hitT;
			closestNode = node;
		}
	}

	if (!closestNode)
	{
		return nullptr;
	}

	outT = closestT;
	return closestNode;
}

static EnemyComponent* FindEnemyInRange(GridSystemComponent* grid, int playerQ, int playerR, int range)
{
	if (!grid || range < 0)
	{
		return nullptr;
	}

	for (auto* enemy : grid->GetEnemies())
	{
		if (!enemy || enemy->GetActorId() == 0)
		{
			continue;
		}

		const int distance = AxialDistance(playerQ, playerR, enemy->GetQ(), enemy->GetR());
		if (distance <= range)
		{
			return enemy;
		}
	}

	return nullptr;
}

static EnemyComponent* FindEnemyAt(GridSystemComponent* grid, int q, int r)
{
	if (!grid)
	{
		return nullptr;
	}



	for (auto* enemy : grid->GetEnemies())
	{
		if (!enemy)
		{
			continue;
		}
		auto* enemyOwner = enemy->GetOwner();
		auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
		if (enemyStat && enemyStat->IsDead())
		{
			continue;
		}
		if (enemy->GetQ() == q && enemy->GetR() == r)
		{
			return enemy;
		}
	}

	return nullptr;
}



PlayerComponent::PlayerComponent() {

}

PlayerComponent::~PlayerComponent() {
	// Event Listener 쓰는 경우만	
	GetEventDispatcher().RemoveListener(EventType::TurnChanged, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().RemoveListener(EventType::MouseRightClick, this);
	GetEventDispatcher().RemoveListener(EventType::Hovered, this);
}

void PlayerComponent::Start()
{
	//start시 GameManagaer get
	ResetTurnResources();
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene) { return; }

	GetEventDispatcher().AddListener(EventType::TurnChanged, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftClick, this);
	GetEventDispatcher().AddListener(EventType::MouseLeftDoubleClick, this);
	GetEventDispatcher().AddListener(EventType::MouseRightClick, this);
	GetEventDispatcher().AddListener(EventType::Hovered, this);
	const auto& objects = scene->GetGameObjects();

	for (const auto& [name, object] : objects) {
		if (!object) { continue; }

		if (auto* grid = object->GetComponent<GridSystemComponent>()) {
			m_GridSystem = grid;
			break;
		}
	}

	m_DebugEquipItem = false;
}

void PlayerComponent::Update(float deltaTime) {
	(void)deltaTime;

	//defense
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene || scene->GetIsPause())
	{
		return;
	}

	SyncCombatModeFromInventory();
	auto* playerStat = owner->GetComponent<PlayerStatComponent>();
	const bool isDead = playerStat && playerStat->IsDead();
	if (!m_HasAppliedCombatVisual || m_LastVisualCombatMode != m_CombatMode || m_LastVisualIsDead != isDead) 
	{
		ApplyVisualPresetByCombatMode();
		m_LastVisualCombatMode = m_CombatMode;
		m_LastVisualIsDead = isDead;
		m_HasAppliedCombatVisual = true;
	}

	if (m_LastRemainMoveResource != m_RemainMoveResource
		|| m_LastRemainActResource != m_RemainActResource
		|| m_LastMoveResource != m_MoveResource
		|| m_LastActResource != m_ActResource)
	{
		UpdateResourceUI();
	}

	auto* gameManager = scene->GetGameManager();
	switch (m_CombatMode)
	{
	case CombatMode::Idle:
		m_DebugCombatMode = "IdleMode";
		break;
	case CombatMode::Melee:
		m_DebugCombatMode = "MeleeMode";
		break;
	case CombatMode::Throw:
		m_DebugCombatMode = "ThrowMode";
		break;
	default:
		m_DebugCombatMode = "IdleMode";
		break;
	}
	const bool allowExplorationTurn = !gameManager && m_CurrentTurn == Turn::PlayerTurn;
	//아이템 장착 테스트


	//Player Turn 종료 조건
// 	if (allowExplorationTurn) {
// 		m_TurnElapsed += deltaTime;
// 
// 		if (!m_TurnEndRequested && m_TurnElapsed >= m_PlayerTurnTime) {
// 			//종료(턴 전환)
// 			GetEventDispatcher().Dispatch(EventType::PlayerTurnEndRequested, nullptr);
// 			m_TurnEndRequested = true;
// 		}
// 	}
	// 이제 전체 턴 관리하는 GameManager에서 넘김 여기선 UI에서 턴 종료했을때만 처리하면 될듯


	//임시로 첫번째 자식을 가지고 있는 아이템으로 지정
	//auto* transformcomponent = owner->GetComponent<TransformComponent>();
	//{
	//	if (!transformcomponent->GetChildrens().empty() && m_MeleeItem == nullptr)
	//	{
	//		GameObject* item = dynamic_cast<GameObject*>(transformcomponent->GetChildrens()[0]->GetOwner());
	//		auto* itemcomp = item->GetComponent<ItemComponent>();
	//		if (itemcomp && itemcomp->GetType() == 1)
	//		{
	//			m_MeleeItem = item;
	//			itemcomp->SetIsEquiped(true);
	//			m_InventoryItemIds.push_back(item->GetName());

	//		}
	//	}

	//}


	//근접 무기 스탯 적용
	auto resetMeleeCombatStats = [&]()
		{
			m_AttackRange = 1;
			m_CurrentWeaponCost = 1;

			auto* playerstatcomponent = owner->GetComponent<PlayerStatComponent>();
			if (!playerstatcomponent)
			{
				return;
			}

			playerstatcomponent->SetRange(1);
			playerstatcomponent->SetEquipmentDefenseBonus(0);
			playerstatcomponent->SetEquipmentHealthBonus(0);
			playerstatcomponent->SetEquipmentStrengthBonus(0);
			playerstatcomponent->SetEquipmentAgilityBonus(0);
			playerstatcomponent->SetEquipmentSenseBonus(0);
			playerstatcomponent->SetEquipmentSkillBonus(0);

			m_IsApplyMeleeStat = false;
		};

	if (m_MeleeItem != nullptr)
	{
		auto* itemcomponent = m_MeleeItem->GetComponent<ItemComponent>();
		if (!itemcomponent)
		{
			resetMeleeCombatStats();
		}
		else if (itemcomponent->GetIsEquiped())
		{
			// 근접 무기 전투값 동기화 (사거리/AP 소모)
			m_AttackRange = max(0, itemcomponent->GetMeleeAttackRange());
			m_CurrentWeaponCost = max(0, itemcomponent->GetActionPointCost());

			//근접 무기의 스탯 적용하기
			if (!m_IsApplyMeleeStat)
			{
				auto* playerstatcomponent = owner->GetComponent<PlayerStatComponent>();
				if (!playerstatcomponent) return;

				int health = playerstatcomponent->GetHealth();
				int strength = playerstatcomponent->GetStrength();
				int agility = playerstatcomponent->GetAgility();
				int sense = playerstatcomponent->GetSense();
				int skill = playerstatcomponent->GetSkill();

				int ihealth = itemcomponent->GetHealth();
				int istrength = itemcomponent->GetStrength();
				int iagility = itemcomponent->GetAgility();
				int isense = itemcomponent->GetSense();
				int iskill = itemcomponent->GetSkill();
				int idefense = itemcomponent->GetDEF();
				int irange = itemcomponent->GetMeleeAttackRange();

				playerstatcomponent->SetHealth(health);
				playerstatcomponent->SetStrength(strength);
				playerstatcomponent->SetAgility(agility);
				playerstatcomponent->SetSense(sense);
				playerstatcomponent->SetSkill(skill);
				playerstatcomponent->SetEquipmentDefenseBonus(idefense);
				playerstatcomponent->SetRange(static_cast<int>(irange));
				playerstatcomponent->SetEquipmentHealthBonus(ihealth);
				playerstatcomponent->SetEquipmentStrengthBonus(istrength);
				playerstatcomponent->SetEquipmentAgilityBonus(iagility);
				playerstatcomponent->SetEquipmentSenseBonus(isense);
				playerstatcomponent->SetEquipmentSkillBonus(iskill);

				m_IsApplyMeleeStat = true;
			}
		}
		else
		{
			resetMeleeCombatStats();
		}
	}
	else
	{
		resetMeleeCombatStats();
	}

	// 장착 무기 표시/숨김: CombatMode에 따라 근접, 던지기, 없음(Idle)
	std::vector<GameObject*> consumableObjects;
	consumableObjects.reserve(3);
	for (const auto& itemName : m_ConsumableItemNames)
	{
		auto* itemObject = FindGameObjectByName(scene, itemName);
		if (!itemObject)
		{
			continue;
		}

		consumableObjects.push_back(itemObject);
	}

	ItemComponent* throwItem = nullptr;
	GameObject* throwItemObject = nullptr;
	if (TryGetConsumableThrowItem(throwItem) && throwItem)
	{
		auto* throwOwner = throwItem->GetOwner();
		throwItemObject = throwOwner ? dynamic_cast<GameObject*>(throwOwner) : nullptr;
	}

	GameObject* equippedItemObject = nullptr;
	switch (m_CombatMode)
	{
	case CombatMode::Melee:
		equippedItemObject = m_MeleeItem;
		break;
	case CombatMode::Throw:
		equippedItemObject = throwItemObject;
		break;
	case CombatMode::Idle:
	default:
		equippedItemObject = nullptr;
		break;
	}

	auto setItemVisible = [](GameObject* itemObject, bool visible)
		{
			if (!itemObject)
			{
				return;
			}

			auto* renderer = itemObject->GetComponent<MeshRenderer>();
			if (renderer)
			{
				renderer->SetVisible(visible);
			}
		};
	setItemVisible(m_MeleeItem, equippedItemObject == m_MeleeItem && equippedItemObject != nullptr);
	for (auto* consumableObject : consumableObjects)
	{
		setItemVisible(consumableObject, equippedItemObject == consumableObject && equippedItemObject != nullptr);
	}

	if (equippedItemObject == nullptr)
	{
		return;
	}
	auto* itemcomponent = equippedItemObject->GetComponent<ItemComponent>();
	if (!itemcomponent)
	{
		return;
	}

	auto* skeletal = owner->GetComponent<SkeletalMeshComponent>();
	if (!skeletal)
	{
		return;
	}

	auto* loader = AssetLoader::GetActive();
	if (!loader)
	{
		return;
	}

	const SkeletonHandle skeletonHandle = skeletal->GetSkeletonHandle();
	if (!skeletonHandle.IsValid())
	{
		return;
	}

	RenderData::Skeleton* skeleton = loader->GetSkeletons().Get(skeletonHandle);
	if (!skeleton)
	{
		return;
	}

	XMFLOAT4X4 equipmentPose = skeleton->equipmentBindPose;
	const int equipmentBoneIndex = skeleton->equipmentBoneIndex;
	if (equipmentBoneIndex >= 0)
	{
		const auto* animComp = owner->GetComponent<SkinningAnimationComponent>();
		if (animComp)
		{

			const auto& globalPose = animComp->GetGlobalPose();
			if (static_cast<size_t>(equipmentBoneIndex) < globalPose.size())
			{
				equipmentPose = globalPose[static_cast<size_t>(equipmentBoneIndex)];
			}
		}
	}

	// equipment 본 포즈 로드
	XMMATRIX equipmentM = XMLoadFloat4x4(&equipmentPose);

	// 스케일 적용 (회전 보존)
	XMMATRIX scaleM = XMMatrixScaling(0.01f, 0.01f, 0.01f);
	equipmentM = XMMatrixMultiply(scaleM, equipmentM);

	// 플레이어 월드 행렬
	auto* playerTransform = owner->GetComponent<TransformComponent>();
	if (!playerTransform)
	{
		return;
	}

	XMMATRIX playerWorldM = XMLoadFloat4x4(&playerTransform->GetWorldMatrix());
	XMMATRIX finalM = XMMatrixMultiply(equipmentM, playerWorldM);

	XMFLOAT4X4 finalPose;
	XMStoreFloat4x4(&finalPose, finalM);

	itemcomponent->SetEquipmentBindPose(finalPose);
}

void PlayerComponent::OnEvent(EventType type, const void* data)
{
	if (type == EventType::MouseLeftClick)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;

		// 전투 Input
		if (gameManager && gameManager->IsCombatInputAllowed())
		{
			if (!scene || !scene->GetServices().Has<InputManager>())
			{
				return;
			}

			auto& input = scene->GetServices().Get<InputManager>();
			if (!input.IsPointInViewport(mouseData->pos))
			{
				return;
			}

			auto camera = scene->GetGameCamera();
			if (!camera)
			{
				return;
			}

			Ray pickRay{};
			if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
			{
				return;
			}

			float hitT = 0.0f;
			auto* clickedNode = FindClosestNodeHit(scene, pickRay, hitT);
			if (!clickedNode)
			{
				return;
			}

			auto* enemy = FindEnemyAt(m_GridSystem, clickedNode->GetQ(), clickedNode->GetR());
			if (!enemy)
			{
				const bool isSelfTile = (clickedNode->GetQ() == m_Q) && (clickedNode->GetR() == m_R);
				if (isSelfTile && m_CombatMode == CombatMode::Throw)
				{
					if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
					{
						if (combatFsm->TryExecutePlayerSelfThrow())
						{
							mouseData->handled = true;
						}
					}
				}
				return;
			}

			if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr) 
			{
				if (m_CombatMode == CombatMode::Throw)
				{
					if (combatFsm->TryExecutePlayerThrowAttack(enemy))
					{
						mouseData->handled = true;
					}
					return;
				}

				const int range = max(0, m_AttackRange);
				const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
				if (distance > range)
				{
					return;
				}

				m_PendingAttackTarget = enemy;
				if (combatFsm->TryExecutePlayerAttackFromInput())
				{
					mouseData->handled = true;
				}
			}
			return;
		}

		// 이동 Input

		if (!gameManager || !gameManager->IsExplorationInputAllowed())
		{
			return;
		}

		if (!scene || !scene->GetServices().Has<InputManager>())
		{
			return;
		}

		auto& input = scene->GetServices().Get<InputManager>();
		if (!input.IsPointInViewport(mouseData->pos))
		{
			return;
		}

		auto camera = scene->GetGameCamera();
		if (!camera)
		{
			return;
		}

		Ray pickRay{};
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
		{
			return;
		}

		//아이템 줍기
		float hitT = 0.0f;
		if (auto* clickedItem = FindClosestItemHit(scene, pickRay, hitT))
		{
			if (TryPickup(clickedItem))
			{
				scene->GetServices().Get<SoundManager>().SFX_Shot(L"GetItem_Player");

				cout << "PickUp" << endl;
				mouseData->handled = true;
				return;
			}
		}
		hitT = 0.0f;


		BeginThrowPreview();




		auto* clickedNode = FindClosestNodeHit(scene, pickRay, hitT);
		if (!clickedNode)
		{
			return;
		}

		std::cout << clickedNode->GetQ() << ", " << clickedNode->GetR() << std::endl; // 클릭된 Node Debug

		if (!clickedNode->GetIsMoveable())
		{

			if (auto* door = clickedNode->GetLinkedDoor())
			{
				//std::cout << "Door Evenet Detected" << std::endl;
				// 인접 노드 판별
				const int distanceToDoor = AxialDistance(m_Q, m_R, clickedNode->GetQ(), clickedNode->GetR());
				if (distanceToDoor > 1)
				{
					return;
				}
				m_PendingDoor = door;
				DispatchPlayerStateEvent(owner, "Door_Interact");

				mouseData->handled = true;
				return;
			}
		}

		auto* enemy = FindEnemyAt(m_GridSystem, clickedNode->GetQ(), clickedNode->GetR());
		if (!enemy)
		{
			const bool isSelfTile = (clickedNode->GetQ() == m_Q) && (clickedNode->GetR() == m_R);
			if (isSelfTile && m_CombatMode == CombatMode::Throw) 
			{
				if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
				{
					if (combatFsm->TryExecutePlayerSelfThrow())
					{
						cout << "Self Throw" << endl;
						mouseData->handled = true;
					}
				}
			}
			return;
		}


		//던지기
		const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
		if (m_CombatMode == CombatMode::Throw) 
		{
			if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
			{
				if (combatFsm->TryExecutePlayerThrowAttack(enemy))
				{
					mouseData->handled = true;
					return;
				}
			}
		}

		const int range = max(0, m_AttackRange);
		if (distance > range)
		{
			return;
		}

		if (m_CombatMode != CombatMode::Melee) 
		{
			std::cout << "MeleeMode\n";
			m_CombatMode = CombatMode::Melee;
		}
		else
		{
			if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
			{
				combatFsm->RequestCombatEnter(GetActorId(), enemy->GetActorId());
			}
		}


		return;
	}

	if (type == EventType::MouseLeftDoubleClick)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;

		if (gameManager && gameManager->IsCombatInputAllowed())
		{
			if (!scene || !scene->GetServices().Has<InputManager>())
			{
				return;
			}

			auto& input = scene->GetServices().Get<InputManager>();
			if (!input.IsPointInViewport(mouseData->pos))
			{
				return;
			}

			auto camera = scene->GetGameCamera();
			if (!camera)
			{
				return;
			}

			Ray pickRay{};
			if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
			{
				return;
			}

			float hitT = 0.0f;
			auto* clickedNode = FindClosestNodeHit(scene, pickRay, hitT);
			if (!clickedNode)
			{
				return;
			}

			auto* enemy = FindEnemyAt(m_GridSystem, clickedNode->GetQ(), clickedNode->GetR());
			if (!enemy)
			{
				const bool isSelfTile = (clickedNode->GetQ() == m_Q) && (clickedNode->GetR() == m_R);
				if (isSelfTile && m_CombatMode == CombatMode::Throw)
				{
					if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
					{
						if (combatFsm->TryExecutePlayerSelfThrow())
						{
							mouseData->handled = true;
						}
					}
				}
				return;
			}

			if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
			{
				if (m_CombatMode == CombatMode::Throw)
				{
					if (combatFsm->TryExecutePlayerThrowAttack(enemy))
					{
						mouseData->handled = true;
					}
					return;
				}

				const int range = max(0, m_AttackRange);
				const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
				if (distance > range)
				{
					return;
				}

				m_PendingAttackTarget = enemy;
				if (combatFsm->TryExecutePlayerAttackFromInput())
				{
					mouseData->handled = true;
				}
			}
			return;
		}

		if (!gameManager || !gameManager->IsExplorationInputAllowed())
		{
			return;
		}

		if (!scene || !scene->GetServices().Has<InputManager>())
		{
			return;
		}

		auto& input = scene->GetServices().Get<InputManager>();
		if (!input.IsPointInViewport(mouseData->pos))
		{
			return;
		}

		auto camera = scene->GetGameCamera();
		if (!camera)
		{
			return;
		}

		Ray pickRay{};
		if (!input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
		{
			return;
		}

		float hitT = 0.0f;
		auto* clickedNode = FindClosestNodeHit(scene, pickRay, hitT);
		if (!clickedNode)
		{
			return;
		}

		auto* enemy = FindEnemyAt(m_GridSystem, clickedNode->GetQ(), clickedNode->GetR());
		if (!enemy)
		{
			return;
		}

		const int range = max(0, m_AttackRange);
		const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
		if (distance > range)
		{
			return;
		}

		m_CombatMode = CombatMode::Melee; 
		{
			std::cout << "MeleeMode\n";
			m_CombatMode = CombatMode::Melee;
		}

		if (auto* combatFsm = owner ? owner->GetComponent<PlayerCombatFSMComponent>() : nullptr)
		{
			combatFsm->RequestCombatEnter(GetActorId(), enemy->GetActorId());
		}

		return;
	}

	if (type == EventType::Hovered)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		if (!scene)
		{
			return;
		}

		auto& services = scene->GetServices();
		if (!services.Has<UIManager>() || !services.Has<InputManager>() || !services.Has<AssetLoader>())
		{
			return;
		}

		auto& uiManager = services.Get<UIManager>();
		auto& input = services.Get<InputManager>();
		auto& assetLoader = services.Get<AssetLoader>();

		TextureHandle hoverInfo = TextureHandle::Invalid();
		TextureHandle hoverEnemyTexture = TextureHandle::Invalid();
		float hoverEnemyHpPercent = 0.0f;
		bool hasHoveredEnemy = false;
		int hoveredEnemyActorId = 0;
		int hoveredEnemyCurrentHp = 0;
		int hoveredEnemyMaxHp = 0;
		if (input.IsPointInViewport(mouseData->pos))
		{
			auto camera = scene->GetGameCamera();
			if (camera)
			{
				Ray pickRay{};
				if (input.BuildPickRay(camera->GetViewMatrix(), camera->GetProjMatrix(), *mouseData, pickRay))
				{
					float hitT = 0.0f;
					if (auto* hoveredItem = FindClosestItemHit(scene, pickRay, hitT))
					{
						hoverInfo = ResolveInfoTextureFromItemObject(scene, dynamic_cast<GameObject*>(hoveredItem->GetOwner()), assetLoader);
					}
				}

				float enemyHitT = 0.0f;
				if (auto* hoveredEnemy = FindClosestEnemyHit(scene, pickRay, enemyHitT))
				{
					auto* enemyOwner = dynamic_cast<GameObject*>(hoveredEnemy->GetOwner());
					auto* enemyStat = enemyOwner ? enemyOwner->GetComponent<EnemyStatComponent>() : nullptr;
					hoverEnemyTexture = ResolveEnemyHoverTexture(enemyOwner);
					hoveredEnemyActorId = hoveredEnemy->GetActorId();
					if (enemyStat)
					{
						hoveredEnemyCurrentHp = enemyStat->GetCurrentHP();
						hoveredEnemyMaxHp = (std::max)(1, enemyStat->GetInitialHP());
						const float currentHp = static_cast<float>(hoveredEnemyCurrentHp);
						const float maxHp = static_cast<float>(hoveredEnemyMaxHp); 
						hoverEnemyHpPercent = currentHp / maxHp;
					}
					hasHoveredEnemy = true;
				}
			}
		}

		Events::EnemyHoveredEvent enemyHoveredEvent{};
		enemyHoveredEvent.actorId = hasHoveredEnemy ? hoveredEnemyActorId : 0;
		enemyHoveredEvent.currentHp = hasHoveredEnemy ? hoveredEnemyCurrentHp : 0;
		enemyHoveredEvent.maxHp = hasHoveredEnemy ? hoveredEnemyMaxHp : 0;
		enemyHoveredEvent.hasEnemy = hasHoveredEnemy;
		GetEventDispatcher().Dispatch(EventType::EnemyHovered, &enemyHoveredEvent);

		UpdateGroundItemInfoPanel(uiManager, scene->GetName(), hoverInfo);
		UpdateEnemyHoverInfoPanel(uiManager, scene->GetName(), hoverEnemyTexture, hoverEnemyHpPercent, hasHoveredEnemy);
		return;
	}


	if (type == EventType::MouseRightClick)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;

		if (!gameManager || gameManager->GetCombatManager()->GetState() != Battle::InBattle) 
		{
			std::cout << "Cancel Throw Preview\n";
			EndThrowPreview();
		}
	}

	if (type == EventType::MouseRightClickHold)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		auto* owner = GetOwner();
		auto* scene = owner ? owner->GetScene() : nullptr;
		auto* gameManager = scene ? scene->GetGameManager() : nullptr;
		if (gameManager && !gameManager->IsExplorationInputAllowed())
		{
			return;
		}


		return;
	}

	if (type == EventType::MouseRightClickUp)
	{
		const auto* mouseData = static_cast<const Events::MouseState*>(data);
		if (!mouseData || mouseData->handled)
		{
			return;
		}

		EndThrowPreview();
		return;
	}

	if (type != EventType::TurnChanged || !data)
	{
		return;
	}

	const auto* payload = static_cast<const Events::TurnChanged*>(data);
	if (!payload)
	{
		return;
	}

	m_CurrentTurn = static_cast<Turn>(payload->turn);
	m_TurnEndRequested = false;
	if (m_CurrentTurn == Turn::PlayerTurn)
	{
		ResetTurnResources();
		BeginThrowPreview();
	}
	else
	{
		EndThrowPreview();
	}
}

// 행동,이동력 초기화 // turn 초기화
void PlayerComponent::ResetTurnResources()
{
	m_RemainMoveResource = m_MoveResource;
	m_RemainActResource = m_ActResource;
	m_HasMoveStart = false;
	m_CombatConfirmRequested = false;
	m_SelectedEnemy = nullptr;
	EndThrowPreview();
	ResetSubFSMFlags();
}

// 움직임
void PlayerComponent::BeginMove()
{
	m_StartQ = m_Q;
	m_StartR = m_R;
	m_HasMoveStart = true;
}

bool PlayerComponent::CommitMove(int targetQ, int targetR)
{
	if (m_CurrentTurn != Turn::PlayerTurn)
	{
		return false;
	}

	const int startQ = m_HasMoveStart ? m_StartQ : m_Q;
	const int startR = m_HasMoveStart ? m_StartR : m_R;
	//
	int cost = -1;
	if (m_GridSystem) {
		cost = m_GridSystem->GetShortestPathLength({ startQ,startR }, { targetQ,targetR });
	}
	if (cost < 0) {
		cost = AxialDistance(startQ, startR, targetQ, targetR); // start와 Target 간의 소모 cost return
	}
	m_HasMoveStart = false;
	if (cost <= 0)
	{
		return true;
	}
	if (cost > m_RemainMoveResource) //남은 MoveResource 보다 크면 Commit X 
	{
		return false;
	}

	m_RemainMoveResource -= cost; //반영
	return true;

}

bool PlayerComponent::ConsumeActResource(int amount)
{
	if (m_CurrentTurn != Turn::PlayerTurn)
	{
		return false;
	}
	if (amount <= 0)
	{
		return true;
	}
	if (amount > m_RemainActResource)
	{
		return false;
	}
	cout << "use cost" << amount << endl;
	m_RemainActResource -= amount;
	return true;
}

void PlayerComponent::RequestCombatConfirm()
{
	m_CombatConfirmRequested = true;
}

bool PlayerComponent::HandleCombatClick(EnemyComponent* enemy)
{
	if (!enemy)
	{
		ClearCombatSelection();
		return false;
	}

	if (m_CurrentTurn != Turn::PlayerTurn)
	{
		return false;
	}

	const int distance = AxialDistance(m_Q, m_R, enemy->GetQ(), enemy->GetR());
	if (distance > 1)
	{
		return false;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	if (m_SelectedEnemy == enemy)
	{
		m_PendingAttackTarget = enemy;
		RequestCombatConfirm();
		DispatchCombatEvent(owner, "Combat_Confirm");
		m_SelectedEnemy = nullptr;
		return true;
	}

	m_SelectedEnemy = enemy;
	m_PendingAttackTarget = enemy;
	if (auto* combatFsm = owner->GetComponent<PlayerCombatFSMComponent>())
	{
		combatFsm->RequestCombatEnter(GetActorId(), enemy->GetActorId());
	}
	return true;
}

void PlayerComponent::ClearCombatSelection()
{
	m_SelectedEnemy = nullptr;
	m_PendingAttackTarget = nullptr;
}

EnemyComponent* PlayerComponent::ResolveCombatTarget(GameObject* obj) const
{
	if (!obj) return nullptr;
	if (auto* enemy = obj->GetComponent<EnemyComponent>()) return enemy;
	if (auto* node = obj->GetComponent<NodeComponent>())
	{
		if (m_GridSystem)
			return m_GridSystem->GetEnemyAt(node->GetQ(), node->GetR());
	}
	return nullptr;
}

EnemyComponent* PlayerComponent::ConsumePendingAttackTarget()
{
	EnemyComponent* target = m_PendingAttackTarget;
	m_PendingAttackTarget = nullptr;
	return target;
}


// 밀기 관련

// 조건 체크
bool PlayerComponent::TryFindPushTarget(EnemyComponent*& outEnemy, NodeComponent*& outNode) const
{
	outEnemy = nullptr;
	outNode = nullptr;
	if (!m_GridSystem)
	{
		return false;
	}

	const int playerQ = m_Q;
	const int playerR = m_R;
	const auto& enemies = m_GridSystem->GetEnemies();

	for (auto* enemy : enemies) {
		if (!enemy) { continue; }

		const int distance = AxialDistance(playerQ, playerR, enemy->GetQ(), enemy->GetR());

		if (distance != 1) { continue; }

		auto* enemyOwner = enemy->GetOwner();

		if (!enemyOwner) { continue; }

		if (auto* enemyStat = enemyOwner->GetComponent<EnemyStatComponent>())
		{
			if (enemyStat->IsDead()) { continue; }
		}

		const int dq = enemy->GetQ() - playerQ;
		const int dr = enemy->GetR() - playerR;
		const AxialKey targetKey{ enemy->GetQ() + dq ,enemy->GetR() + dr };
		auto* targetNode = m_GridSystem->GetNodeByKey(targetKey);

		if (!targetNode) { continue; }

		auto* targetOwner = targetNode->GetOwner();
		if (!targetOwner || !targetOwner->GetComponent<PushNodeComponent>()) { continue; }
		// if (targetNode->GetState() != NodeState::Empty) { continue; } 
		outEnemy = enemy;
		outNode = targetNode;
		return true; // 밀기 세팅 완료
	}

	return false;
}

// 밀기 동작
bool PlayerComponent::ResolvePushTarget(EnemyComponent* enemy, NodeComponent* targetNode)
{
	if (!enemy || !targetNode || !m_GridSystem) { return false; }

	auto* enemyOwner = enemy->GetOwner();
	if (!enemyOwner) { return false; }

	auto* enemyTransform = enemyOwner->GetComponent<TransformComponent>();
	auto* targetOwner = targetNode->GetOwner();
	auto* targetTransform = targetOwner ? targetOwner->GetComponent<TransformComponent>() : nullptr;


	if (!enemyTransform || !targetTransform) { return false; }

	enemyTransform->SetPosition(targetTransform->GetPosition());
	enemy->SetQR(targetNode->GetQ(), targetNode->GetR());

	if (auto* enemyStat = enemyOwner->GetComponent<EnemyStatComponent>())
	{
		enemyStat->SetCurrentHP(0);
	}

	return true;
}

void PlayerComponent::ClearPendingPush()
{
	m_PendingPushEnemy = nullptr;
	m_PendingPushNode = nullptr;
}

bool PlayerComponent::ConsumeCombatConfirmRequest()
{
	if (!m_CombatConfirmRequested)
	{
		return false;
	}

	m_CombatConfirmRequested = false;
	return true;
}

bool PlayerComponent::ConsumePushPossible()
{
	//return ConsumeFlag(m_PushPossible);
	EnemyComponent* enemy = nullptr;
	NodeComponent* targetNode = nullptr;
	const bool possible = TryFindPushTarget(enemy, targetNode);
	if (possible)
	{
		m_PendingPushEnemy = enemy;
		m_PendingPushNode = targetNode;
	}
	else
	{
		ClearPendingPush();
	}
	return possible;
}


bool PlayerComponent::ConsumePushTargetFound()
{
	//return ConsumeFlag(m_PushTargetFound);
	if (m_PendingPushEnemy && m_PendingPushNode)
	{
		return true;
	}

	EnemyComponent* enemy = nullptr;
	NodeComponent* targetNode = nullptr;
	const bool found = TryFindPushTarget(enemy, targetNode);
	if (found)
	{
		m_PendingPushEnemy = enemy;
		m_PendingPushNode = targetNode;
		return true;
	}

	ClearPendingPush();
	return false;
}

bool PlayerComponent::ConsumePushSuccess()
{
	//return ConsumeFlag(m_PushSuccess);
	const bool diceSuccess = ConsumeFlag(m_PushSuccess);
	if (!diceSuccess)
	{
		ClearPendingPush();
		return false;
	}

	if (!m_PendingPushEnemy || !m_PendingPushNode)
	{
		EnemyComponent* enemy = nullptr;
		NodeComponent* targetNode = nullptr;
		if (!TryFindPushTarget(enemy, targetNode))
		{
			ClearPendingPush();
			return false;
		}
		m_PendingPushEnemy = enemy;
		m_PendingPushNode = targetNode;
	}

	const bool success = ResolvePushTarget(m_PendingPushEnemy, m_PendingPushNode);
	ClearPendingPush();
	return success;
}

bool PlayerComponent::ConsumeDoorConfirmed()
{
	return ConsumeFlag(m_DoorConfirmed);
}

bool PlayerComponent::ConsumeDoorSuccess()
{
	return ConsumeFlag(m_DoorSuccess);
}

bool PlayerComponent::ConsumeInventoryAtShop()
{
	return ConsumeFlag(m_InventoryAtShop);
}

bool PlayerComponent::ConsumeInventoryCanDrop()
{
	return ConsumeFlag(m_InventoryCanDrop);
}

bool PlayerComponent::ConsumeShopHasSpace()
{
	return ConsumeFlag(m_ShopHasSpace);
}

bool PlayerComponent::ConsumeShopHasMoney()
{
	return ConsumeFlag(m_ShopHasMoney);
}

void PlayerComponent::SetPendingDoor(DoorComponent* door)
{
	m_PendingDoor = door;
}

DoorComponent* PlayerComponent::ConsumePendingDoor()
{
	DoorComponent* door = m_PendingDoor;
	m_PendingDoor = nullptr;
	return door;
}

void PlayerComponent::ResetSubFSMFlags()
{
	m_PushPossible = true;
	m_PushTargetFound = true;
	m_PushSuccess = true;
	m_DoorConfirmed = true;
	m_DoorSuccess = true;
	m_InventoryAtShop = true;
	m_InventoryCanDrop = true;
	m_ShopHasSpace = true;
	m_ShopHasMoney = true;

	m_PendingDoor = nullptr;
	ClearPendingPush();
}

bool PlayerComponent::ConsumeFlag(bool& flag)
{
	const bool value = flag;
	flag = true;
	return value;
}

bool PlayerComponent::TryGetConsumableThrowRange(int& outRange) const
{
	ItemComponent* throwItem = nullptr;
	if (!TryGetConsumableThrowItem(throwItem) || !throwItem)
	{
		return false;
	}

	const int throwRange = throwItem->GetThrowRange();
	if (throwRange <= 0)
	{
		return false;
	}

	outRange = throwRange;
	return true;
}

bool PlayerComponent::TryGetConsumableThrowItemBySlot(int slotIndex, ItemComponent*& outItem) const
{
	outItem = nullptr;
	if (slotIndex < 0 || slotIndex >= 3)
	{
		return false;
	}

	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	const auto& itemName = m_ConsumableItemNames[slotIndex];
	auto* itemObject = FindGameObjectByName(scene, itemName);
	if (!itemObject) 
	{
		return false;
	}

	auto* itemComponent = itemObject->GetComponent<ItemComponent>();
	if (!itemComponent)
	{
		return false;
	}

	const int itemType = itemComponent->GetType();
	if (itemType != static_cast<int>(ItemType::THROW)
		&& itemType != static_cast<int>(ItemType::HEAL))
	{
		return false;
	}

	

	if (itemComponent->GetThrowRange() <= 0) 
	{
		return false;
	}

	outItem = itemComponent;
	return true;
}

bool PlayerComponent::TryGetConsumableThrowItem(ItemComponent*& outItem) const
{
	if (TryGetConsumableThrowItemBySlot(m_SelectedConsumableSlot, outItem))
	{
		return true;
	}

	for (int i = 0; i < 3; ++i)
	{
		if (i == m_SelectedConsumableSlot)
		{
			continue;
		}

		if (TryGetConsumableThrowItemBySlot(i, outItem))
		{
			return true;
		}
	}

	outItem = nullptr;
	return false;
}

bool PlayerComponent::TryGetEquippedMeleeItem(ItemComponent*& outItem) const
{
	outItem = nullptr;
	if (!m_MeleeItem)
	{
		return false;
	}

	auto* meleeItemComponent = m_MeleeItem->GetComponent<ItemComponent>();
	if (!meleeItemComponent || !meleeItemComponent->GetIsEquiped())
	{
		return false;
	}

	outItem = meleeItemComponent;
	return true;
}

void PlayerComponent::SelectConsumableThrowSlot(int slotIndex)
{
	if (slotIndex < 0 || slotIndex >= 3)
	{
		return;
	}

	m_SelectedConsumableSlot = slotIndex;
}


void PlayerComponent::ConsumeThrowItem(ItemComponent* throwItem)
{
	if (!throwItem)
	{
		return;
	}

	auto* itemOwner = throwItem->GetOwner();
	const std::string itemName = itemOwner ? itemOwner->GetName() : std::string{};
	int consumedSlot = -1;
	for (int i = 0; i < 3; ++i) 
	{
		if (!m_ConsumableItemNames[i].empty() && m_ConsumableItemNames[i] == itemName) 
		{
			consumedSlot = i;
			break;
		}
	}
	if (consumedSlot >= 0)
	{
		for (int i = consumedSlot; i < 2; ++i)
		{
			m_ConsumableItemNames[i] = std::move(m_ConsumableItemNames[i + 1]);
		}
		m_ConsumableItemNames[2].clear();
	}


	if (itemOwner)
	{
		const std::string& itemName = itemOwner->GetName();
		auto it = std::remove(m_InventoryItemIds.begin(), m_InventoryItemIds.end(), itemName);
		if (it != m_InventoryItemIds.end())
		{
			m_InventoryItemIds.erase(it, m_InventoryItemIds.end());
		}
	}

	throwItem->SetIsEquiped(false);
	ConsumeActResource(throwItem->GetActionPointCost());

	m_CombatMode = ResolveBaseCombatMode();
	m_ThrowPreviewRange = 0;
	if (m_GridSystem) 
	{
		m_GridSystem->SetThrowRangePreview(false, 0);
	}

	UpdateInventorySlotUI();
}

void PlayerComponent::BeginThrowPreview()
{
	if (m_CombatMode != CombatMode::Throw) 
	{
		return;
	}

	if (!m_GridSystem)
	{
		return;
	}

	int range = 0;
	if (!TryGetConsumableThrowRange(range))
	{
		return;
	}

	m_ThrowPreviewRange = range;
	m_GridSystem->SetThrowRangePreview(true, range);
}

void PlayerComponent::EndThrowPreview()
{
	if (m_CombatMode != CombatMode::Throw) 
	{
		return;
	}

	m_CombatMode = ResolveBaseCombatMode();
	m_ThrowPreviewRange = 0;
	if (m_GridSystem)
	{
		m_GridSystem->SetThrowRangePreview(false, 0);
	}
}

PlayerComponent::CombatMode PlayerComponent::ResolveBaseCombatMode() const
{
	if (!m_MeleeItem)
	{
		return CombatMode::Idle;
	}

	auto* meleeItemComponent = m_MeleeItem->GetComponent<ItemComponent>();
	if (!meleeItemComponent || !meleeItemComponent->GetIsEquiped())
	{
		return CombatMode::Idle;
	}

	return CombatMode::Melee;
}

void PlayerComponent::SyncCombatModeFromInventory()
{
	if (m_CombatMode == CombatMode::Throw)
	{
		return;
	}

	m_CombatMode = ResolveBaseCombatMode();
}

void PlayerComponent::UpdateResourceUI()
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return;
	}

	auto& services = scene->GetServices();
	if (!services.Has<UIManager>())
	{
		return;
	}

	auto& uiManager = services.Get<UIManager>();
	const std::string sceneName = scene->GetName();

	auto updateProgress = [&](const std::string& uiName, int remain, int maxValue)
		{
			auto uiObject = uiManager.FindUIObject(sceneName, uiName);
			if (!uiObject)
			{
				return;
			}

			if (!uiObject->GetScene())
			{
				uiObject->SetScene(scene);
			}

			auto* progress = uiObject->GetComponent<UIProgressBarComponent>();
			if (!progress)
			{
				return;
			}

			const float percent = (maxValue > 0)
				? (static_cast<float>(remain) / static_cast<float>(maxValue))
				: 0.0f;
			progress->SetPercent(percent);
		};

	updateProgress("ActionPoint", m_RemainActResource, m_ActResource);
	updateProgress("MovePoint", m_RemainMoveResource, m_MoveResource);

	m_LastRemainMoveResource = m_RemainMoveResource;
	m_LastRemainActResource = m_RemainActResource;
	m_LastMoveResource = m_MoveResource;
	m_LastActResource = m_ActResource;
}

void PlayerComponent::UpdateInventorySlotUI()
{
	auto* owner = GetOwner();
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene)
	{
		return;
	}

	auto& services = scene->GetServices();
	if (!services.Has<UIManager>() || !services.Has<AssetLoader>())
	{
		return;
	}

	auto& uiManager = services.Get<UIManager>();
	auto& assetLoader = services.Get<AssetLoader>();
	const std::string sceneName = scene->GetName();

	const TextureHandle meleeIcon = ResolveIconTextureFromItemObject(m_MeleeItem, assetLoader);
	const TextureHandle meleeInfo = ResolveInfoTextureFromItemObject(scene, m_MeleeItem, assetLoader);
	UpdateSlotIcon(uiManager, sceneName, GetMeleeSlotIconNameCandidates(), meleeIcon);
	UpdateInfoPanelTexture(uiManager, sceneName, GetMeleeInfoPanelNameCandidates(), meleeInfo);

	for (int i = 0; i < 3; ++i)
	{
		// 인벤토리 인덱스 0/1/2 -> UI의 1/2/3번 슬롯
		auto* itemObject = FindGameObjectByName(scene, m_ConsumableItemNames[i]);
		const TextureHandle throwIcon = ResolveIconTextureFromItemObject(itemObject, assetLoader);
		const TextureHandle throwInfo = ResolveInfoTextureFromItemObject(scene, itemObject, assetLoader);
		const auto& slotNames = GetThrowSlotIconNameCandidates(i);
		if (!slotNames.empty())
		{
			UpdateSlotIcon(uiManager, sceneName, slotNames, throwIcon);
		}

		const auto& infoPanelNames = GetThrowInfoPanelNameCandidates(i);
		if (!infoPanelNames.empty())
		{
			UpdateInfoPanelTexture(uiManager, sceneName, infoPanelNames, throwInfo);
		}
	}
}

void PlayerComponent::HandleCombatModeButtonState(const std::string& buttonEventName)
{
	if (buttonEventName == "Player_Melee")
	{
		cout << "Melee" << endl;
		if (m_CombatMode == CombatMode::Throw)
		{
			EndThrowPreview();
		}
		else
		{
			m_CombatMode = ResolveBaseCombatMode();
		}
		return;
	}

	int throwSlot = -1;
	if (buttonEventName == "Player_Throw1" || buttonEventName == "Player_Throw_1")
	{
		cout << "Player_Throw1" << endl;

		throwSlot = 0;
	}
	else if (buttonEventName == "Player_Throw2" || buttonEventName == "Player_Throw_2")
	{
		cout << "Player_Throw2" << endl;

		throwSlot = 1;
	}
	else if (buttonEventName == "Player_Throw3" || buttonEventName == "Player_Throw_3")
	{
		cout << "Player_Throw3" << endl;

		throwSlot = 2;
	}
	else
	{
		return;
	}

	SelectConsumableThrowSlot(throwSlot);
	int throwRange = 0;
	if (TryGetConsumableThrowRange(throwRange) && throwRange > 0)
	{
		m_CombatMode = CombatMode::Throw;
		BeginThrowPreview();
	}
	else
	{
		if (m_CombatMode == CombatMode::Throw && m_GridSystem)
		{
			m_ThrowPreviewRange = 0;
			m_GridSystem->SetThrowRangePreview(false, 0);
		}
		m_CombatMode = ResolveBaseCombatMode();

	}
}

void PlayerComponent::ApplyVisualPresetByCombatMode()
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* visualPreset = owner->GetComponent<PlayerVisualPresetComponent>();
	if (!visualPreset)
	{
		return;
	}

	auto tryApplyAny = [visualPreset](std::initializer_list<const char*> stateTags)
		{
			for (const char* tag : stateTags)
			{
				if (tag && visualPreset->ApplyByStateTag(tag))
				{
					return true;
				}
			}

			return false;
		};

	auto* playerStat = owner->GetComponent<PlayerStatComponent>();
	const bool isDead = playerStat && playerStat->IsDead();
	if (isDead)
	{
		switch (m_CombatMode)
		{
		case CombatMode::Melee:
			if (!tryApplyAny({ "MeleeDead" }))
			{
				tryApplyAny({ "MeleeDead" });
			}
			return;
		case CombatMode::Throw:
			if (!tryApplyAny({ "ThrowDead" }))
			{
				tryApplyAny({ "ThrowDead" });
			}
			return;
		default:
			tryApplyAny({ "Dead", "IdleDead" });
			break;
		}

	}
	switch (m_CombatMode)
	{
	case CombatMode::Melee:
		tryApplyAny({ "Melee", "Meele" });
		break;
	case CombatMode::Throw:
		tryApplyAny({ "Throw" });
		break;
	default:
		break;
	}
}

void PlayerComponent::ApplyAnimation()
{
	ApplyVisualPresetByCombatMode();
}

bool PlayerComponent::TryPickup(ItemComponent* item)
{
	if (!item)
	{
		return false;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	auto* itemOwner = item->GetOwner();
	auto* itemObject = itemOwner ? dynamic_cast<GameObject*>(itemOwner) : nullptr;
	if (!itemObject)
	{
		return false;
	}

	//획득 반경
	constexpr float kPickupRadius = 1.5f;
	auto* itemTransform = itemObject->GetComponent<TransformComponent>();
	auto* playerTransform = owner->GetComponent<TransformComponent>();
	if (itemTransform && playerTransform)
	{
		const float distSq = DistanceSq2D(playerTransform->GetWorldPos(), itemTransform->GetWorldPos());
		if (distSq > kPickupRadius * kPickupRadius)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerEquipFailed, item);
			return false;
		}
	}

	const int itemType = item->GetType();
	int consumableSlot = -1;
	if (itemType == static_cast<int>(ItemType::EQUIPMENT))
	{
		if (m_MeleeItem)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerEquipFailed, item);
			return false;
		}
	}
	else if (itemType == static_cast<int>(ItemType::HEAL) || itemType == static_cast<int>(ItemType::THROW))
	{
		for (int i = 0; i < 3; ++i)
		{
			if (m_ConsumableItemNames[i].empty())
			{
				consumableSlot = i;
				break;
			}
		}

		if (consumableSlot < 0)
		{
			GetEventDispatcher().Dispatch(EventType::PlayerEquipFailed, item);
			return false;
		}
	}

	if (!item->RequestPickup(owner))
	{
		return false;
	}

	const bool isGoldBar = (item->GetItemIndex() == 1);
	if (!isGoldBar)
	{
		AddToInventory(item);
	}
	auto* scene = owner->GetScene();
	bool shouldRemovePickedObject = false;
	if (isGoldBar)
	{
		const int gainedGold = max(0, item->GetPrice());
		m_Money += gainedGold;
		const Events::GoldAcquiredEvent goldEvent{ gainedGold, m_Money };
		GetEventDispatcher().Dispatch(EventType::GoldAcquired, &goldEvent);
		shouldRemovePickedObject = true;
	}
	if (itemType == static_cast<int>(ItemType::EQUIPMENT)
		|| itemType == static_cast<int>(ItemType::HEAL)
		|| itemType == static_cast<int>(ItemType::THROW)) 
	{
		auto* scene = owner->GetScene();
		GameObject* equippedObject = nullptr;
		if (scene)
		{
			auto& services = scene->GetServices();
			if (services.Has<GameDataRepository>())
			{
				const auto* definition = services.Get<GameDataRepository>().GetItem(item->GetItemIndex());
				if (definition)
				{
					equippedObject = SpawnEquippedItem(*scene, *definition);
				}
			}
		}

		if (equippedObject)
		{
			shouldRemovePickedObject = true;
			if (itemType == static_cast<int>(ItemType::EQUIPMENT))
			{
				m_MeleeItem = equippedObject;
				m_IsApplyMeleeStat = false;
				SyncCombatModeFromInventory();
			}
			else if (consumableSlot >= 0)
			{
				m_ConsumableItemNames[consumableSlot] = equippedObject->GetName();
			}

			auto& inventoryName = m_InventoryItemIds.back();
			inventoryName = equippedObject->GetName();

			if (auto* renderer = itemObject->GetComponent<MeshRenderer>())
			{
				renderer->SetVisible(false);
				renderer->SetRenderLayer(static_cast<UINT8>(RenderData::RenderLayer::None));
			}
		}
		else
		{
			if (itemType == static_cast<int>(ItemType::EQUIPMENT))
			{
				m_MeleeItem = itemObject;
				SyncCombatModeFromInventory();
			}
			else if (consumableSlot >= 0)
			{
				m_ConsumableItemNames[consumableSlot] = itemObject->GetName();
			}
			item->SetIsEquiped(true);
		}
	}
	ConsumeActResource(1);

	item->CompletePickup(owner);
	if (shouldRemovePickedObject && scene)
	{
		scene->QueueGameObjectRemoval(itemObject->GetName());
	}

	UpdateInventorySlotUI();
	return true;
}
void PlayerComponent::AddToInventory(ItemComponent* item)
{
	if (!item)
	{
		return;
	}

	auto* itemOwner = item->GetOwner();
	if (!itemOwner)
	{
		return;
	}

	m_InventoryItemIds.push_back(itemOwner->GetName());
}
