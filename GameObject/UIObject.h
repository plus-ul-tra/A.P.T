#pragma once
#include "Object.h"
#include "UIPrimitives.h"
#include <windows.h>
#include "json.hpp"

class UIObject : public Object
{
	friend class UIManager;
public:
	UIObject(EventDispatcher& eventDispatcher);
	virtual ~UIObject() = default;

	//void Render(std::vector<UIRenderInfo>& renderInfo);
	//void Render(std::vector<UITextInfo>& textInfo);

	void  Serialize(nlohmann::json& j) const override;
	void  Deserialize(const nlohmann::json& j) override;
		  
		  
	void  SetZOrder(int zOrder);
	void  SetZOrderFromComponent(int zOrder);
	int   GetZOrder() const { return m_ZOrder; }
		  
		  
	bool  HitCheck       (const POINT& pos);
		  
	void  SetIsFullScreen(bool isFullScreen) { m_IsFullScreen = isFullScreen; }
	bool  IsFullScreen   ();
		  
	void  SetIsVisible(bool isVisible);
	void  SetIsVisibleFromComponent(bool isVisible);
	void  SetIsVisibleFromParent(bool isVisible);
	void  SetOpacity(float opacity);
	void  SetOpacityFromComponent(float opacity);
	float GetOpacity() const { return m_Opacity; }
	bool  IsVisible() const;
	bool  IsLocallyVisible() const { return m_IsVisible; }

	void SetBounds(const UIRect& bounds)
	{
		m_Bounds = bounds;
		m_HasBounds = true;
	}

	const UIRect& GetBounds() const
	{
		return m_Bounds;
	}

	bool HasBounds() const
	{
		return m_HasBounds;
	}

	void SetParentName(const std::string& name) { m_ParentName = name; }
	void ClearParentName() { m_ParentName.clear(); }
	const std::string& GetParentName() const { return m_ParentName; }

	void SetAnchorMin(const UIAnchor& anchor) { m_AnchorMin = anchor; }
	void SetAnchorMax(const UIAnchor& anchor) { m_AnchorMax = anchor; }
	void SetPivot(const UIAnchor& anchor) { m_Pivot = anchor; }
	const UIAnchor& GetAnchorMin() const { return m_AnchorMin; }
	const UIAnchor& GetAnchorMax() const { return m_AnchorMax; }
	const UIAnchor& GetPivot() const { return m_Pivot; }
	void SetRotationDegrees(const float& degree) { m_RotationDegrees = degree; }
	const float& GetRotationDegrees() const { return m_RotationDegrees; }


	// UI 오브젝트 컴포넌트 추가/삭제 시 플래그 갱신 예시 (UIObject 내부)
	void UpdateInteractableFlags();
	

protected:
	// UI 오브젝트에 컴포넌트 보유 여부 플래그 (UIObject 클래스에 추가)
	bool hasButton = false;
	bool hasSlider = false;
	bool hasUIFSM = false;


	int   m_ZOrder = 0;
	bool  m_IsFullScreen = false;
	bool  m_IsVisible = true;
	bool  m_IsVisibleFromParent = true;
	float m_Opacity = 1.0f;

	UIRect m_Bounds{};
	bool m_HasBounds = false;

	std::string m_ParentName;
	UIAnchor m_AnchorMin{ 0.0f, 0.0f };
	UIAnchor m_AnchorMax{ 0.0f, 0.0f };
	UIAnchor m_Pivot{ 0.0f, 0.0f };
	float m_RotationDegrees = 0.0f;
};

