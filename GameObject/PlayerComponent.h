#pragma once
#include "Component.h"
#include "GameState.h"
#include <string>
#include <vector>
#include "GameObject.h"

class GridSystemComponent;
class EnemyComponent;
class DoorComponent;
class NodeComponent;
class ItemComponent;

// PlayerComponent 는 Player와 관련된 Data와 중요 로직
// Player의 다른 Component의 중추적인 역할
class PlayerComponent : public Component, public IEventListener {
	friend class Editor;
public:
	enum class CombatMode
	{
		Idle,
		Melee,
		Throw
	};

	static constexpr const char* StaticTypeName = "PlayerComponent";
	const char* GetTypeName() const override;

	PlayerComponent();
	virtual ~PlayerComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요



	// Getter Setter
	void SetQR(int q, int r) { m_Q = q, m_R = r; }
	const int& GetQ() const { return m_Q; }
	const int& GetR() const { return m_R; }
	
	void SetMoveResource(const int& move)  { m_MoveResource  = move; }
	void SetActResource(const int& act)	   { m_ActResource = act; }

	const int& GetMoveResource()	   const { return m_MoveResource; }
	const int& GetActResource()		   const { return m_ActResource; }
	const int& GetCurrentWeaponCost() const { return m_CurrentWeaponCost; }
	const int& GetAttackRange() const { return m_AttackRange; }
	const int& GetRemainMoveResource() const { return m_RemainMoveResource; }
	const int& GetRemainActResource()  const { return m_RemainActResource; }
	int GetActorId() const { return m_ActorId; }
	const int& GetMoney() const { return m_Money; }
	const std::vector<std::string>& GetInventoryItemIds() const { return m_InventoryItemIds; }
	Turn GetCurrentTurn() const { return m_CurrentTurn; }
	GridSystemComponent* GetGridSystem() const { return m_GridSystem; }
	const bool& GetDebugEquipItem() const { return m_DebugEquipItem; }
	const std::string& GetDebugCombatMode() const { return m_DebugCombatMode; }
	bool IsThrowPreviewActive() const { return m_CombatMode == CombatMode::Throw; }
	void SetIsThrowPreviewActive(bool value) { if (value) m_CombatMode = CombatMode::Throw; else if (m_CombatMode == CombatMode::Throw) m_CombatMode = CombatMode::Idle; }
	bool GetIsThrowPreviewActive() const { return IsThrowPreviewActive(); }
	CombatMode GetCombatMode() const { return m_CombatMode; }

	void ResetTurnResources();
	void BeginMove();
	bool CommitMove(int targetQ, int targetR);
	bool ConsumeActResource(int amount);

	void RequestCombatConfirm();
	bool HasCombatConfirmRequest() const { return m_CombatConfirmRequested; }
	bool HandleCombatClick(EnemyComponent* enemy);
	void ClearCombatSelection();
	EnemyComponent* ResolveCombatTarget(GameObject* obj) const;
	EnemyComponent* ConsumePendingAttackTarget();
	bool ConsumeCombatConfirmRequest();
	bool TryGetConsumableThrowRange(int& outRange) const;
	bool TryGetConsumableThrowItem(ItemComponent*& outItem) const;
	bool TryGetEquippedMeleeItem(ItemComponent*& outItem) const;
	void ConsumeThrowItem(ItemComponent* throwItem);
	void SelectConsumableThrowSlot(int slotIndex);
	bool ConsumePushPossible();
	bool ConsumePushTargetFound();
	bool ConsumePushSuccess();
	bool ConsumeDoorConfirmed();
	bool ConsumeDoorSuccess();
	bool ConsumeInventoryAtShop();
	bool ConsumeInventoryCanDrop();
	bool ConsumeShopHasSpace();
	bool ConsumeShopHasMoney();
	bool TryPickup(ItemComponent* item);
	void AddToInventory(ItemComponent* item);

