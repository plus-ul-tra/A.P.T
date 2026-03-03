#pragma once
#include "Component.h"
#include "RenderData.h"

class NodeComponent;
class MaterialComponent;

class ExitComponent : public Component {
	friend class Editor;

public :
	static constexpr const char* StaticTypeName = "ExitComponent";
	const char* GetTypeName() const override;
	ExitComponent() = default;
	virtual ~ExitComponent() = default;


	void Start() override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetTargetScene(const std::string& name) { m_TargetScene = name; }
	const std::string& GetTargetScene() const { return m_TargetScene; }

private:
	void UpdateHighlight(float deltaTime);
	void RequestSceneChange();

	std::string m_TargetScene;
	NodeComponent* m_Node = nullptr;
	MaterialComponent* m_Material = nullptr;
	bool m_SceneChangeRequested = false;
	bool m_HasBaseMaterial = false;
	float m_HighlightTime = 1.0f;
	RenderData::MaterialData m_BaseMaterialOverrides{};
};