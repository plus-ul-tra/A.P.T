#pragma once
#include "Component.h"

class SceneChangeTestComponent : public Component, public IEventListener {

	friend class Editor;

public :
	static constexpr const char* StaticTypeName = "SceneChangeTestComponent";
	const char* GetTypeName() const override;

	SceneChangeTestComponent();
	virtual ~SceneChangeTestComponent();

	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SceneChange();
	void SetTargetScene(const std::string& name) { m_TargetScene = name; }
	const std::string& GetTargetScene() const { return m_TargetScene; }

private:
	std::string m_TargetScene;
};