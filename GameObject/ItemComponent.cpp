#include "ReflectionMacro.h"

#include "Scene.h"
#include "AssetLoader.h"
#include "GameDataRepository.h"
#include "MaterialComponent.h"
#include "GameObject.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "Object.h"
#include "Scene.h"
#include "SkeletalMeshComponent.h"
#include "TransformComponent.h"
#include "ServiceRegistry.h"
#include "SoundManager.h"
#include "PlayerComponent.h"
#include <algorithm>
#include <cmath>

#include "ItemComponent.h"


REGISTER_COMPONENT(ItemComponent)
REGISTER_PROPERTY(ItemComponent, ItemIndex)
REGISTER_PROPERTY(ItemComponent, IsEquiped)
REGISTER_PROPERTY(ItemComponent, Type)
REGISTER_PROPERTY(ItemComponent, Name)
REGISTER_PROPERTY(ItemComponent, IconPath)
REGISTER_PROPERTY(ItemComponent, MeshPath)
REGISTER_PROPERTY(ItemComponent, DescriptionIndex)
REGISTER_PROPERTY(ItemComponent, Price)
REGISTER_PROPERTY(ItemComponent, MeleeAttackRange)
REGISTER_PROPERTY(ItemComponent, DiceRoll)
REGISTER_PROPERTY(ItemComponent, DiceType)
REGISTER_PROPERTY(ItemComponent, BaseModifier)
REGISTER_PROPERTY(ItemComponent, Health)
REGISTER_PROPERTY(ItemComponent, Strength)
REGISTER_PROPERTY(ItemComponent, Agility)
REGISTER_PROPERTY(ItemComponent, Sense)
REGISTER_PROPERTY(ItemComponent, Skill)
REGISTER_PROPERTY(ItemComponent, DEF)
REGISTER_PROPERTY(ItemComponent, ThrowRange)
REGISTER_PROPERTY(ItemComponent, ActionPointCost)
REGISTER_PROPERTY(ItemComponent, DifficultyGroup)

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

	PlayerComponent* FindPlayerComponent(Scene* scene)
	{
		if (!scene)
		{
			return nullptr;
		}

		for (const auto& [name, object] : scene->GetGameObjects())
		{
			(void)name;
			if (!object)
			{
				continue;
			}

			if (auto* player = object->GetComponent<PlayerComponent>())
			{
				return player;
			}
		}

		return nullptr;
	}

	void EnsureRenderComponents(Object& object, const std::string& meshPath)
	{
		auto* gameObject = dynamic_cast<GameObject*>(&object);
		if (!gameObject)
		{
			return;
		}

		auto* meshComponent = gameObject->GetComponent<MeshComponent>();
		if (!meshComponent)
		{
			meshComponent = gameObject->AddComponent<MeshComponent>();
		}

		auto* meshRenderer = gameObject->GetComponent<MeshRenderer>();
		if (!meshRenderer)
		{
			meshRenderer = gameObject->AddComponent<MeshRenderer>();
		}
		if (meshRenderer)
		{
			meshRenderer->SetRenderLayer(static_cast<UINT8>(RenderData::RenderLayer::TransparentItems));
			meshRenderer->SetVisible(true);
		}

		auto* materialComponent = gameObject->GetComponent<MaterialComponent>();
		if (!materialComponent)
		{
			materialComponent = gameObject->AddComponent<MaterialComponent>();
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
			meshComponent->SetMeshHandle(asset->meshes.front());
		}

		if (materialComponent && !asset->materials.empty())
		{
			materialComponent->SetMaterialHandle(asset->materials.front());
		}
	}
}

ItemComponent::ItemComponent()
{
}

ItemComponent::~ItemComponent()
{
}

void ItemComponent::Start()
{
	XMStoreFloat4x4(&m_EquipmentBindPose, XMMatrixIdentity());

	m_IsEquiped = false;

	if (m_ItemIndex < 0)
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

	ServiceRegistry& services = scene->GetServices();
	if (!services.Has<GameDataRepository>())
	{
		return;
	}

	auto& repository = services.Get<GameDataRepository>();

	const ItemDefinition* definition = repository.GetItem(m_ItemIndex);
	if (!definition)
	{
		return;
	}

	ApplyItemDefinition(*this, *definition);
	EnsureRenderComponents(*owner, m_MeshPath);
}

