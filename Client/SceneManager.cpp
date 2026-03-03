// Game / Client 용
#include "pch.h"
#include "SceneManager.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include "Scene.h"
#include "UIManager.h"
#include "ClientScene.h"
#include "DefaultScene.h"
#include "InputManager.h"
#include "Event.h"
#include "json.hpp"
#include "CameraObject.h"
#include "GameDataRepository.h"
#include "InitiativeUIComponent.h"
#include "AssetLoader.h"
#include <cctype>
#include <algorithm>

void SceneManager::Initialize()
{
	
	m_UIManager    = &m_Services.Get<UIManager>();
	m_GameManager  = &m_Services.Get<GameManager>();
	m_InputManager = &m_Services.Get<InputManager>();
	if (m_GameManager)
	{
		m_GameManager->SetServices(&m_Services);
		DataSheetPaths dataPaths{};
		dataPaths.itemsPath = "../Resources/CSV/ItemDataTable.csv";
		dataPaths.enemiesPath = "Data/enemies.csv";
		dataPaths.dropTablesPath = "../Resources/CSV/DropTable.csv";
		m_GameManager->SetDataSheetPaths(dataPaths);
		m_GameManager->SetFloorSceneNames({ "Title", "Stage1", "Stage2_Test", "Ending", "PlayerTest"});
	}
	if (m_InputManager)
	{
		m_InputManager->SetGameManager(m_GameManager);
		m_InputManager->SetEventDispatcher(m_EventDispatcher);
	}
	// Sound Manager

	//std::filesystem::path scenesPath = "../Resources/Scenes";

	// Game에서 로드할 것 여기서 명시 
	LoadGameScenesFromDirectory(scenesPath,{


		"Title",
		"Stage1",
		//"Stage2"
		"Ending",
		"DeadScene"

		//"BossStage"
		});

	//Load 실패
	if (!m_CurrentScene)
	{
		std::cerr << "No scene loaded from " << scenesPath.string() << std::endl;
	}
}

void SceneManager::Update(float deltaTime)
{
	if (!m_CurrentScene)
		return;

	if (m_CurrentScene->GetIsPause())
		deltaTime = 0.0f;

	UpdateSceneTransition(deltaTime);

	static float totalTime = 0;
	totalTime += deltaTime;

	if (totalTime >= 0.016f) {
		m_CurrentScene->FixedUpdate();
	}

	if (m_GameManager)
	{
		m_GameManager->Update(deltaTime);
	}

	if (m_UIManager)
	{
		m_UIManager->Update(deltaTime);
	}

	m_CurrentScene->Update(deltaTime);
}

void SceneManager::StateUpdate(float deltaTime)
{
	if (!m_CurrentScene)
		return;

	m_CurrentScene->StateUpdate(deltaTime);
}

void SceneManager::Reset()
{
	if (m_GameManager)
		m_GameManager->ClearEventDispatcher();
	if (m_UIManager)
		m_UIManager->Reset();
	SetEventDispatcher(nullptr);
	m_Scenes.clear();
	m_CurrentScene.reset();
	m_ChangeSceneName.clear();
	m_SceneUIData.clear();
	m_SceneTemplateData.clear();
	m_TransitionPhase = SceneTransitionPhase::None;
	m_TransitionTimer = 0.0f;
	m_TransitionTextureHandle = TextureHandle::Invalid();
	m_TransitionTextureResolved = false;
}

