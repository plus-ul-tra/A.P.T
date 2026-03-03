#pragma once
 
#include "NzWndBase.h"
#include <wrl/client.h>
#include "RenderData.h"
#include "Engine.h"
#include "RenderTargetContext.h"
#include <string>
#include <unordered_map>

class ServiceRegistry;
class Engine;
class Renderer;
class SceneManager;
class GameObject;
class InputManager;
class AssetLoader;
class SoundManager;
enum class Phase;


class GameApplication : public NzWndBase
{

public:
	GameApplication(ServiceRegistry& serviceRegistry, Engine& engine, Renderer& renderer, SceneManager& sceneManager, InputManager& InputManager) : 
		NzWndBase(), m_Services(serviceRegistry), m_Engine(engine), m_Renderer(renderer), 
		m_SceneManager(sceneManager), m_InputManager(InputManager) { }
	virtual ~GameApplication() = default;

	bool Initialize();
	void Run();
	void Finalize();

	bool OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
	//void UpdateInput();
	void UpdateLogic();
	void Update();
	void ApplySceneBGM();
	void Render();
	
	void OnResize(int width, int height) override;
	void OnClose() override;


	float m_GameSpeed = 1.0f;
	float m_fFrameCount = 0.0f;
	UINT64 m_FrameIndex = 0;
	
	RenderTargetContext   m_SceneRenderTarget;
	ServiceRegistry& m_Services;
	Engine&			 m_Engine;
	Renderer&		 m_Renderer;

	RenderData::FrameData m_FrameData;

	SceneManager&	 m_SceneManager;
	InputManager&    m_InputManager;

	AssetLoader* m_AssetLoader;
	SoundManager* m_SoundManager;
	std::wstring m_InitialBGMName = L"Main";
	float m_InitialBGMFadeTime = 1.0f;
	float m_SceneChangeBGMFadeTime = 1.0f;
	std::wstring m_CombatBGMName = L"Combat";
	float m_CombatBGMFadeTime = 0.5f;
	float m_DefaultBGMVolume = 0.65f;
	float m_DefaultSFXVolume = 1.2f;
	std::unordered_map<std::string, std::wstring> m_SceneBGMMap;
	std::string m_LastSceneName;
	std::wstring m_LastSceneBGMName;
	bool m_IsCombatBGMPlaying = false;
};