void ItemComponent::Update(float deltaTime)
{
	Object* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* transform = owner->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	if (m_IsThrown)
	{
		m_ThrowElapsed += deltaTime;
		const float duration = m_ThrowDuration > 0.0f ? m_ThrowDuration : 0.001f;
		const float t = std::clamp(m_ThrowElapsed / duration, 0.0f, 1.0f);
		const float arcOffset = 4.0f * m_ThrowArcHeight * t * (1.0f - t);
		const XMFLOAT3 pos{
			m_ThrowStart.x + (m_ThrowTarget.x - m_ThrowStart.x) * t,
			m_ThrowStart.y + (m_ThrowTarget.y - m_ThrowStart.y) * t + arcOffset,
			m_ThrowStart.z + (m_ThrowTarget.z - m_ThrowStart.z) * t
		};
		transform->SetPosition(pos);

		if (t >= 1.0f)
		{
			if (auto* scene = owner->GetScene())
			{
				
				// Here
				auto& services = scene->GetServices();
				if (services.Has<SoundManager>())
				{
					services.Get<SoundManager>().SFX_Shot(L"Ranged_Attack_NonFragile");
				}

				scene->QueueGameObjectRemoval(owner->GetName());
			}
		}

		return;
	}

	//장착됨 상태라면 이 아이템을 가지는 PlayerComponent에서 행렬을 넘겨주고, 그 값을 Transform으로 적용
	if (m_IsEquiped)
	{
		XMVECTOR scale;
		XMVECTOR rotQuat;
		XMVECTOR translation;

		XMMATRIX world = XMLoadFloat4x4(&m_EquipmentBindPose);

		bool success = XMMatrixDecompose(
			&scale,
			&rotQuat,
			&translation,
			world
		);

		XMFLOAT3 pos;
		XMFLOAT3 scl;
		XMFLOAT4 rot;

		XMStoreFloat3(&pos, translation);
		XMStoreFloat3(&scl, scale);
		XMStoreFloat4(&rot, rotQuat);

		transform->SetPosition(pos);
		transform->SetRotation(rot);
		transform->SetScale(scl);

	}
	
	auto* scene = owner ? owner->GetScene() : nullptr;
	if (!scene || scene->GetIsPause())
	{
		return;
	}

	SelfRotate(deltaTime);
	SelfBob(deltaTime);
}

void ItemComponent::OnEvent(EventType type, const void* data)
{
}

bool ItemComponent::RequestPickup(Object* picker)
{
	if (m_PickupState != ItemPickupState::World)
	{
		return false;
	}

	m_PickupState = ItemPickupState::PickingUp;
	m_PickupOwner = picker;
	return true;
}

void ItemComponent::CompletePickup(Object* picker)
{
	if (m_PickupState != ItemPickupState::PickingUp)
	{
		return;
	}

	m_PickupState = ItemPickupState::Owned;
	m_PickupOwner = picker;
}

bool ItemComponent::Sell()
{
	auto* owner = GetOwner();
	if (!owner)
	{
		return false;
	}

	auto* scene = owner->GetScene();
	auto* player = FindPlayerComponent(scene);
	if (!player)
	{
		return false;
	}

	const int sellPrice = max(0, m_Price);
	player->SetMoney(player->GetMoney() + sellPrice);
	return true;
}

void ItemComponent::BeginThrow(const XMFLOAT3& start, const XMFLOAT3& target, float duration)
{
	m_IsEquiped = false;
	m_IsThrown = true;
	m_ThrowElapsed = 0.0f;
	m_ThrowDuration = duration > 0.0f ? duration : 0.001f;
	m_ThrowStart = start;
	m_ThrowTarget = target;
	const float dx = m_ThrowTarget.x - m_ThrowStart.x;
	const float dz = m_ThrowTarget.z - m_ThrowStart.z;
	const float planarDistance = sqrtf(dx * dx + dz * dz);
	m_ThrowArcHeight = max(0.3f, planarDistance * 0.25f);
	if (auto* owner = GetOwner())
	{
		if (auto* transform = owner->GetComponent<TransformComponent>())
		{
			transform->SetPosition(start);
		}
	}
}

void ItemComponent::SelfRotate(float deltaTime)
{
	Object* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* transform = owner->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	XMFLOAT4 currentRot = transform->GetRotation();
	XMVECTOR currentQuat = XMLoadFloat4(&currentRot);
	XMVECTOR deltaQuat = XMQuaternionRotationRollPitchYaw(0.0f, deltaTime, 0.0f);
	XMVECTOR nextQuat = XMQuaternionNormalize(XMQuaternionMultiply(currentQuat, deltaQuat));

	XMStoreFloat4(&currentRot, nextQuat);
	transform->SetRotation(currentRot);
}

void ItemComponent::SelfBob(float dTime)
{
	Object* owner = GetOwner();
	if (!owner)
	{
		return;
	}

	auto* transform = owner->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	m_BobTime += dTime;

	XMFLOAT3 pos = transform->GetPosition();

	const float bobHeight = 0.001f;   // 움직이는 높이
	const float bobSpeed = 2.0f;    // 속도

	pos.y += sinf(m_BobTime * bobSpeed) * bobHeight;

	transform->SetPosition(pos);
}