void SceneManager::Render(RenderData::FrameData& frameData)
{
	if (!m_CurrentScene)
	{
		return;
	}

	m_CurrentScene->Render(frameData);
	if (m_UIManager)
	{
		m_UIManager->BuildUIFrameData(frameData);
	}

	ResolveTransitionTextureHandle();

	const float viewportWidth = static_cast<float>(frameData.context.gameCamera.width);
	const float viewportHeight = static_cast<float>(frameData.context.gameCamera.height);
	if (m_TransitionPhase != SceneTransitionPhase::None && viewportWidth > 0.0f && viewportHeight > 0.0f)
	{
		const float overlayHeight = GetTransitionOverlayHeight(viewportWidth, viewportHeight);
		RenderData::UIElement overlay{};
		overlay.position = { 0.0f, GetTransitionOverlayY(overlayHeight, viewportHeight) };
		overlay.size = { viewportWidth, overlayHeight };
		overlay.rotation = GetTransitionOverlayRotation();
		overlay.color = { 0.0f, 0.0f, 0.0f, 1.0f };
		overlay.opacity = 1.0f;
		overlay.zOrder = 1000000;
		if (m_TransitionTextureHandle.IsValid())
		{
			overlay.useMaterialOverrides = true;
			overlay.materialOverrides.textureHandle = m_TransitionTextureHandle;
		}
		frameData.uiElements.push_back(overlay);
	}
}



std::shared_ptr<Scene> SceneManager::AddScene(const std::string& name, std::shared_ptr<Scene> scene)
{
	m_Scenes[name] = scene;

	m_Scenes[name]->SetGameManager(&m_Services.Get<GameManager>());
	m_Scenes[name]->SetSceneManager(this);

	return m_Scenes[name];
}

void SceneManager::SetCurrentScene(const std::string& name)
{	
	RecreateSceneFromTemplate(name);
	auto it = m_Scenes.find(name);
	if (it != m_Scenes.end())
	{
		const std::string previousSceneName = m_CurrentScene ? m_CurrentScene->GetName() : std::string{};
		if (m_UIManager && m_CurrentScene)
		{
			auto& uiMap = m_UIManager->GetUIObjects();
			auto itScene = uiMap.find(m_CurrentScene->GetName());
			if (itScene != uiMap.end())
			{
				for (const auto& [uiName, uiObject] : itScene->second)
				{
					if (!uiObject)
					{
						continue;
					}

					if (auto* initiative = uiObject->GetComponent<InitiativeUIComponent>())
					{
						initiative->DetachFromDispatcher();
					}
					uiObject->SetScene(nullptr);
				}
			}
		}

		if (m_GameManager && m_CurrentScene)
		{
			m_GameManager->CapturePlayerData(m_CurrentScene.get());
		}

		if (m_CurrentScene)
		{
			m_CurrentScene->Leave();
		}
		if (!previousSceneName.empty() && previousSceneName != name)
		{
			RecreateSceneFromTemplate(previousSceneName);
		}

		m_CurrentScene = it->second;
		m_CurrentScene->Enter();

		m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());

		m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());

		if (m_GameManager)
		{
			m_CurrentScene->SetGameManager(m_GameManager);
			m_InputManager->SetGameManager(m_GameManager);
			m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
			m_GameManager->SetActiveScene(m_CurrentScene.get());
			if (name == "Stage1")
			{
				m_GameManager->ClearPlayerData();
			}
			m_GameManager->ApplyPlayerData(m_CurrentScene.get());
			m_GameManager->TurnReset();
		}
		 
 		if (m_UIManager)
		{
 			m_UIManager->SetCurrentScene(name);
			RestoreSceneUI(m_CurrentScene);
 		}
	}
}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() const
{
	return m_CurrentScene;
}

