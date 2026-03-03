#pragma once
#include "Component.h"

class SceneDelayComponent : public Component
{
	friend class Editor;

public:
	static constexpr const char* StaticTypeName = "SceneDelayComponent";
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	const std::string& GetTargetScene() const { return m_TargetScene; }
	void SetTargetScene(const std::string& value) { m_TargetScene = value; }

	const float& GetDelaySeconds() const { return m_DelaySeconds; }
	void SetDelaySeconds(const float& value) { m_DelaySeconds = value; }

	const bool& GetEnabled() const { return m_Enabled; }
	void SetEnabled(const bool& value) { m_Enabled = value; }

	const bool& GetRequested() const { return m_Requested; }
	const float& GetElapsedSeconds() const { return m_ElapsedSeconds; }

	void ResetTimer();

private:
	void RequestSceneChange();

private:
	std::string m_TargetScene;
	float m_DelaySeconds = 5.0f;
	bool m_Enabled = true;

	float m_ElapsedSeconds = 0.0f;
	bool m_Requested = false;
};