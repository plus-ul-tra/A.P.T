#pragma once
#include "Component.h"
#include "GameState.h"

class GridSystemComponent;
class EnemyMovementComponent;
class EnemyComponent;

class EnemyControllerComponent : public Component, public IEventListener {

	friend class Editor;
public: 
	static constexpr const char* StaticTypeName = "EnemyControllerComponent";
	const char* GetTypeName() const override;

	EnemyControllerComponent() = default;
	virtual ~EnemyControllerComponent();

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	EnemyMovementComponent* GetCurrentEnemyMovement();
	bool IsCurrentEnemyMoveComplete();
private:

	void GetSystem();
	bool CheckActiveEnemies();
	EnemyComponent* GetCurrentEnemy() const;

	GridSystemComponent* m_GridSystem = nullptr;
	bool  m_TurnEndRequested = false;
	bool  m_CombatMoveInProgress = false;
	bool  m_CombatTurnEndRequested = false;
	int   m_ExploreEnemyIndex = 0;
	bool  m_WaitingExploreDelay = false;
	float m_ExploreDelayElapsed = 0.0f;
	float m_ExploreDelayDuration = 0.0f;
};
