#include "AIController.h"

AIController::AIController(BTExecutor& executor) : m_Executor(executor)
{
}

void AIController::Initialize()
{
	if (m_IsInitialized)
		return;

	m_Executor.Build(m_Instance);
	m_IsInitialized = true;
}

void AIController::Tick(float deltaTime)
{
	if (!m_IsInitialized)
		Initialize();

	m_Executor.Tick(m_Instance, m_Blackboard, deltaTime);
}
