
#include "pch.h"
#include "GameApplication.h"
#include "GameObject.h"
//#include "Reflection.h"
#include "Engine.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "ServiceRegistry.h"
#include "SoundManager.h"
#include "InputManager.h"
#include "CameraComponent.h"
#include "CameraObject.h"
#include "UIManager.h"


bool GameApplication::Initialize()
{
	const wchar_t* className = L"APT";
	const wchar_t* windowName = L"APT";

	if (false == Create(className, windowName, 2560, 1600)) // 해상도 변경
	{
		return false;
	}
	m_Engine.CreateDevice(m_hwnd);

	m_AssetLoader = &m_Services.Get<AssetLoader>();
	m_AssetLoader->LoadAll();
	m_SoundManager = &m_Services.Get<SoundManager>();
	m_SoundManager->Init();
	m_SoundManager->CreateBGMSource(m_AssetLoader->GetBGMPaths());
	m_SoundManager->CreateSFXSource(m_AssetLoader->GetSFXPaths());
	m_SoundManager->SetDirty();
	m_SoundManager->SetVolume_BGM(m_DefaultBGMVolume);
	m_SoundManager->SetVolume_SFX(m_DefaultSFXVolume);
	m_SceneBGMMap.clear();

	// Scene별 곡 등록
	// 별도 등록하지 않으면 직전 Scene의 BGM 계속 Loop
	m_SceneBGMMap.emplace("Title", L"Title");
	m_SceneBGMMap.emplace("Stage1", L"Idle");
	//m_SceneBGMMap.emplace("Stage2", L"Title");

	OnResize(m_width, m_height);
	auto& uiManager = m_Services.Get<UIManager>();
	uiManager.SetReferenceResolution(UISize{ 2560.0f, 1600.0f });
	uiManager.SetUseAnchorLayout(false);
	uiManager.SetUseResolutionScale(true);
	m_Renderer.Initialize(m_hwnd, m_width, m_height, m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());
	m_SceneManager.Initialize();
	// GameManager에 SceneManager 등록

	m_SceneRenderTarget.SetDevice(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());

	return true;
}

void GameApplication::Run()
{
	MSG msg = { 0 };

	while (WM_QUIT != msg.message && !m_SceneManager.ShouldQuit())
	{
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			if (false == m_InputManager.OnHandleMessage(msg)) {
				TranslateMessage(&msg);
			}
			DispatchMessage(&msg);
		}
		else
		{
			m_Engine.UpdateTime();
			Update();
			m_Engine.UpdateInput();
			UpdateLogic();
			Render();
		}
	}
}

void GameApplication::Finalize()
{
	__super::Destroy();
}

bool GameApplication::OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	return false;
}

void GameApplication::UpdateLogic()
{
	m_SceneManager.ChangeScene();
	ApplySceneBGM();
}

void GameApplication::Update()
{
	ApplySceneBGM();
	float dTime = m_Engine.GetTime();
	dTime *= m_GameSpeed;
	//m_Engine.UpdateInput();
	m_SceneManager.StateUpdate(dTime);
	m_SceneManager.Update(dTime);
	
	m_Services.Get<SoundManager>().Update();
	// FixedUpdate
	{

		while (m_fFrameCount >= 0.016f)
		{
			m_fFrameCount -= 0.016f;
		}

	}
}

void GameApplication::ApplySceneBGM()
{
	auto currentScene = m_SceneManager.GetCurrentScene();
	if (!currentScene)
	{
		return;
	}

	const std::string sceneName = currentScene->GetName();
	if (sceneName == m_LastSceneName)
	{
		return;
	}

	m_LastSceneName = sceneName;
	auto it = m_SceneBGMMap.find(sceneName);
	if (it == m_SceneBGMMap.end())
	{
		return;
	}

	m_SoundManager->BGM_Shot(it->second, m_SceneChangeBGMFadeTime);
}

void GameApplication::Render()
{
	if (!m_Engine.GetD3DDXDC()) return;
	//m_Engine.GetRenderer().RenderBegin();

	ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
	m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
	SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());

	ClearBackBuffer(COLOR(0.12f, 0.12f, 0.12f, 1.0f), m_Engine.GetD3DDXDC(), *rtvs);

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}
	// 현재 Scene의 Camera 받기
	if (auto gameCamera = scene->GetGameCamera())
	{
		if (auto* cameraComponent = gameCamera->GetComponent<CameraComponent>())
		{
			cameraComponent->SetViewport({ static_cast<float>(m_width), static_cast<float>(m_height) });
		}
	}

	m_FrameData.context.frameIndex = static_cast<UINT32>(m_FrameIndex++);
	m_FrameData.context.deltaTime = m_Engine.GetTimer().DeltaTime();
	m_SceneManager.Render(m_FrameData);
	m_Renderer.RenderFrame(m_FrameData);
	m_Renderer.RenderToBackBuffer();
	Flip(m_Renderer.GetSwapChain().Get());
}


void GameApplication::OnResize(int width, int height)
{
	__super::OnResize(width, height);
	m_InputManager.SetViewportRect({ 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) });
	m_Services.Get<UIManager>().SetViewportSize(UISize{ static_cast<float>(width), static_cast<float>(height) });
}

void GameApplication::OnClose()
{
}
