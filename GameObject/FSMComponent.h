#pragma once
#include "Component.h"
#include "FSM.h"
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

struct FSMAction
{
	std::string id;
	nlohmann::json params;
};

struct FSMTransition
{
	std::string eventName;
	std::string targetState;
	int priority = 0;
};

struct FSMState
{
	std::string				   name;
	std::vector<FSMAction>	   onEnter;
	std::vector<FSMAction>	   onExit;
	std::vector<FSMTransition> transitions;
};

struct FSMGraph
{
	std::vector<FSMState> states;
	std::string			  initialState;
};

// 에디터 저장/로드 예시(JSON)
// {
//   "type": "FSMComponent",
//   "Graph": {
//     "initialState": "Idle",
//     "states": [
//       {
//         "name": "Idle",
//         "onEnter": [
//           { "id": "Anim_Play", "params": { "clip": "Idle", "loop": true } }
//         ],
//         "onExit": [
//           { "id": "SFX_Play", "params": { "key": "Stop_Idle" } }
//         ],
//         "transitions": [
//           { "event": "UI_Pressed", "target": "Pressed", "priority": 0 }
//         ]
//       }
//     ]
//   },
//   "CurrentStateName": "Idle"
// }
//
// 액션 파라미터 직렬화 예시(params는 자유 JSON):
// - { "id": "UI_SetOpacity", "params": { "value": 0.4 } }
// - { "id": "Anim_SetSpeed", "params": { "value": 1.2 } }
// - { "id": "SetComponentActive", "params": { "component": "HitBox", "active": false } }

class FSMComponent : public Component, public IEventListener
{
public:
	static constexpr const char* StaticTypeName = "FSMComponent";
	const char* GetTypeName() const override;

	using ActionHandler = std::function<void(const FSMAction&)>;

	void Start() override;
	void Update(float deltaTime) override;
	void OnEvent(EventType type, const void* data) override;

	void DispatchEvent(const std::string& eventName);

	void SetGraph(const FSMGraph& graph);
	const FSMGraph& GetGraph() const { return m_Graph; }

	void SetCurrentStateName(const std::string& stateName);
	const std::string& GetCurrentStateName() const { return m_CurrentStateName; }

	void BindActionHandler(const std::string& actionId, ActionHandler handler);

protected:
	virtual std::optional<std::string> TranslateEvent(EventType type, const void* data);
	virtual void HandleAction(const FSMAction& action);

private:
	const FSMState* FindState(const std::string& stateName) const;
	void EnterState(const FSMState& state, bool runActions);
	void ExecuteAction(const std::vector<FSMAction>& actions);
	void RebuildRuntimeFSM();

	FSMGraph									   m_Graph;
	std::string									   m_CurrentStateName;
	std::unordered_map<std::string, ActionHandler> m_ActionHandlers;
	FSM											   m_RuntimeFSM;
};

