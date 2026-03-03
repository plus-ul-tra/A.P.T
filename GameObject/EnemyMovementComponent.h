#pragma once
#include "Component.h"
#include <DirectXMath.h>
#include <array>
#include "GridSystemComponent.h"

class EnemyComponent;
class TransformComponent;

enum class ERotationOffset : int {
	clock_1,
	clock_3,
	clock_5,
	clock_7,
	clock_9,
	clock_11,
};

enum class EMoveOrder
{
	None,
	Patrol,
	Approach,
	RunOff,
	MaintainRange
};

// 이벤트 리스너는 쓸 얘들만
class EnemyMovementComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "EnemyMovementComponent";
	const char* GetTypeName() const override;

	EnemyMovementComponent()=default;
	virtual ~EnemyMovementComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요
	
	void MoveRunOff();
	void MoveApproach();
	void MoveMaintainRange();
	void MovePatrol();
	bool IsMoveComplete() const { return m_IsMoveComplete;  }

	void RequestMoveToTarget();
	void RequestRunOff();
	void RequestMaintainRange();
	void RotateTowardTarget(int targetQ, int targetR);


	struct PatrolPoint
	{
		int q = 0;
		int r = 0;
	};

	const std::array<PatrolPoint, 3>& GetPatrolPoints() const { return m_PatrolPoints; }
	void SetPatrolPoints(const std::array<PatrolPoint, 3>& points);

private:
	std::array<PatrolPoint, 3> m_PatrolPoints{};
	int  m_PatrolIndex = 0;
	bool m_HasPatrolPoints = false;

private:
	EMoveOrder m_PendingOrder = EMoveOrder::None;

	void SetEnemyRotation(TransformComponent* transComp, ERotationOffset dir);
	bool SelectMoveKeyTowardTarget(const AxialKey& start, const AxialKey& target, int moveRange,
		AxialKey& outPrevious, AxialKey& outNext, bool& outReachedTarget) const;
	bool MoveToNode(const AxialKey& previous, const AxialKey& current);
	bool HasValidPatrolPoints(const std::array<PatrolPoint, 3>& points) const;

	void GetSystem();
	bool m_IsMoveComplete = false;
	GridSystemComponent* m_GridSystem = nullptr;
};