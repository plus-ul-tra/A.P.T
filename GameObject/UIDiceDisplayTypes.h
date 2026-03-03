#pragma once
#include "UIPrimitives.h"
#include "ResourceHandle.h"
#include <array>
#include <string>

struct UIDiceDigitSlot
{
	UIAnchor anchor{ 0.0f, 0.0f };
	UIAnchor pivot { 0.0f, 0.0f };
	UIRect	 bounds{ 0.0f, 0.0f, 0.0f, 0.0f };
	std::array<UIAnchor, 10> digitOffsets{};
	bool useParentOffset = false;
};

struct UIDiceLayout
{
	std::string     type;
	TextureHandle   diceTexture = TextureHandle::Invalid();
	UIDiceDigitSlot tens;
	UIDiceDigitSlot ones;
};