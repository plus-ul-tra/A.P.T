#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"
#include <string>
#include <vector>

class UIObject;

enum class UIHorizontalAlignment
{
	Left,
	Center,
	Right,
	Fill
};

struct HorizontalBoxSlot
{
	UIObject*             child		  = nullptr;
	std::string			  childName;
	UISize                desiredSize{};
	UIPadding             padding{};
	float                 fillWeight  = 0.0f;
	float                 layoutScale = 1.0f;
	UIHorizontalAlignment alignment   = UIHorizontalAlignment::Left;
};

class HorizontalBox : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "HorizontalBox";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void AddSlot(const HorizontalBoxSlot& slot);

	std::vector<HorizontalBoxSlot>& GetSlotsMutable()	   { return m_Slots; }
	const std::vector<HorizontalBoxSlot>& GetSlots() const { return m_Slots; }
	void SetSlots(const std::vector<HorizontalBoxSlot>& slots);

	void SetSpacing(const float& spacing) { m_Spacing = spacing; }
	const float& GetSpacing() const { return m_Spacing; }

	std::vector<HorizontalBoxSlot>& GetSlotsRef() { return m_Slots; }
	bool RemoveSlotByChild(const UIObject* child);
	void ClearSlots();

	std::vector<UIRect> ArrangeChildren(float startX, float startY, const UISize& availableSize) const;

private:
	std::vector<HorizontalBoxSlot> m_Slots;
	float m_Spacing = 0.0f;
};

