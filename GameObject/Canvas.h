#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"
#include <vector>

class UIObject;

struct CanvasSlot
{
	UIObject*   child = nullptr;
	std::string childName;
	UIRect      rect{};
};

class Canvas : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "Canvas";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void AddSlot(const CanvasSlot& slot);
	void SetSlots(const std::vector<CanvasSlot>& slots);
	const std::vector<CanvasSlot>& GetSlots() const { return m_Slots; }
	std::vector<CanvasSlot>& GetSlotsRef() { return m_Slots; }
	bool RemoveSlotByChild(const UIObject* child);
	void ClearSlots();

private:
	std::vector<CanvasSlot> m_Slots;
};

