#pragma once
#include "UIComponent.h"
#include "UIPrimitives.h"
#include <string>
#include <vector>

class EventDispatcher;
class UIManager;
class UIObject;
class Scene;

namespace Events
{
	struct DiceRollEvent;
}

class UIDiceRollAnimationComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIDiceRollAnimationComponent";
	~UIDiceRollAnimationComponent() override;
	const char* GetTypeName() const override;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void		SetEnabled(const bool& enabled);
	const bool& GetEnabled() const { return m_Enabled; }

	void			   SetDiceContext(const std::string& context);
	const std::string& GetDiceContext() const { return m_DiceContext; }

	void SetDelayRange(float minDelay, float maxDelay);

	void		 SetDelayMin(const float& value);
	const float& GetDelayMin() const { return m_DelayMin; }

	void		 SetDelayMax(const float& value);
	const float& GetDelayMax() const { return m_DelayMax; }

	void		 SetAnimationDuration(const float& duration);
	const float& GetAnimationDuration() const { return m_AnimationDuration; }

	void		 SetScaleStart(const float& scale);
	const float& GetScaleStart() const { return m_ScaleStart; }

	void		 SetScalePeak(const float& scale);
	const float& GetScalePeak() const { return m_ScalePeak; }

	void		 SetScaleEnd(const float& scale);
	const float& GetScaleEnd() const { return m_ScaleEnd; }

	void		SetApplyToDigits(const bool& apply);
	const bool& GetApplyToDigits() const { return m_ApplyToDigits; }

	void		SetAnimateIndividuals(const bool& animate);
	const bool& GetAnimateIndividuals() const { return m_AnimateIndividuals; }

	void			   SetTensDigitObjectName(const std::string& name);
	const std::string& GetTensDigitObjectName() const { return m_TensDigitObjectName; }

	void			   SetOnesDigitObjectName(const std::string& name);
	const std::string& GetOnesDigitObjectName() const { return m_OnesDigitObjectName; }

private:
	struct BoundsSnapshot
	{
		UIObject* object = nullptr;
		UIRect	  bounds{};
	};

	UIObject* FindUIObject(const std::string& name) const;

	void  BeginAnimation();
	void  CacheBounds();
	void  ApplyScale(float scale);
	void  RestoreBounds();
	float GetRandomDelay();
	float EvaluateScale(float t) const;
	bool  TryPrepareRuntimeBindings();

	std::vector<BoundsSnapshot> m_Targets;
	std::string m_DiceContext;
	std::string m_TensDigitObjectName = "DiceTens";
	std::string m_OnesDigitObjectName = "DiceOnes";
	float m_DelayMin = 0.1f;
	float m_DelayMax = 0.3f;
	float m_AnimationDuration = 0.25f;
	float m_ScaleStart = 1.2f;
	float m_ScalePeak = 1.25f;
	float m_ScaleEnd = 1.0f;
	float m_AnimationTimer = 0.0f;
	bool  m_Enabled = true;
	bool  m_ApplyToDigits = true;
	float m_DelayTimer = 0.0f;
	bool  m_Waiting = false;
	bool  m_Animating = false;
	bool  m_AnimateIndividuals = true;
	EventDispatcher* m_Dispatcher = nullptr;
	bool m_RuntimeBindingsReady = false;
	bool m_ListenerRegistered = false;
};
