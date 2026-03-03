#pragma once
#include "Component.h"
#include "GameObject.h"

class CameraComponent;
class TransformComponent;

// Game에서의 카메라 움직임(영역 제한 + 줌)을 다룸
class CameraLogicComponent2 : public Component, public IEventListener {
	friend class Editor;
public:

	static constexpr const char* StaticTypeName = "CameraLogicComponent2";
	const char* GetTypeName() const override;

	CameraLogicComponent2() = default;
	virtual ~CameraLogicComponent2();

	void SetMaxZoom(const float& value) { m_MaxZoom = value; }
	void SetMinZoom(const float& value) { m_MinZoom = value; }
	void SetMinX(const float& value) { m_MinX = value; }
	void SetMaxX(const float& value) { m_MaxX = value; }
	void SetMinZ(const float& value) { m_MinZ = value; }
	void SetMaxZ(const float& value) { m_MaxZ = value; }
	void SetMoveSpeed(const float& value) { m_MoveSpeed = value; }

	const float& GetMaxZoom() const { return m_MaxZoom; }
	const float& GetMinZoom() const { return m_MinZoom; }
	const float& GetMinX() const { return m_MinX; }
	const float& GetMaxX() const { return m_MaxX; }
	const float& GetMinZ() const { return m_MinZ; }
	const float& GetMaxZ() const { return m_MaxZ; }
	const float& GetMoveSpeed() const { return m_MoveSpeed; }

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

private:
	void CamZoom();
	void MoveInBound(float deltaTime);

private:
	float m_MaxZoom = 0.0f;
	float m_MinZoom = 0.0f;
	float m_ZoomSpeed = 1.0f;

	float m_MinX = -10.0f;
	float m_MaxX = 10.0f;
	float m_MinZ = -10.0f;
	float m_MaxZ = 10.0f;
	float m_MoveSpeed = 5.0f;

	TransformComponent* m_Transform = nullptr;
	CameraComponent* m_Camera = nullptr;
	float m_PendingZoomInput = 0.0f;
	float m_CurrentZoomOffset = 0.0f;
	bool m_MoveW = false;
	bool m_MoveA = false;
	bool m_MoveS = false;
	bool m_MoveD = false;
};
