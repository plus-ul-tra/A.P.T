#include "ReflectionMacro.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "AssetLoader.h"
#include "DiceSystem.h"
#include "GameDataRepository.h"
#include "GameObject.h"
#include "ItemComponent.h"
#include "ItemSpawnerComponent.h"
#include "LootRoller.h"
#include "LogSystem.h"
#include "MaterialComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "Scene.h"
#include "ServiceRegistry.h"
#include "TransformComponent.h"
#include "EnemyComponent.h"
#include "EnemyStatComponent.h"
#include "GridSystemComponent.h"
#include "NodeComponent.h"
#include "PlayerComponent.h"
#include "BoxColliderComponent.h"
#include "json.hpp"

REGISTER_COMPONENT(ItemSpawnerComponent)
REGISTER_PROPERTY(ItemSpawnerComponent, FixedItemTemplateName)
REGISTER_PROPERTY(ItemSpawnerComponent, DropItemTemplateName)
REGISTER_PROPERTY(ItemSpawnerComponent, FixedItemIndex)
REGISTER_PROPERTY(ItemSpawnerComponent, EnemyDefinitionId)
REGISTER_PROPERTY(ItemSpawnerComponent, DropTableGroupOverride)
REGISTER_PROPERTY(ItemSpawnerComponent, DebugDropTrigger)
REGISTER_PROPERTY(ItemSpawnerComponent, DropOnDeath)

namespace
{
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

	void ApplyItemDefinition(ItemComponent& item, const ItemDefinition& definition)
	{
		item.SetItemIndex(definition.index);
		item.SetType(ToItemType(definition.category));
		item.SetIconPath(definition.iconPath);
		item.SetMeshPath(definition.meshPath);
		item.SetPrice(definition.basePrice);
		item.SetMeleeAttackRange(definition.range);
		item.SetThrowRange(definition.throwRange);
		item.SetActionPointCost(definition.actionPointCost);
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

	std::string BuildSpawnName(const std::string& base)
	{
		static int counter = 0;
		const std::string prefix = base.empty() ? "ItemSpawned" : base;
		return prefix + "_" + std::to_string(counter++);
	}

	void EnsureRenderComponents(GameObject& object, const ItemDefinition& definition)
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
		}

		auto* materialComponent = object.GetComponent<MaterialComponent>();
		if (!materialComponent)
		{
			materialComponent = object.AddComponent<MaterialComponent>();
		}

		auto* boxColliderComponent = object.GetComponent<BoxColliderComponent>();
		if (!boxColliderComponent)
		{
			boxColliderComponent = object.AddComponent<BoxColliderComponent>();
		}


		if (definition.meshPath.empty())
		{
			return;
		}

		auto* loader = AssetLoader::GetActive();
		if (!loader)
		{
			return;
		}

		const auto* asset = loader->GetAsset(definition.meshPath);
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

	constexpr float kItemOverlapRatio = 0.2f;
	constexpr int kNeighborCount = 6;
	constexpr int kNeighborOffsets[kNeighborCount][2] = {
		{ 1, 0 },
		{ 1, -1 },
		{ 0, -1 },
		{ -1, 0 },
		{ -1, 1 },
		{ 0, 1 }
	};

	struct VertexCandidate
	{
		XMFLOAT3 position{};
		float distanceSq = 0.0f;
		int vertexIndex = 0;
	};

	float DistanceSq2D(const XMFLOAT3& a, const XMFLOAT3& b)
	{
		const float dx = a.x - b.x;
		const float dz = a.z - b.z;
		return dx * dx + dz * dz;
	}

	GridSystemComponent* FindGridSystem(const Scene& scene)
	{
		const auto& objects = scene.GetGameObjects();
		for (const auto& [name, object] : objects)
		{
			if (!object)
			{
				continue;
			}
			if (auto* grid = object->GetComponent<GridSystemComponent>())
			{
				return grid;
			}
		}
		return nullptr;
	}