void SceneManager::ChangeScene(const std::string& name)
{
	const std::string previousSceneName = m_CurrentScene ? m_CurrentScene->GetName() : std::string{};
	if (m_CurrentScene) {
		if (m_UIManager)
		{
			auto& uiMap = m_UIManager->GetUIObjects();
			auto itScene = uiMap.find(m_CurrentScene->GetName());
			if (itScene != uiMap.end())
			{
				for (const auto& [uiName, uiObject] : itScene->second)
				{
					if (!uiObject)
					{
						continue;
					}

					if (auto* initiative = uiObject->GetComponent<InitiativeUIComponent>())
					{
						initiative->DetachFromDispatcher();
					}
					uiObject->SetScene(nullptr);
				}
			}
			m_UIManager->ClearSceneUI(m_CurrentScene->GetName());
		}

		if (m_GameManager)
		{
			m_GameManager->CapturePlayerData(m_CurrentScene.get());
		}
		m_CurrentScene->Leave();
		if (!previousSceneName.empty() && previousSceneName != name)
		{
			RecreateSceneFromTemplate(previousSceneName);
		}
	}

	RecreateSceneFromTemplate(name);
	auto it = m_Scenes.find(name);

	if (it != m_Scenes.end())
	{
		m_CurrentScene = it->second;
		m_CurrentScene->Enter();
		if (m_InputManager)
		{
			m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		}
		SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		//UI 생기면 그때
		if (m_UIManager)
		{
			m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
		}

		if (m_GameManager)
		{
			m_CurrentScene->SetGameManager(m_GameManager);
			m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
			if (name == "Stage1")
			{
				m_GameManager->ClearPlayerData();
			}
			m_GameManager->ApplyPlayerData(m_CurrentScene.get());
		}

		if (m_UIManager)
		{
			m_UIManager->SetCurrentScene(name);
			RestoreSceneUI(m_CurrentScene);
		}
	}
}

void SceneManager::ChangeScene()
{
	if (m_ChangeSceneName.empty() || !m_CurrentScene)
	{
		return;
	}

	if (m_ChangeSceneName == m_CurrentScene->GetName())
	{
		m_ChangeSceneName.clear();
		return;
	}

	StartSceneTransition(m_ChangeSceneName);
}

void SceneManager::SetChangeScene(const std::string& name)
{
	m_ChangeSceneName = name;
}

void SceneManager::SetEventDispatcher(EventDispatcher* eventDispatcher)
{
	if (m_EventDispatcher == eventDispatcher)
	{
		return;
	}

	if (m_EventDispatcher)
	{
		m_EventDispatcher->RemoveListener(EventType::SceneChangeRequested, this);
	}

	m_EventDispatcher = eventDispatcher;

	if (m_EventDispatcher)
	{
		m_EventDispatcher->AddListener(EventType::SceneChangeRequested, this);
	}
}

void SceneManager::OnEvent(EventType type, const void* data)
{
	if (type != EventType::SceneChangeRequested || !data)
	{
		return;
	}

	const auto* request = static_cast<const Events::SceneChangeRequest*>(data);
	if (!request)
	{
		return;
	}

	SetChangeScene(request->name);
	ChangeScene();
}

void SceneManager::StartSceneTransition(const std::string& name)
{
	if (name.empty() || m_TransitionPhase != SceneTransitionPhase::None)
	{
		return;
	}

	m_ChangeSceneName = name;
	m_TransitionPhase = SceneTransitionPhase::FadeOut;
	m_TransitionTimer = 0.0f;
	m_SceneSwapPending = false;
}

void SceneManager::UpdateSceneTransition(float deltaTime)
{
	if (m_TransitionPhase == SceneTransitionPhase::None)
	{
		return;
	}

	m_TransitionTimer += std::max(0.0f, deltaTime);

	if (m_TransitionPhase == SceneTransitionPhase::FadeOut)
	{
		if (m_FadeOutDuration <= 0.0f)
		{
			std::string nextScene = m_ChangeSceneName;
			m_TransitionPhase = SceneTransitionPhase::FadeIn;
			m_TransitionTimer = 0.0f;
			m_SceneSwapPending = false;
			ChangeScene(nextScene);
			m_ChangeSceneName.clear();
			return;
		}

		if (m_SceneSwapPending)
		{
			std::string nextScene = m_ChangeSceneName;
			m_TransitionPhase = SceneTransitionPhase::FadeIn;
			m_TransitionTimer = 0.0f;
			m_SceneSwapPending = false;
			ChangeScene(nextScene);
			m_ChangeSceneName.clear();
			return;
		}

		if (m_TransitionTimer >= m_FadeOutDuration)
		{
			m_TransitionTimer = m_FadeOutDuration;
			m_SceneSwapPending = true;
		}
		return;
	}

	if (m_TransitionPhase == SceneTransitionPhase::FadeIn)
	{
		if (m_TransitionTimer >= m_FadeInDuration)
		{
			m_TransitionPhase = SceneTransitionPhase::None;
			m_TransitionTimer = 0.0f;
		}
	}
}

