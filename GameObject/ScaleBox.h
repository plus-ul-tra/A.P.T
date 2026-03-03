#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"

class ScaleBox : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "ScaleBox";
	const char* GetTypeName() const override;

	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void SetStretch(const UIStretch& stretch) { m_Stretch = stretch; }
	const UIStretch& GetStretch() const        { return m_Stretch;    }

	void SetStretchDirection(const UIStretchDirection& direction) { m_StretchDirection = direction; }
	const UIStretchDirection& GetStretchDirection() const		  { return m_StretchDirection;      }

	float  CalculateScale(const UISize& availableSize, const UISize& desiredSize) const;
	UISize CalculateScaledSize(const UISize& availableSize, const UISize& desiredSize) const;

private:
	UIStretch          m_Stretch          = UIStretch::ScaleToFit;
	UIStretchDirection m_StretchDirection = UIStretchDirection::Both;
};