	std::optional<XMFLOAT3> FindPlayerPosition(const Scene& scene)
	{
		const auto& objects = scene.GetGameObjects();
		for (const auto& [name, object] : objects)
		{
			if (!object)
			{
				continue;
			}
			if (auto* player = object->GetComponent<PlayerComponent>())
			{
				if (auto* transform = object->GetComponent<TransformComponent>())
				{
					return transform->GetWorldPos();
				}
			}
		}
		return std::nullopt;
	}

	bool HasItemAtPosition(const Scene& scene, const XMFLOAT3& position, float threshold)
	{
		const float thresholdSq = threshold * threshold;
		const auto& objects = scene.GetGameObjects();
		for (const auto& [name, object] : objects)
		{
			if (!object)
			{
				continue;
			}
			if (!object->GetComponent<ItemComponent>())
			{
				continue;
			}
			auto* transform = object->GetComponent<TransformComponent>();
			if (!transform)
			{
				continue;
			}
			if (DistanceSq2D(transform->GetWorldPos(), position) <= thresholdSq)
			{
				return true;
			}
		}
		return false;
	}

	float EstimateInnerRadius(NodeComponent* centerNode)
	{
		if (!centerNode)
		{
			return 1.0f;
		}
		auto* centerOwner = centerNode->GetOwner();
		auto* centerTransform = centerOwner ? centerOwner->GetComponent<TransformComponent>() : nullptr;
		if (!centerTransform)
		{
			return 1.0f;
		}
		for (auto* neighbor : centerNode->GetNeighbors())
		{
			if (!neighbor)
			{
				continue;
			}
			auto* neighborOwner = neighbor->GetOwner();
			auto* neighborTransform = neighborOwner ? neighborOwner->GetComponent<TransformComponent>() : nullptr;
			if (!neighborTransform)
			{
				continue;
			}
			const float dist = std::sqrt(DistanceSq2D(centerTransform->GetPosition(), neighborTransform->GetPosition()));
			if (dist > 0.0f)
			{
				return dist * 0.5f;
			}
		}
		return 1.0f;
	}

	std::optional<XMFLOAT3> FindDropVertexPosition(const Object& owner, const Scene& scene)
	{
		auto* ownerTransform = owner.GetComponent<TransformComponent>();
		if (!ownerTransform)
		{
			return std::nullopt;
		}

		auto* grid = FindGridSystem(scene);
		if (!grid)
		{
			return ownerTransform->GetWorldPos();
		}

		auto* enemy = owner.GetComponent<EnemyComponent>();
		if (!enemy)
		{
			return ownerTransform->GetWorldPos();
		}

		const AxialKey centerKey{ enemy->GetQ(), enemy->GetR() };
		auto* centerNode = grid->GetNodeByKey(centerKey);
		const float innerRadius = EstimateInnerRadius(centerNode);
		const float outerRadius = innerRadius * 2.0f / std::sqrt(3.0f);
		const float itemOverlapThreshold = outerRadius * kItemOverlapRatio;
		const XMFLOAT3 centerPos = ownerTransform->GetWorldPos();
		const auto playerPos = FindPlayerPosition(scene);

		std::vector<VertexCandidate> candidates;
		candidates.reserve(kNeighborCount);

		for (int i = 0; i < kNeighborCount; ++i)
		{
			const float angle = (60.0f * static_cast<float>(i) - 30.0f) * (3.1415926535f / 180.0f);
			const XMFLOAT3 vertexPos{
				centerPos.x + outerRadius * std::cos(angle),
				centerPos.y,
				centerPos.z + outerRadius * std::sin(angle)
			};

			const int prevIndex = (i + kNeighborCount - 1) % kNeighborCount;
			const AxialKey neighborA{ centerKey.q + kNeighborOffsets[i][0], centerKey.r + kNeighborOffsets[i][1] };
			const AxialKey neighborB{ centerKey.q + kNeighborOffsets[prevIndex][0], centerKey.r + kNeighborOffsets[prevIndex][1] };

			NodeComponent* nodesToCheck[3] = {
				centerNode,
				grid->GetNodeByKey(neighborA),
				grid->GetNodeByKey(neighborB)
			};

			bool valid = true;
			for (auto* node : nodesToCheck)
			{
				if (!node || !node->GetIsMoveable())
				{
					valid = false;
					break;
				}
			}
			if (!valid)
			{
				continue;
			}

			float distanceSq = 0.0f;
			if (playerPos)
			{
				distanceSq = DistanceSq2D(*playerPos, vertexPos);
			}

			candidates.push_back(VertexCandidate{ vertexPos, distanceSq, i });
		}

		if (candidates.empty())
		{
			return std::nullopt;
		}

		if (playerPos)
		{
			std::sort(candidates.begin(), candidates.end(),
				[](const VertexCandidate& a, const VertexCandidate& b)
				{
					return a.distanceSq < b.distanceSq;
				});
		}

		for (const auto& candidate : candidates)
		{
			if (HasItemAtPosition(scene, candidate.position, itemOverlapThreshold))
			{
				continue;
			}
			return candidate.position;
		}

		return std::nullopt;
	}

