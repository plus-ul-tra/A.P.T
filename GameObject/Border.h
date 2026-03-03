#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"

class Border : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "Border";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetPadding(const UIPadding& padding) { m_Padding = padding; }
	const UIPadding& GetPadding() const { return m_Padding; }

	UIRect GetContentRect(const UIRect& bounds) const;

private:
	UIPadding m_Padding{};
};

