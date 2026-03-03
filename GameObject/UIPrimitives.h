#pragma once

#include <algorithm>

struct UISize
{
	float width = 0.0f;
	float height = 0.0f;
};

struct UIAnchor
{
	float x = 0.0f;
	float y = 0.0f;
};

struct UIRect
{
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
};

struct UIPadding
{
	float left = 0.0f;
	float top = 0.0f;
	float right = 0.0f;
	float bottom = 0.0f;
};

enum class UIStretch
{
	None,
	ScaleToFit,
	ScaleToFill
};

enum class UIStretchDirection
{
	Both,
	DownOnly,
	UpOnly
};

enum class UIFillDirection
{
	LeftToRight,
	RightToLeft,
	TopToBottom,
	BottomToTop
};

enum class UIProgressFillMode
{
	Rect,
	Mask
};

inline float ClampUIValue(float value, float minValue, float maxValue)
{
	if (maxValue < minValue)
	{
		return value;
	}

	return std::clamp(value, minValue, maxValue);
}