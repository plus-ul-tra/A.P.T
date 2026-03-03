#pragma once
#include "Component.h"
#include <vector>
#include "IEventListener.h"

class Scene;
class UIManager;

class UIComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "UIComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void Serialize(nlohmann::json& j) const override;
	void Deserialize(const nlohmann::json& j) override;

	void SetVisible(const bool& visible);
	void SetZOrder(const int& value);
	void SetOpacity(const float& value);

	//참조변환
	const bool&	GetVisible()const { return m_Visible; }
	const int&	GetZOrder()	const { return m_ZOrder; }
	const float& GetOpacity()const { return m_Opacity; }
protected:
	Scene* GetScene() const;
	UIManager* GetUIManager() const;

	bool  m_Visible = true;
	int   m_ZOrder = 0;
	float m_Opacity = 1.0f;
	mutable UIManager* m_UIManager = nullptr;
	mutable Scene* m_UIScene = nullptr;
};

