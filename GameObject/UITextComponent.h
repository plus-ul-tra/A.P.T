#pragma once
#include "UIComponent.h"
#include "MathHelper.h"
#include <string>

class UITextComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UITextComponent";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetText(const std::string& text) { m_Text = text; }
	const std::string& GetText() const { return m_Text; }

	void SetFontSize(const float& size) { m_FontSize = size; }
	const float& GetFontSize() const { return m_FontSize; }

	void SetTextColor(const XMFLOAT4& color) { m_TextColor = color; }
	const XMFLOAT4& GetTextColor() const	 { return m_TextColor; }

private:
	std::string m_Text;
	float m_FontSize = 16.0f;
	XMFLOAT4 m_TextColor{ 1.0f, 1.0f, 1.0f, 1.0f };
};

