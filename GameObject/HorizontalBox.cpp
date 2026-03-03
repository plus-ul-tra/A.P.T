#include "HorizontalBox.h"
#include "ReflectionMacro.h"
#include "UIObject.h"
#include <algorithm>

REGISTER_UI_COMPONENT(HorizontalBox)
REGISTER_PROPERTY(HorizontalBox, Slots)
REGISTER_PROPERTY(HorizontalBox, Spacing)

void HorizontalBox::Update(float deltaTime)
{
	UIComponent::Update(deltaTime);
}

void HorizontalBox::OnEvent(EventType type, const void* data)
{
	UIComponent::OnEvent(type, data);
}

void HorizontalBox::AddSlot(const HorizontalBoxSlot& slot)
{
	HorizontalBoxSlot updatedSlot = slot;
	if (updatedSlot.child && updatedSlot.childName.empty())
	{
		updatedSlot.childName = updatedSlot.child->GetName();
	}
	m_Slots.push_back(updatedSlot);
}

void HorizontalBox::SetSlots(const std::vector<HorizontalBoxSlot>& slots)
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

bool HorizontalBox::RemoveSlotByChild(const UIObject* child)
{
	const std::string targetName = child ? child->GetName() : "";
	const auto endIt = std::remove_if(m_Slots.begin(), m_Slots.end(), [&](const HorizontalBoxSlot& slot)
		{
			if(slot.child == child)
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

void HorizontalBox::ClearSlots()
{
	m_Slots.clear();
}

std::vector<UIRect> HorizontalBox::ArrangeChildren(float startX, float startY, const UISize& availableSize) const
{
	std::vector<UIRect> arranged;
	arranged.reserve(m_Slots.size());

	float totalFixedWidth  = 0.0f;
	float totalFillWeight = 0.0f;

	for (const auto& slot : m_Slots)
	{
		const float slotScale = slot.layoutScale > 0.0f ? slot.layoutScale : 1.0f;

		const float paddingLeft = slot.padding.left;
		const float paddingRight = slot.padding.right;

		if (slot.alignment == UIHorizontalAlignment::Fill)
		{
			const float effectiveWeight = slot.fillWeight > 0.0f ? slot.fillWeight : 1.0f;
			totalFillWeight += effectiveWeight;
		}
		else
		{
			totalFixedWidth += slot.desiredSize.width * slotScale;
		}
		totalFixedWidth += paddingLeft + paddingRight;
	}

	const float totalSpacing = m_Slots.size() > 1 ? m_Spacing * static_cast<float>(m_Slots.size() - 1) : 0.0f;
	totalFixedWidth += totalSpacing;
	float remaining = availableSize.width - totalFixedWidth;
	remaining = max(0.0f, remaining);
	const float contentWidth = totalFixedWidth + (totalFillWeight > 0.0f ? remaining : 0.0f);
	float cursorX = startX + max(0.0f, (availableSize.width - contentWidth) * 0.5f);

	for (size_t index = 0; index < m_Slots.size(); ++index)
	{
		const auto& slot = m_Slots[index];
		const float slotScale = slot.layoutScale > 0.0f ? slot.layoutScale : 1.0f;
		const float paddingLeft = slot.padding.left;
		const float paddingRight = slot.padding.right;
		const float paddingTop = slot.padding.top;
		const float paddingBottom = slot.padding.bottom;
		float width = slot.desiredSize.width * slotScale;
		if (slot.alignment == UIHorizontalAlignment::Fill && totalFillWeight > 0.0f)
		{
			const float effectiveWeight = slot.fillWeight > 0.0f ? slot.fillWeight : 1.0f;
			width = max(0.0f, remaining * (effectiveWeight / totalFillWeight));
		}

		const float x = cursorX + paddingLeft;
		const float y = startY + paddingTop;
		const float height = max(0.0f, availableSize.height - paddingTop - paddingBottom);

		const float clampedHeight = max(0.0f, min(height, width));
		if (clampedHeight > 0.0f && height > clampedHeight)
		{
			const float extra = height - clampedHeight;
			const float topAdjust = extra * 0.5f;
			arranged.push_back(UIRect{ x, y + topAdjust, width, clampedHeight });
		}
		else
		{
			arranged.push_back(UIRect{ x, y, width, height });
		}

		cursorX += width + paddingLeft + paddingRight;

		if (index + 1 < m_Slots.size())
		{
			cursorX += m_Spacing;
		}
	}

	return arranged;
}