	void SetCurrentWeaponCost(const int& value) { m_CurrentWeaponCost = value; }
	void SetAttackRange(const int& value) { m_AttackRange = value; }
	void SetPushPossible(bool value) { m_PushPossible = value; }
	void SetPushTargetFound(bool value) { m_PushTargetFound = value; }
	void SetPushSuccess(bool value) { m_PushSuccess = value; }
	void SetDoorConfirmed(bool value) { m_DoorConfirmed = value; }
	void SetDoorSuccess(bool value) { m_DoorSuccess = value; }
	void SetPendingDoor(DoorComponent* door);
	DoorComponent* ConsumePendingDoor();
	void SetInventoryAtShop(bool value) { m_InventoryAtShop = value; }
	void SetInventoryCanDrop(bool value) { m_InventoryCanDrop = value; }
	void SetShopHasSpace(bool value) { m_ShopHasSpace = value; }
	void SetShopHasMoney(bool value) { m_ShopHasMoney = value; }
	void SetActorId(int value) { m_ActorId = value; }
	void SetMoney(const int& value) { m_Money = value; }
	void SetInventoryItemIds(const std::vector<std::string>& value) { m_InventoryItemIds = value; }
	void SetDebugEquipItem(bool value) { m_DebugEquipItem = value; }
	void HandleCombatModeButtonState(const std::string& buttonEventName);

private:
	void ResetSubFSMFlags();
	bool ConsumeFlag(bool& flag);
	bool TryFindPushTarget(EnemyComponent*& outEnemy, NodeComponent*& outNode) const;
	bool ResolvePushTarget(EnemyComponent* enemy, NodeComponent* targetNode);
	void ClearPendingPush();
	void BeginThrowPreview();
	void EndThrowPreview();
	CombatMode ResolveBaseCombatMode() const;
	void SyncCombatModeFromInventory();
	void UpdateResourceUI();
	void ApplyAnimation();
	void ApplyVisualPresetByCombatMode();
	void UpdateInventorySlotUI();
	bool TryGetConsumableThrowItemBySlot(int slotIndex, ItemComponent*& outItem) const;

	// 외부지정 가능
	// 이동력, 행동력
	int m_MoveResource = 3; // 초기설정
	int m_ActResource = 6;  // 초기설정
	int m_CurrentWeaponCost = 1;
	int m_AttackRange = 1;

	//ReadOnly
	// Grid 기반 좌표 // 현재위치
	int m_Q = 0;
	int m_R = 0;

	//내부
	// 남은 값 (턴 변경 시 초기화)
	int m_RemainMoveResource = 0;
	int m_RemainActResource = 0;
	int m_LastRemainMoveResource = -1;
	int m_LastRemainActResource = -1;
	int m_LastMoveResource = -1;
	int m_LastActResource = -1;
	int m_ActorId = 1;
	int m_Money = 10;		
	std::vector<std::string> m_InventoryItemIds;
	int m_StartQ = 0; 
	int m_StartR = 0; 
	bool m_HasMoveStart = false;
	Turn m_CurrentTurn = Turn::PlayerTurn;
	bool m_TurnEndRequested = false;
	bool m_CombatConfirmRequested = false;
	EnemyComponent* m_SelectedEnemy = nullptr;
	EnemyComponent* m_PendingAttackTarget = nullptr;

	// 밀기
	bool m_PushPossible = true;
	bool m_PushTargetFound = true;
	bool m_PushSuccess = true;
	EnemyComponent* m_PendingPushEnemy = nullptr;
	NodeComponent* m_PendingPushNode = nullptr;

	// 문
	bool m_DoorConfirmed = true;
	bool m_DoorSuccess = true;
	DoorComponent* m_PendingDoor = nullptr; 

	
	bool m_InventoryAtShop = true;
	bool m_InventoryCanDrop = true;
	bool m_ShopHasSpace = true;
	bool m_ShopHasMoney = true;
	CombatMode m_CombatMode = CombatMode::Idle;
	bool m_DebugEquipItem = false;
	int m_ThrowPreviewRange = 0;
	std::string m_DebugCombatMode = "IdleMode";
	CombatMode m_LastVisualCombatMode = CombatMode::Idle;
	bool m_HasAppliedCombatVisual = false;
	bool m_LastVisualIsDead = false;
	GridSystemComponent* m_GridSystem = nullptr;

	GameObject* m_MeleeItem = nullptr;		//임시로 게임오브젝트 1개만 멤버로 저장
	std::string m_ConsumableItemNames[3] = {};
	int m_SelectedConsumableSlot = 0;
	bool m_IsApplyMeleeStat = false;
};