void SceneManager::ResolveTransitionTextureHandle()
{
	if (m_TransitionTextureResolved)
	{
		return;
	}

	m_TransitionTextureResolved = true;
	m_TransitionTextureHandle = TextureHandle::Invalid();

	auto equalsIgnoreCase = [](const std::string& a, const std::string& b)
		{
			if (a.size() != b.size())
			{
				return false;
			}
			for (size_t i = 0; i < a.size(); ++i)
			{
				if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
				{
					return false;
				}
			}
			return true;
		};

	auto endsWithIgnoreCase = [&](const std::string& value, const std::string& suffix)
		{
			if (suffix.size() > value.size())
			{
				return false;
			}
			const std::string tail = value.substr(value.size() - suffix.size());
			return equalsIgnoreCase(tail, suffix);
		};

	auto& loader = m_Services.Get<AssetLoader>();
	const auto& keyToHandle = loader.GetTextures().GetKeyToHandle();
	for (const auto& [key, handle] : keyToHandle)
	{
		if (!handle.IsValid())
		{
			continue;
		}

		const std::string* displayName = loader.GetTextures().GetDisplayName(handle);
		if (displayName && equalsIgnoreCase(*displayName, "UI/fadeBlack"))
		{
			m_TransitionTextureHandle = handle;
			return;
		}

		if (endsWithIgnoreCase(key, "/fadeBlack.png") || endsWithIgnoreCase(key, "\\fadeBlack.png"))
		{
			m_TransitionTextureHandle = handle;
			return;
		}
	}
}


float SceneManager::GetTransitionOverlayHeight(float viewportWidth, float viewportHeight) const
{
	if (viewportWidth <= 0.0f || viewportHeight <= 0.0f)
	{
		return viewportHeight;
	}

	if (!m_TransitionTextureHandle.IsValid())
	{
		return viewportHeight;
	}

	constexpr float kFadeTextureWidth = 2560.0f;
	constexpr float kFadeTextureHeight = 3200.0f;
	constexpr float kMinTextureWidth = 1.0f;
	const float scaledHeight = viewportWidth * (kFadeTextureHeight / std::max(kFadeTextureWidth, kMinTextureWidth));
	return std::max(viewportHeight, scaledHeight);
}

float SceneManager::GetTransitionOverlayY(float overlayHeight, float viewportHeight) const
{
	if (overlayHeight <= 0.0f || viewportHeight <= 0.0f)
	{
		return 0.0f;
	}

	const float coveredY = -(overlayHeight - viewportHeight);
	const float fadeOutStartY = -overlayHeight + m_FadeOutStartOffsetY;
	const float fadeOutEndY = coveredY + m_FadeOutEndOffsetY;
	const float fadeInStartY = coveredY + m_FadeInStartOffsetY;
	const float fadeInEndY = -overlayHeight + m_FadeInEndOffsetY;

	if (m_TransitionPhase == SceneTransitionPhase::FadeOut)
	{
		if (m_FadeOutDuration <= 0.0f)
		{
			return fadeOutEndY;
		}
		const float t = std::clamp(m_TransitionTimer / m_FadeOutDuration, 0.0f, 1.0f);
		return fadeOutStartY + (fadeOutEndY - fadeOutStartY) * t;
	}

	if (m_TransitionPhase == SceneTransitionPhase::FadeIn)
	{
		if (m_FadeInDuration <= 0.0f)
		{
			return fadeInEndY;
		}
		const float t = std::clamp(m_TransitionTimer / m_FadeInDuration, 0.0f, 1.0f);
		return fadeInStartY - (fadeInEndY - fadeInStartY) * t;
	}

	return fadeInEndY;
}

