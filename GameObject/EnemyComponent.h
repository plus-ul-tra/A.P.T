#pragma once
#include "Component.h"
#include "GameState.h"
#include "ResourceHandle.h"
#include "ResourceRefs.h"

#include <memory>
#include <vector>

class AIController;
class BTExecutor;
class TransformComponent;
class MaterialComponent;
class PlayerComponent;
enum class ERotationOffset : int;
class GridSystemComponent;
class NodeComponent;

class EnemyComponent : public Component, public IEventListener {
	friend class Editor;
public:
	static constexpr const char* StaticTypeName = "EnemyComponent";
	const char* GetTypeName() const override;

	EnemyComponent();
	virtual ~EnemyComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override; // IEventListener 필요

	void SetQR(int q, int r) { m_Q = q, m_R = r; }
	const int& GetQ() const { return m_Q; }
	const int& GetR() const { return m_R; }

	void SetMoveDistance(const int& value) { m_MoveDistance = value; }
	const int& GetMoveDistance() const { return m_MoveDistance; }
	Turn GetCurrentTurn() const { return m_CurrentTurn; }
	bool ConsumeMoveRequest();
	int GetActorId() const { return m_ActorId; }
	void SetActorId(int value) { m_ActorId = value; }
	void SetFacing(ERotationOffset value) { m_Facing = value; }
	ERotationOffset GetFacing() const { return m_Facing; }

	void SetDebugSightLines(const bool& value) { m_DebugSightLines = value; }
	const bool& GetDebugSightLines() const { return m_DebugSightLines; }
	void SetEndTurnDelay(const float& value) { m_EndTurnDelay = value; }
	const float& GetEndTurnDelay() const { return m_EndTurnDelay; }
	bool IsTargetVisible() const { return m_TargetVisible; }
	const AnimationHandle& GetDeathAnimationHandle() const { return m_DeathAnimationHandle; }
	void SetDeathAnimationHandle(const AnimationHandle& value);
	const TextureHandle& GetHoverInfoTextureHandle() const { return m_HoverInfoTextureHandle; }
	void SetHoverInfoTextureHandle(const TextureHandle& value) { m_HoverInfoTextureHandle = value; }
	const AnimationRef& GetDeathAnimation() const { return m_DeathAnimation; }
	void SetDeathAnimation(const AnimationRef& value) { m_DeathAnimation = value; }
	const float& GetDeathAnimationBlendTime() const { return m_DeathAnimationBlendTime; }
	void SetDeathAnimationBlendTime(const float& value) { m_DeathAnimationBlendTime = value; }
	const bool& GetUseDeathAnimationBlend() const { return m_UseDeathAnimationBlend; }
	void SetUseDeathAnimationBlend(const bool& value) { m_UseDeathAnimationBlend = value; }
	bool IsExploreTurnFinished() const { return m_ExploreTurnFinished; }
	void RefreshSightDebugLines();
	float GetExploreDelayRemaining() const { return m_ExploreDelayRemaining; }


private:

	void ClearSightDebug();
	void UpdateSightDebugLines(int sightRange);
	void SyncFacingFromTransform();

	int m_Q;
	int m_R;
	int m_MoveDistance = 1;
	int m_ActorId = 0;
	Turn m_CurrentTurn = Turn::PlayerTurn;

	std::unique_ptr<BTExecutor>   m_BTExecutor;
	std::unique_ptr<AIController> m_AIController;
	TransformComponent* m_TargetTransform = nullptr;
	PlayerComponent* m_TargetPlayer = nullptr;
	GridSystemComponent* m_GridSystem = nullptr;
	bool  m_MoveRequested = false;
	bool  m_TargetVisible = false;
	float m_EndTurnDelay = 1;
	ERotationOffset m_Facing;
	bool  m_DebugSightLines = false;
	bool  m_DeathAnimationStarted = false;
	bool  m_DeathAnimationCompleted = false;
	bool  m_DeathReported = false;
	bool  m_ExploreTurnFinished = false;
	float m_ExploreDelayRemaining = 0.0f;
	bool  m_PendingExploreEnd = false;
	bool  m_ExploreMoveIssued = false;
	AnimationHandle m_DeathAnimationHandle = AnimationHandle::Invalid();
	TextureHandle m_HoverInfoTextureHandle = TextureHandle::Invalid();
	AnimationRef m_DeathAnimation;
	float m_DeathAnimationBlendTime = 0.15f;
	bool m_UseDeathAnimationBlend = true;
	std::vector<NodeComponent*> m_SightDebugNodes;
};