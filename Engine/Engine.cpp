#include "pch.h"
#include "Engine.h"
#include "ServiceRegistry.h"
#include "InputManager.h"
#include "Renderer.h"

void Engine::Initailize()
{
	m_Device		  =	std::make_unique<Device>();
	m_GameTimer.Reset();
	m_InputManager = &m_Services.Get<InputManager>();
}

void Engine::UpdateTime()
{
	m_GameTimer.Tick();
}

float Engine::GetTime() const
{
	return m_GameTimer.DeltaTime();
}


void Engine::UpdateInput()
{
	m_InputManager->Update();
}

//아직 생성은 안함
void Engine::CreateDevice(HWND hwnd) {
	if (!m_Device) {
		// 없다면 생성 시도
		m_Device = std::make_unique<Device>();
	}
	//실제 Device class 객체 생성
	m_Device->Create(hwnd);
}
