#include "FSM.h"
#include <algorithm>

void FSM::AddTransition(const std::string& from, const std::string& to, const std::string& evt, std::function<bool()> condition, int priority)
{
	Transition transition;
	transition.from      = from;
	transition.to        = to;
	transition.onEvent   = evt;
	transition.condition = condition ? std::move(condition) : []() { return true; };
	transition.priority  = priority;
	m_Transitions[from][evt].push_back(std::move(transition));
}

void FSM::Update(float deltaTime)
{
	auto it = m_States.find(m_CurrentState);
	if (it != m_States.end() && it->second.OnUpdate)
	{
		it->second.OnUpdate(deltaTime);
	}
}

void FSM::SetInitialState(const StateID& id)
{
	auto it = m_States.find(id);
	if (it == m_States.end())
	{
		return;
	}

	m_CurrentState = id;
	if (it->second.OnEnter)
	{
		it->second.OnEnter();
	}
}

void FSM::Trigger(const std::string& evt)
{
	auto it = m_Transitions.find(m_CurrentState);
	if (it == m_Transitions.end())
		return;

	auto it2 = it->second.find(evt);
	if (it2 == it->second.end())
		return;

	const auto& transitions = it2->second;
	if (transitions.empty())
		return;

	const Transition* best = nullptr;

	for (const auto& transition : transitions)
	{
		if (transition.condition && !transition.condition())
			continue;

		if (!best || transition.priority > best->priority)
			best = &transition;
	}

	if (best)
		ChangeState(best->to);
}

void FSM::ChangeState(const StateID& id)
{
	if (m_CurrentState == id) return;

	if (m_States[m_CurrentState].OnExit)
	{
		m_States[m_CurrentState].OnExit();
	}

	m_CurrentState = id;

	if (m_States[m_CurrentState].OnEnter)
	{
		m_States[m_CurrentState].OnEnter();
	}
}
