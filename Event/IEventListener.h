#pragma once
 
enum class EventType
{
	//Input
	KeyDown,
	KeyUp,
	MouseLeftClick,
	MouseLeftClickHold,
	MouseLeftClickUp,
	MouseLeftDoubleClick,
	MouseRightClick,
	MouseRightClickHold,
	MouseRightClickUp,
	Dragged,
	Hovered,
	MouseWheelUp,
	MouseWheelDown,

	//UI
	Pressed,
	Released,
	Moved,
	UIHovered,
	UIDragged,
	UIDoubleClicked,

	//Combat
	CombatEnter,
	CombatExit,
	CombatInitiativeBuilt,
	CombatTurnAdvanced,
	CombatContextReady,
	CombatInitComplete,
	CombatEnded,

	//Collision
	CollisionEnter,
	CollisionStay,
	CollisionExit,
	CollisionTrigger,

	//AI
	AIMoveRequested,
	AIRunOffMoveRequested,
	AIMaintainRangeRequested,
	AITurnEndRequested,
	AIMeleeAttackRequested,
	AIRangedAttackRequested,

	//Turn
	PlayerTurnEndRequested,
	EnemyTurnEndRequested,
	TurnChanged,
	ExploreTurnEnded,
	ExploreEnemyStepEnded,
	PhaseRequestEnterCombat,
	PostCombatToShop,
	PostCombatToExploration,
	ShopDone,
	FloorReady,
	GameStart,
	InitComplete,
	GameWin,
	GameOver,

	//Scene
	SceneChangeRequested,

	//Player
	PlayerMove,
	PlayerAttack,
	PlayerEquipFailed,
	PlayerDoorInteract,
	PlayerDoorCancel,
	PlayerShopOpen,
	PlayerShopClose,
	PlayerDiceRoll, 
	PlayerDiceUIOpen,
	PlayerDiceUIReset,
	PlayerDiceRollRequested,
	PlayerDiceRollApplied,
	PlayerDiceTotalsApplied,
	PlayerDiceResultShown,
	PlayerDiceUIClose,
	PlayerDiceDecisionRequested,
	PlayerDiceDecisionFaceRolled,
	PlayerDiceDecisionResult,
	PlayerDiceInitiativeResolved,
	PlayerDiceStatRollRequested,
	PlayerDiceTypeDetermined,
	PlayerDiceStatResolved,
	PlayerDiceAnimationStarted,
	PlayerDiceAnimationCompleted,
	PlayerDiceContinueRequested,
	PlayerStatChanged,


	DiceRolled,
	CombatNumberPopup,
	GoldAcquired,

	//Enemy
	EnemyAttack,
	EnemyHovered,

};

class IEventListener
{
public:
	virtual ~IEventListener() = default;
	virtual void OnEvent(EventType type, const void* data) = 0;
	virtual bool ShouldHandleEvent(EventType type, const void* data)
	{
		(void)type;
		(void)data;
		return true;
	}
	virtual int GetEventPriority() const { return 0; }
};