float SceneManager::GetTransitionOverlayRotation() const
{
	if (m_TransitionPhase == SceneTransitionPhase::FadeIn)
	{
		return 180.0f;
	}

	return 0.0f;
}
void SceneManager::LoadGameScenesFromDirectory(const std::filesystem::path& directoryPath, const std::vector<std::string>& sceneNames)
{

	if (directoryPath.empty() || !std::filesystem::exists(directoryPath))
	{
		std::cout << "Invalid directory" << std::endl;
		return;
	}

	for (const auto& sceneName : sceneNames)
	{
		std::filesystem::path scenePath =
			directoryPath / (sceneName + ".json");
		if (!std::filesystem::exists(scenePath))
		{
			std::cout << "Scene not found: " << sceneName << std::endl;
			continue;
		}

		std::cout << scenePath << " Scene Find" << std::endl;
		LoadGameSceneFromJson(scenePath);
	}

	// 첫 번째 Scene을 Entry Scene으로 설정
	if (!m_CurrentScene && !sceneNames.empty())
	{
		SetCurrentScene(sceneNames.front());
	}

}

bool SceneManager::LoadGameSceneFromJson(const std::filesystem::path& filepath)
{
	if (filepath.empty()) {
		std::cout << "No Scene Name"<<std::endl;
		return false;
	}

	std::ifstream ifs(filepath);

	if (!ifs.is_open()) {
		return false;
	}

	nlohmann::json j;
	ifs >> j;

	const std::string sceneName = filepath.stem().string();
	m_SceneTemplateData[sceneName] = j;
	auto loadedScene = std::make_shared<ClientScene>(m_Services);
	//loadedScene->SetName(filepath.stem().string());
	loadedScene->SetName(sceneName);
	loadedScene->Initialize();
	loadedScene->Deserialize(j);
	loadedScene->SetIsPause(false);
	AddScene(loadedScene->GetName(), loadedScene);
	if (!m_CurrentScene)
	{
		SetCurrentScene(loadedScene->GetName());
	}

	if (m_UIManager && j.contains("ui"))
	{
		m_SceneUIData[loadedScene->GetName()] = j.at("ui");
		m_UIManager->SetEventDispatcher(&loadedScene->GetEventDispatcher());
		m_UIManager->DeserializeSceneUI(loadedScene->GetName(), j.at("ui"));
		auto& uiMap = m_UIManager->GetUIObjects();
		auto itScene = uiMap.find(loadedScene->GetName());
		if (itScene != uiMap.end())
		{
			for (const auto& [name, uiObject] : itScene->second)
			{
				if (uiObject)
				{
					uiObject->SetScene(loadedScene.get());
					uiObject->Start();
				}
			}
		}
	}

	return true;

}

void SceneManager::RestoreSceneUI(const std::shared_ptr<Scene>& scene)
{
	if (!m_UIManager || !scene)
	{
		return;
	}

	const auto& sceneName = scene->GetName();
	auto itData = m_SceneUIData.find(sceneName);
	if (itData == m_SceneUIData.end())
	{
		return;
	}

	m_UIManager->DeserializeSceneUI(sceneName, itData->second);
	auto& uiMap = m_UIManager->GetUIObjects();
	auto itScene = uiMap.find(sceneName);
	if (itScene != uiMap.end())
	{
		for (const auto& [name, uiObject] : itScene->second)
		{
			if (uiObject)
			{
				uiObject->SetScene(scene.get());
				uiObject->Start();
			}
		}
	}
}

void SceneManager::RecreateSceneFromTemplate(const std::string& name)
{
	auto itTemplate = m_SceneTemplateData.find(name);
	if (itTemplate == m_SceneTemplateData.end())
	{
		return;
	}

	auto freshScene = std::make_shared<ClientScene>(m_Services);
	freshScene->SetName(name);
	freshScene->Initialize();
	freshScene->Deserialize(itTemplate->second);
	freshScene->SetIsPause(false);
	freshScene->SetGameManager(&m_Services.Get<GameManager>());
	freshScene->SetSceneManager(this);
	m_Scenes[name] = freshScene;
}
