#include "Canvas.h"
#include "ReflectionMacro.h"
#include "UIObject.h"
#include <algorithm>

REGISTER_UI_COMPONENT(Canvas)
REGISTER_PROPERTY(Canvas, Slots)

void Canvas::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void Canvas::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void Canvas::AddSlot(const CanvasSlot& slot)
{
	CanvasSlot updatedSlot = slot;
	if (updatedSlot.child && updatedSlot.childName.empty())
	{
		updatedSlot.childName = updatedSlot.child->GetName();
	}
	m_Slots.push_back(updatedSlot);
}

void Canvas::SetSlots(const std::vector<CanvasSlot>& slots)
{
	m_Slots = slots;
	for (auto& slot : m_Slots)
	{
		if (slot.child && slot.childName.empty())
		{
			slot.childName = slot.child->GetName();
		}
	}
}

bool Canvas::RemoveSlotByChild(const UIObject* child)
{
	const std::string targetName = child ? child->GetName() : "";
	const auto endIt = std::remove_if(m_Slots.begin(), m_Slots.end(), [&](const CanvasSlot& slot)
		{
			if (slot.child == child)
			{
				return true;
			}
			return !targetName.empty() && slot.childName == targetName;
		});

	if (endIt == m_Slots.end())
	{
		return false;
	}

	m_Slots.erase(endIt, m_Slots.end());
	return true;
}

void Canvas::ClearSlots()
{
	m_Slots.clear();
}