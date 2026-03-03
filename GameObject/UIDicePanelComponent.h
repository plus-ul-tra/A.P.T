#pragma once
#include "UIComponent.h"
#include "UIDicePanelTypes.h"
#include <string>
#include <unordered_map>
#include <vector>

class UIManager;
class UIObject;
class Scene;

class UIDiceDisplayComponent;
class UIDiceRollAnimationComponent;

class UIDicePanelComponent : public UIComponent
{
public:
	static constexpr const char* StaticTypeName = "UIDicePanelComponent";
	const char* GetTypeName() const override;

	virtual ~UIDicePanelComponent();

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void		SetEnabled(const bool& enabled);
	const bool& GetEnabled() const { return m_Enabled; }

	void SetSlots(std::vector<UIDicePanelSlot> slots);
	const std::vector<UIDicePanelSlot>& GetSlots() const { return m_Slots; }

	void			   SetActiveDiceType(const std::string& type);
	const std::string& GetActiveDiceType() const { return m_ActiveDiceType; }

	void		SetAutoVisibility(const bool& enabled);
	const bool& GetAutoVisibility() const { return m_AutoVisibility; }

	void		SetApplyDecisionD20OnRequest(const bool& enabled);
	const bool& GetApplyDecisionD20OnRequest() const { return m_ApplyDecisionD20OnRequest; }

	void RefreshBindings();

private:
	UIObject* FindUIObject(const std::string& name) const;
	void ApplySlot(UIObject& object, const UIDicePanelSlot& slot) const;
	bool TryPrepareRuntimeBindings();
	bool CanApplySlotsImmediately() const;
	void ApplySlotsImmediate() const;
	void ResetActiveSlotValues() const;
	std::string ResolveSlotDiceType(const UIDicePanelSlot& slot) const;
	bool ShouldShowSlot(const UIDicePanelSlot& slot) const;

	std::vector<UIDicePanelSlot> m_Slots;
	std::string m_ActiveDiceType;
	std::string m_PendingDiceType;
	std::unordered_map<std::string, std::string> m_ContextDiceTypes;
	bool m_Enabled = true;
	bool m_AutoVisibility = true;
	bool m_ApplyDecisionD20OnRequest = true;
	bool m_StatRollRequested = false;
	bool m_BindingsDirty = true;
	EventDispatcher* m_Dispatcher = nullptr;
	bool m_RuntimeBindingsReady = false;
	bool m_ListenersRegistered = false;
};

