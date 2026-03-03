#pragma once
#include "FSMComponent.h"

class PlayerMoveFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "PlayerMoveFSMComponent";
	const char* GetTypeName() const override;

	PlayerMoveFSMComponent();
	virtual ~PlayerMoveFSMComponent() override = default;

	void Start() override;

	// 드래그 프리뷰를 FSM이 켜고/끄는 플래그
	bool IsDraggingActive() const { return m_DraggingActive; }

	// Pending(Q, R) 저장소
	void SetPendingTarget(int q, int r) { m_HasPendingTarget = true; m_PendingQ = q; m_PendingR = r; }
	void ClearPendingTarget() { m_HasPendingTarget = false; m_PendingQ = 0; m_PendingR = 0; }
	bool HasPendingTarget() const { return m_HasPendingTarget; }
	int  GetPeningQ() const { return m_PendingQ; }
	int  GetPeningR() const { return m_PendingR; }
	 
	// Update에서 프리뷰 처리
	void Update(float deltaTime) override;

private:
	void UpdateDragPreview();

private:
	bool m_DraggingActive  = false;
	bool m_CommitSucceeded = false;

	//valid/invalid 변화 감지용
	bool m_LastValid = false;

	bool m_HasPendingTarget = false;

	int  m_PendingQ = 0;
	int  m_PendingR = 0;

	bool m_DebugVisualToggleFlip = false;
};