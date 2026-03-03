#pragma once
#include "GameTimer.h"
#include <memory>
#include "Device.h"

class ServiceRegistry;
class Renderer;
class Device;
class InputManager;

class Engine
{
public:
	Engine(ServiceRegistry& serviceRegistry, Renderer& renderer) : m_Services(serviceRegistry) { Initailize(); }
	~Engine() = default;
	
	void  UpdateTime  ();
	float GetTime     () const;
	void  UpdateInput ();
	void  CreateDevice(HWND hwnd);

	// 개별 Device, DXDC Get 
	ID3D11Device*			 Get3DDevice() const	  { return m_Device ? m_Device->GetDevice().Get() : nullptr; }
	ID3D11DeviceContext*	 GetD3DDXDC()  const	  { return m_Device ? m_Device->GetDXDC().Get() : nullptr; }

	GameTimer& GetTimer() { return m_GameTimer; }

	void Reset()
	{
		m_Device.reset();
	}

	ServiceRegistry& GetServices()			   { return m_Services; }
	const ServiceRegistry& GetServices() const { return m_Services; }

private:
	void Initailize();

private:
	ServiceRegistry& m_Services;
	GameTimer		 m_GameTimer;
	InputManager*	 m_InputManager;
	std::unique_ptr<Device>			 m_Device		   = nullptr; 	//Device 에 member로 Device, DXDC 존재
};

