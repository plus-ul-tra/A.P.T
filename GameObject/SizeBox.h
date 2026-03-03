#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"

class SizeBox : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "SizeBox";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetWidthOverride(const float& width) {
		m_WidthOverride = width;
		m_HasWidthOverride = true;
	}
	void SetHeightOverride(const float& height) {
		m_HeightOverride = height;
		m_HasHeightOverride = true;
	}

	void ClearWidthOverride()  { m_HasWidthOverride = false;  }
	void ClearHeightOverride() { m_HasHeightOverride = false; }

	const float& GetWidthOverride() const { return m_WidthOverride; }
	const float& GetHeightOverride() const { return m_HeightOverride; }
	const bool& GetHasWidthOverride() const { return m_HasWidthOverride; }
	const bool& GetHasHeightOverride() const { return m_HasHeightOverride; }

	void SetHasWidthOverride(const bool& hasOverride) { m_HasWidthOverride = hasOverride; }
	void SetHasHeightOverride(const bool& hasOverride) { m_HasHeightOverride = hasOverride; }

	void SetMinSize(const UISize& size) { m_MinSize = size; }
	void SetMaxSize(const UISize& size) { m_MaxSize = size; }
	const UISize& GetMinSize() const { return m_MinSize; }
	const UISize& GetMaxSize() const { return m_MaxSize; }

	UISize GetDesiredSize(const UISize& contentSize) const;

private:
	bool   m_HasWidthOverride = false;
	bool   m_HasHeightOverride = false;
	float  m_WidthOverride = 0.0f;
	float  m_HeightOverride = 0.0f;
	UISize m_MinSize{};
	UISize m_MaxSize{};
};

