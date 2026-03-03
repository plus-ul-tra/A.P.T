#pragma once
#include "Component.h"

class TransformComponent;

class WaterRiseComponent : public Component
{
public:
	static constexpr const char* StaticTypeName = "WaterRiseComponent";
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const float& GetMinY() const { return m_MinY; }
	void SetMinY(const float& value) { m_MinY = value; }

	const float& GetMaxY() const { return m_MaxY; }
	void SetMaxY(const float& value) { m_MaxY = value; }

	const float& GetRiseDurationSeconds() const { return m_RiseDurationSeconds; }
	void SetRiseDurationSeconds(const float& value) { m_RiseDurationSeconds = value; }

	const bool& GetApplyMinYOnStart() const { return m_ApplyMinYOnStart; }
	void SetApplyMinYOnStart(const bool& value) { m_ApplyMinYOnStart = value; }

	const bool& GetLoop() const { return m_Loop; }
	void SetLoop(const bool& value) { m_Loop = value; }

	const bool& GetEnabled() const { return m_Enabled; }
	void SetEnabled(const bool& value) { m_Enabled = value; }

	const float& GetElapsedSeconds() const { return m_ElapsedSeconds; }
	const bool& GetFinished() const { return m_Finished; }

	void ResetRise();

private:
	void ApplyCurrentY();

private:
	float m_MinY = 0.0f;
	float m_MaxY = 10.0f;
	float m_RiseDurationSeconds = 30.0f;
	bool  m_ApplyMinYOnStart = true;
	bool  m_Loop = false;
	bool  m_Enabled = true;

	float m_ElapsedSeconds = 0.0f;
	bool  m_Finished = false;
};