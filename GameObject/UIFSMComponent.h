#pragma once
#include "FSMComponent.h"
#include <unordered_map>
#include "UIPrimitives.h"

enum class Turn;

struct UIFSMEventCallback
{
	std::string eventName;
	std::string callbackId;
};

struct UIFSMCallbackAction
{
	std::string callbackId;
	std::vector<FSMAction> actions;
};

class UIFSMComponent : public FSMComponent
{
public:
	static constexpr const char* StaticTypeName = "UIFSMComponent";
	const char* GetTypeName() const override;

	UIFSMComponent();
	~UIFSMComponent() override;

	void Start() override;

	void OnEvent(EventType type, const void* data) override;
	bool ShouldHandleEvent(EventType type, const void* data) override;

	using Callback = std::function<void(const std::string&, const void*)>;
	using LegacyCallback = std::function<void(EventType, const void*)>;
	void RegisterCallback(const std::string& id, Callback callback);
	void RegisterCallback(const std::string& id, LegacyCallback callback);
	void ClearCallback(const std::string& id);
	void TriggerEventByName(const std::string& eventName, const void* data = nullptr);

	void SetEventCallbacks(const std::vector<UIFSMEventCallback>& callbacks) { m_EventCallbacks = callbacks; }
	const std::vector<UIFSMEventCallback>& GetEventCallbacks() const { return m_EventCallbacks; }

	void SetCallbackActions(const std::vector<UIFSMCallbackAction>& callbacks) { m_CallbackActions = callbacks; }
	const std::vector<UIFSMCallbackAction>& GetCallbackActions() const { return m_CallbackActions; }

protected:
	std::optional<std::string> TranslateEvent(EventType type, const void* data) override;
	std::optional<EventType> EventTypeFromName(const std::string& eventName) const;
	void HandleEventByName(const std::string& eventName, const void* data);

private:
	std::vector<UIFSMEventCallback> m_EventCallbacks;
	std::vector<UIFSMCallbackAction> m_CallbackActions;
	std::unordered_map<std::string, Callback> m_Callbacks;
	std::unordered_map<std::string, LegacyCallback> m_LegacyCallbacks;
	std::optional<UIRect> m_CachedBounds;

	bool m_HasTurnEndRequestAction = false;
	bool m_PendingDiceStatRollRequest = false;

	void UpdateTurnEndButtonState(Turn turn);
};

