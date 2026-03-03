#include "FSMComponent.h"
#include "ReflectionMacro.h"
#include "FSMEventRegistry.h"
#include "FSMActionRegistry.h"
#include <iostream>

REGISTER_COMPONENT(FSMComponent)
REGISTER_PROPERTY(FSMComponent, Graph)
REGISTER_PROPERTY(FSMComponent, CurrentStateName)

void RegisterFSMBaseDefinitions()
{
	auto& actionRegistry = FSMActionRegistry::Instance();
	actionRegistry.RegisterAction({
		"None",
		"FSM",
		{
			{ "value", "bool", true, false }
		}
		});

	auto& eventRegistry = FSMEventRegistry::Instance();
	eventRegistry.RegisterEvent({ "None", "Common" });
}

void FSMComponent::Start()
{
	RebuildRuntimeFSM();
	if (m_CurrentStateName.empty() && !m_Graph.initialState.empty())
	{
		if (const auto* state = FindState(m_Graph.initialState))
		{
			EnterState(*state, true);
		}
	}
}

void FSMComponent::Update(float deltaTime)
{
	m_RuntimeFSM.Update(deltaTime);
}

void FSMComponent::OnEvent(EventType type, const void* data)
{
	if (auto eventName = TranslateEvent(type, data))
	{
		DispatchEvent(*eventName);
	}
}

void FSMComponent::DispatchEvent(const std::string& eventName)
{
	m_RuntimeFSM.Trigger(eventName);
}

void FSMComponent::SetGraph(const FSMGraph& graph)
{
	m_Graph = graph;
	RebuildRuntimeFSM();
}

void FSMComponent::SetCurrentStateName(const std::string& stateName)
{
	m_CurrentStateName = stateName;
	RebuildRuntimeFSM();
}

void FSMComponent::BindActionHandler(const std::string& actionId, ActionHandler handler)
{
	m_ActionHandlers[actionId] = std::move(handler);
}

std::optional<std::string> FSMComponent::TranslateEvent(EventType, const void*)
{
	return std::nullopt;
}

void FSMComponent::HandleAction(const FSMAction& action)
{
	auto it = m_ActionHandlers.find(action.id);
	if (it == m_ActionHandlers.end())
	{
		std::cout << "[FSMComponent] Missing action handler: " << action.id << std::endl;
		return;
	}

	it->second(action);
}

const FSMState* FSMComponent::FindState(const std::string& stateName) const
{
	for (const auto& state : m_Graph.states)
	{
		if (state.name == stateName)
		{
			return &state;
			{

			};
		}
	}

	return nullptr;
}

void FSMComponent::EnterState(const FSMState& state, bool runActions)
{
	m_CurrentStateName = state.name;
	if (runActions)
	{
		ExecuteAction(state.onEnter);
	}
}

void FSMComponent::ExecuteAction(const std::vector<FSMAction>& actions)
{
	for (const auto& action : actions)
	{
		HandleAction(action);
	}
}

void FSMComponent::RebuildRuntimeFSM()
{
	m_RuntimeFSM = FSM{};

	for (const auto& state : m_Graph.states)
	{
		State runtimeState;
		runtimeState.OnEnter = [this, stateName = state.name]()
			{
				if (const auto* current = FindState(stateName))
				{
					EnterState(*current, true);
				}
			};

		runtimeState.OnExit = [this, stateName = state.name]()
			{
				if (const auto* current = FindState(stateName))
				{
					ExecuteAction(current->onExit);
				}
			};

		runtimeState.OnUpdate = nullptr;
		m_RuntimeFSM.AddState(state.name, runtimeState);

		for (const auto& transition : state.transitions)
		{
			if (!transition.eventName.empty() && !transition.targetState.empty())
			{
				m_RuntimeFSM.AddTransition(state.name, transition.targetState, transition.eventName, transition.priority);
			}
		}
	}

	if (!m_CurrentStateName.empty())
	{
		m_RuntimeFSM.SetInitialState(m_CurrentStateName);
	}
	else if (!m_Graph.initialState.empty())
	{
		m_RuntimeFSM.SetInitialState(m_Graph.initialState);
	}
}