	void FinalizeSpawn(
		const Object& owner,
		const std::shared_ptr<GameObject>& spawned,
		const std::optional<XMFLOAT3>& positionOverride = std::nullopt) 
	{
		if (!spawned)
		{
			return;
		}

		if (auto* spawnTransform = spawned->GetComponent<TransformComponent>())
		{
			if (positionOverride)
			{
				spawnTransform->SetPosition(*positionOverride);
			}
			else if (auto* ownerTransform = owner.GetComponent<TransformComponent>()) 
			{
				spawnTransform->SetPosition(ownerTransform->GetPosition());
			}
		}

		spawned->Start();
	}
}

void ItemSpawnerComponent::Start()
{
}

void ItemSpawnerComponent::Update(float deltaTime)
{
	(void)deltaTime;

	SpwanFixedItem();

	if (m_DebugDropTrigger)
	{
		m_DebugDropTrigger = false;
		m_DropTriggered = false;
		DropItem();
		return;
	}

	if (!m_DropOnDeath || m_DropTriggered)
	{
		return;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* stat = owner->GetComponent<EnemyStatComponent>();
	if (!stat || stat->GetCurrentHP() > 0)
	{
		return;
	}

	DropItem();
}

void ItemSpawnerComponent::OnEvent(EventType type, const void* data)
{
	(void)type;
	(void)data;
}

void ItemSpawnerComponent::SpwanFixedItem()
{
	if (m_FixedItemSpawned)
	{
		return;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* scene = owner->GetScene();
	if (!scene)
	{
		return;
	}

	std::shared_ptr<GameObject> spawned;

	if (!m_FixedItemTemplateName.empty())
	{
		const auto& objects = scene->GetGameObjects();
		auto it = objects.find(m_FixedItemTemplateName);
		if (it != objects.end() && it->second)
		{
			nlohmann::json templateJson;
			it->second->Serialize(templateJson);
			templateJson["name"] = BuildSpawnName(m_FixedItemTemplateName);
			spawned = scene->CreateGameObject(templateJson["name"].get<std::string>());
			if (spawned)
			{
				spawned->Deserialize(templateJson);
			}
		}
	}

	if (!spawned && m_FixedItemIndex >= 0)
	{
		auto& services = scene->GetServices();
		if (services.Has<GameDataRepository>())
		{
			const auto* itemDefinition = services.Get<GameDataRepository>().GetItem(m_FixedItemIndex);
			if (itemDefinition)
			{
				const std::string spawnName = BuildSpawnName(itemDefinition->name);
				spawned = scene->CreateGameObject(spawnName);
				if (spawned)
				{
					if (auto* itemComponent = spawned->AddComponent<ItemComponent>())
					{
						ApplyItemDefinition(*itemComponent, *itemDefinition);
					}

					EnsureRenderComponents(*spawned, *itemDefinition);
				}
			}
		}
	}

	if (!spawned)
	{
		return;
	}

	FinalizeSpawn(*owner, spawned);

	m_SpawnItem = spawned.get();
	m_FixedItemSpawned = true;
}

void ItemSpawnerComponent::DropItem()
{
	if (m_DropTriggered)
	{
		return;
	}

	auto* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* scene = owner->GetScene();
	if (!scene)
	{
		return;
	}

	auto& services = scene->GetServices();
	if (!services.Has<GameDataRepository>() || !services.Has<DiceSystem>())
	{
		return;
	}

	auto& repository = services.Get<GameDataRepository>();
	auto& diceSystem = services.Get<DiceSystem>();
	LogSystem* logger = services.Has<LogSystem>() ? &services.Get<LogSystem>() : nullptr;

	const EnemyDefinition* enemyDefinition = nullptr;
	EnemyDefinition fallbackDefinition{};
	if (m_EnemyDefinitionId > 0)
	{
		enemyDefinition = repository.GetEnemy(m_EnemyDefinitionId);
	}

	if (!enemyDefinition)
	{
		auto* stat = owner->GetComponent<EnemyStatComponent>();
		if (!stat && m_DropTableGroupOverride <= 0)
		{
			return;
		}
		fallbackDefinition.name = owner->GetName();
		if (stat)
		{
			fallbackDefinition.difficultyGroup = stat->GetDifficultyGroup();
			fallbackDefinition.dropTableGroup = stat->GetDifficultyGroup();
		}
		else if (m_DropTableGroupOverride > 0)
		{
			fallbackDefinition.difficultyGroup = m_DropTableGroupOverride;
			fallbackDefinition.dropTableGroup = m_DropTableGroupOverride;
		}
		enemyDefinition = &fallbackDefinition;
	}
	else
	{
		fallbackDefinition = *enemyDefinition;
		enemyDefinition = &fallbackDefinition;
	}

	if (m_DropTableGroupOverride > 0)
	{
		fallbackDefinition.dropTableGroup = m_DropTableGroupOverride;
	}

	LootRoller lootRoller;
	const std::optional<LootRollResult> result =
		lootRoller.RollDrop(*enemyDefinition, repository, diceSystem, logger);

	if (!result)
	{
		m_DropTriggered = true;
		return;
	}

	const ItemDefinition* itemDefinition = repository.GetItem(result->itemIndex);
	if (!itemDefinition)
	{
		m_DropTriggered = true;
		return;
	}

	const std::optional<XMFLOAT3> dropPosition = FindDropVertexPosition(*owner, *scene);
	if (!dropPosition)
	{
		m_DropTriggered = true;
		return;
	}

	std::shared_ptr<GameObject> spawned;
	const std::string templateName =
		m_DropItemTemplateName.empty() ? m_FixedItemTemplateName : m_DropItemTemplateName;

	if (!templateName.empty())
	{
		const auto& objects = scene->GetGameObjects();
		auto it = objects.find(templateName);
		if (it != objects.end() && it->second)
		{
			nlohmann::json templateJson;
			it->second->Serialize(templateJson);
			templateJson["name"] = BuildSpawnName(templateName);
			spawned = scene->CreateGameObject(templateJson["name"].get<std::string>());
			if (spawned)
			{
				spawned->Deserialize(templateJson);
			}
		}
	}

	if (!spawned)
	{
		const std::string spawnName = BuildSpawnName(itemDefinition->name);
		spawned = scene->CreateGameObject(spawnName);
		if (spawned)
		{
			if (auto* itemComponent = spawned->AddComponent<ItemComponent>())
			{
				ApplyItemDefinition(*itemComponent, *itemDefinition);
			}

			EnsureRenderComponents(*spawned, *itemDefinition);
		}
	}
	else
	{
		if (auto* itemComponent = spawned->GetComponent<ItemComponent>())
		{
			ApplyItemDefinition(*itemComponent, *itemDefinition);
		}
		else if (auto* itemComponent = spawned->AddComponent<ItemComponent>())
		{
			ApplyItemDefinition(*itemComponent, *itemDefinition);
		}

		EnsureRenderComponents(*spawned, *itemDefinition);
	}

	FinalizeSpawn(*owner, spawned, dropPosition);
	m_DropTriggered = true;
}
