#pragma once
#include "Component.h"
#include <string>

enum class ItemType
{
	GOLD,
	EQUIPMENT,
	HEAL,
	THROW,

	TYPE_MAX
};

enum class ItemPickupState
{
	World,
	PickingUp,
	Owned
};

class ItemComponent : public Component
{
public:
	static constexpr const char* StaticTypeName = "ItemComponent";
	const char* GetTypeName() const override;

	ItemComponent();
	virtual ~ItemComponent();

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	bool RequestPickup(Object* picker);
	void CompletePickup(Object* picker);
	bool Sell();
	ItemPickupState GetPickupState() const { return m_PickupState; }

	const int& GetItemIndex() const { return m_ItemIndex; }
	void SetItemIndex(const int& value) { m_ItemIndex = value; }

	const bool& GetIsEquiped() const { return m_IsEquiped; }
	void SetIsEquiped(const bool& value) { m_IsEquiped = value; }

	const int& GetType() const { return m_Type; }
	void SetType(const int& value) { m_Type = value; }

	const int& GetName() const { return m_Name; }
	void SetName(const int& value) { m_Name = value; }

	const std::string& GetIconPath() const { return m_IconPath; }
	void SetIconPath(const std::string& value) { m_IconPath = value; }

	const std::string& GetMeshPath() const { return m_MeshPath; }
	void SetMeshPath(const std::string& value) { m_MeshPath = value; }

	const int& GetDescriptionIndex() const { return m_DescriptionIndex; }
	void SetDescriptionIndex(const int& value) { m_DescriptionIndex = value; }

	const int& GetPrice() const { return m_Price; }
	void SetPrice(const int& value) { m_Price = value; }

	const int& GetMeleeAttackRange() const { return m_MeleeAttackRange; }
	void SetMeleeAttackRange(const int& value) { m_MeleeAttackRange = value; }

	const int& GetDiceRoll() const { return m_DiceRoll; }
	void SetDiceRoll(const int& value) { m_DiceRoll = value; }

	const int& GetDiceType() const { return m_DiceType; }
	void SetDiceType(const int& value) { m_DiceType = value; }

	const int& GetBaseModifier() const { return m_BaseModifier; }
	void SetBaseModifier(const int& value) { m_BaseModifier = value; }

	const int& GetHealth() const { return m_Health; }
	void SetHealth(const int& value) { m_Health = value; }

	const int& GetStrength() const { return m_Strength; }
	void SetStrength(const int& value) { m_Strength = value; }

	const int& GetAgility() const { return m_Agility; }
	void SetAgility(const int& value) { m_Agility = value; }

	const int& GetSense() const { return m_Sense; }
	void SetSense(const int& value) { m_Sense = value; }

	const int& GetSkill() const { return m_Skill; }
	void SetSkill(const int& value) { m_Skill = value; }

	const int& GetDEF() const { return m_DEF; }
	void SetDEF(const int& value) { m_DEF = value; }

	const int& GetThrowRange() const { return m_ThrowRange; }
	void SetThrowRange(const int& value) { m_ThrowRange = value; }

	const int& GetActionPointCost() const { return m_ActionPointCost; }
	void SetActionPointCost(const int& value) { m_ActionPointCost = value; }

	const int& GetDifficultyGroup() const { return m_DifficultyGroup; }
	void SetDifficultyGroup(const int& value) { m_DifficultyGroup = value; }

	const DirectX::XMFLOAT4X4& GetEquipmentBindPose() const { return m_EquipmentBindPose; }
	void SetEquipmentBindPose(const DirectX::XMFLOAT4X4& value) { m_EquipmentBindPose = value; }
	void BeginThrow(const DirectX::XMFLOAT3& start, const DirectX::XMFLOAT3& target, float duration = 0.25f);

private:
	void SelfRotate(float dTime);
	void SelfBob(float dTime);

	float m_BobTime = 0.0f;

	int m_ItemIndex				= -1;

	ItemPickupState m_PickupState = ItemPickupState::World;
	Object* m_PickupOwner = nullptr;

	bool m_IsEquiped			= false;
	int m_Type					= 0;
	int m_Name					= 0;
	std::string m_IconPath		= "";
	std::string m_MeshPath		= "";
	int m_DescriptionIndex		= 0;
	int m_Price					= 0;
	int m_MeleeAttackRange		= 1;
	int m_DiceRoll				= 0;     // 주사위 굴림횟수 (DnD룰에서 1d4의 d)
	int m_DiceType				= 0;    // 주사위 면체 수 (DnD룰에서 1d4의 4)
	int m_BaseModifier			= 0;      // 데미지 보정치 (고정 추가 데미지)
	int m_Health				= 0;
	int m_Strength				= 0;
	int m_Agility				= 0;
	int m_Sense					= 0;
	int m_Skill					= 0;
	int m_DEF					= 0;
	int m_ThrowRange			= 0;
	int m_ActionPointCost		= 1;
	int m_DifficultyGroup		= 0;

	DirectX::XMFLOAT4X4 m_EquipmentBindPose{};

	bool m_IsThrown = false;
	float m_ThrowElapsed = 0.0f;
	float m_ThrowDuration = 0.25f;
	DirectX::XMFLOAT3 m_ThrowStart{};
	DirectX::XMFLOAT3 m_ThrowTarget{};

	float m_ThrowArcHeight = 0.0f;
};