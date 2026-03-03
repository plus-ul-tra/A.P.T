//editor
#include "pch.h"
#include "SceneManager.h"
#include "DefaultScene.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include "Renderer.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include "InputManager.h"
#include "UIManager.h"
#include "CameraObject.h"
#include "GameDataRepository.h"
#include "InitiativeUIComponent.h"

//editor 용으로 개발 필요함 - 편집할 Scene 선택, 생성
void SceneManager::Initialize()
{
	/*m_UIManager.Start();
	m_UIManager.SetCurrentScene("TitleScene");*/

	// 씬없는 경우.. 인데
	// ★★★★★★★★★★★★★★
	// Editor의 경우 m_Scenes 로 여러개를 들고 있을 필요가 없음. -> 이제 있음 Play모드 시 게임 흐름 보려면 
	// 자기가 쓰는 Scene 하나만 있으면됨.
	// 다른 Scene의 경우 경로에서 Scenedata json으로 띄우기만 하면 됨.
	// 그냥 Default 생성
	m_InputManager = &m_Services.Get<InputManager>();
	m_UIManager = &m_Services.Get<UIManager>();
	m_GameManager = &m_Services.Get<GameManager>();

	if (m_GameManager)
	{
		m_GameManager->SetServices(&m_Services);
		DataSheetPaths dataPaths{};
		dataPaths.itemsPath = "../Resources/CSV/ItemDataTable.csv";
		dataPaths.enemiesPath = "Data/enemies.csv";
		dataPaths.dropTablesPath = "../Resources/CSV/DropTable.csv";
		m_GameManager->SetDataSheetPaths(dataPaths);
		m_GameManager->SetFloorSceneNames({ "Stage1 ", "Stage2 ", "Stage3 ", "Ending " });
	}
	if (m_InputManager)
	{
		m_InputManager->SetGameManager(m_GameManager);
	}

	auto emptyScene = std::make_shared<DefaultScene>(m_Services);
	emptyScene->SetSceneManager(this);
	emptyScene->SetName("Untitled Scene");
	emptyScene->Initialize();
	emptyScene->SetIsPause(true);
	emptyScene->Enter();
	// editor는 Current Scene만 띄운다.
	SetCurrentScene(emptyScene);
}

void SceneManager::Update(float deltaTime)
{
	if (!m_CurrentScene)
		return;
	if (m_CurrentScene->GetIsPause())
	{
		return;
	}

	static float totalTime = 0;
	totalTime += deltaTime;

	if (totalTime >= 0.016f)
		m_CurrentScene->FixedUpdate();

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
}

void SceneManager::Render()
{
	if (!m_CurrentScene)
	{
		return;
	}

	RenderData::FrameData frameData{};
	m_CurrentScene->Render(frameData);
	if (m_UIManager)
	{
		m_UIManager->BuildUIFrameData(frameData);
	}
	/*std::vector<RenderInfo> renderInfo;
	std::vector<UIRenderInfo> uiRenderInfo;
	std::vector<UITextInfo> uiTextInfo;
	m_CurrentScene->Render(renderInfo, uiRenderInfo, uiTextInfo);
	m_Renderer.Draw(renderInfo, uiRenderInfo, uiTextInfo);*/
}


void SceneManager::SetCurrentScene(std::shared_ptr<Scene> scene)
{
	auto oldScene = m_CurrentScene;
	if (m_GameManager && m_CurrentScene)
	{
		m_GameManager->CapturePlayerData(m_CurrentScene.get());
	}

	if (m_UIManager && oldScene)
	{
		auto& uiMap = m_UIManager->GetUIObjects();
		auto itScene = uiMap.find(oldScene->GetName());
		if (itScene != uiMap.end())
		{
			for (const auto& [name, uiObject] : itScene->second)
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


	m_CurrentScene = scene;
	m_CurrentScene->Enter();
	m_Camera = m_CurrentScene->GetGameCamera();
	m_InputManager->SetViewportRect({ 0, 0, static_cast<LONG>(m_Camera->GetViewportSize().Width), static_cast<LONG>(m_Camera->GetViewportSize().Height) });


	m_InputManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
	m_InputManager->ResetState();


	m_UIManager->SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());
	m_UIManager->SetCurrentScene(m_CurrentScene->GetName());
	m_UIManager->RefreshUIListForCurrentScene();
	SetEventDispatcher(&m_CurrentScene->GetEventDispatcher());

	if (m_GameManager)
	{
		m_CurrentScene->SetGameManager(m_GameManager);
		m_InputManager->SetGameManager(m_GameManager);
		m_GameManager->SetEventDispatcher(m_CurrentScene->GetEventDispatcher());
		m_GameManager->SetActiveScene(m_CurrentScene.get());
		m_GameManager->ApplyPlayerData(m_CurrentScene.get());
	}

}

std::shared_ptr<Scene> SceneManager::GetCurrentScene() const
{
	return m_CurrentScene;
}

void SceneManager::ChangeScene(const std::string& name)
{
	if (name.empty())
	{
		return;
	}

	if (!m_CurrentScene)
	{
		return;
	}

	if (m_CurrentScene->GetName() == name)
	{
		return;
	}

	const auto* scenePath = FindScenePathByName(name);
	if (!scenePath)
	{
		return;
	}
	const bool wasPaused = m_CurrentScene->GetIsPause();
	if(m_GameManager)
	{
		m_GameManager->CapturePlayerData(m_CurrentScene.get());
	}
	m_CurrentScene->Leave();
	if (LoadSceneFromJson(*scenePath))
	{
		m_CurrentScene->SetIsPause(wasPaused);
	}
}

void SceneManager::ChangeScene()
{
	if (!m_CurrentScene)
	{
		return;
	}

	if (m_ChangeSceneName != m_CurrentScene->GetName())
	{
		ChangeScene(m_ChangeSceneName);
		m_ChangeSceneName.clear();
	}

}

void SceneManager::SetChangeScene(std::string name)
{
	m_ChangeSceneName = name;
}


const std::filesystem::path* SceneManager::FindScenePathByName(const std::string& name) const
{
	if (name.empty())
	{
		return nullptr;
	}

	auto it = m_Scenes.find(name);
	if (it == m_Scenes.end())
	{
		return nullptr;
	}
	return &it->second;
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
}

bool SceneManager::CreateNewScene(const std::filesystem::path& filePath)
{
	if (filePath.empty())
	{
		return false;
	}

	auto newScene = std::make_shared<DefaultScene>(m_Services);
	newScene->SetSceneManager(this);
	newScene->SetName(filePath.stem().string());
	newScene->Initialize();
	newScene->SetIsPause(true);
	//AddOrReplaceScene(newScene);
	SetCurrentScene(newScene);
	m_CurrentScenePath = filePath;

	return SaveSceneToJson(filePath);
}


bool SceneManager::LoadSceneFromJson(const std::filesystem::path& filePath)
{	// 선택했을때 해당 Scene을 Deserialize 해서 
	// 현재 씬으로
	if (filePath.empty())
	{
		return false;
	}

	std::ifstream ifs(filePath);
	if (!ifs.is_open())
	{
		return false;
	}

	nlohmann::json j;
	ifs >> j;

	auto loadedScene = std::make_shared<DefaultScene>(m_Services);
	loadedScene->SetName(filePath.stem().string());
	loadedScene->Initialize();
	loadedScene->Deserialize(j);
	loadedScene->SetIsPause(true);
	//AddOrReplaceScene(loadedScene);
	SetCurrentScene(loadedScene);
	m_CurrentScenePath = filePath;

	if (m_UIManager && j.contains("ui"))
	{
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

bool SceneManager::LoadSceneFromJsonData(const nlohmann::json& data, const std::filesystem::path& filePath)
{
	if(data.is_null())
		return false;

	auto loadedScene = std::make_shared<DefaultScene>(m_Services);
	if (!filePath.empty())
	{
		loadedScene->SetName(filePath.stem().string());
	}
	else
	{
		loadedScene->SetName("Untitled Scene");
	}

	loadedScene->Initialize();
	loadedScene->Deserialize(data);
	loadedScene->SetIsPause(true);
	//AddOrReplaceScene(loadedScene);
	SetCurrentScene(loadedScene);
	m_CurrentScenePath = filePath;

	if (m_UIManager && data.contains("ui"))
	{
		m_UIManager->DeserializeSceneUI(loadedScene->GetName(), data.at("ui"));
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

bool SceneManager::SaveSceneToJson(const std::filesystem::path& filePath)const
{	
	// 같은 이름이 있는경우 그냥 덮어쓰기 할것
	if (!m_CurrentScene)
	{
		return false;
	}

	const_cast<Scene*>(m_CurrentScene.get())->EnsureAutoComponentsForSave();

	nlohmann::json currentData;
	m_CurrentScene->Serialize(currentData);

	if (m_UIManager)
	{
		nlohmann::json uiData;
		m_UIManager->SerializeSceneUI(m_CurrentScene->GetName(), uiData);
		currentData["ui"] = uiData;
	}

	nlohmann::json mergedData = currentData;
	if (std::filesystem::exists(filePath))
	{
		std::ifstream ifs(filePath);
		if (ifs.is_open())
		{
			try
			{
				ifs >> mergedData;
			}
			catch (const nlohmann::json::exception&)
			{
				mergedData = nlohmann::json::object();
			}
		}
	}

	auto mergeNamedArray = [](const nlohmann::json& base, const nlohmann::json& incoming)
		{
			if (!incoming.is_array())
			{
				return base.is_array() ? base : nlohmann::json::array();
			}

			nlohmann::json result = nlohmann::json::array();
			std::unordered_map<std::string, nlohmann::json> baseByName;
			if (base.is_array())
			{
				for (const auto& entry : base)
				{
					if (entry.contains("name"))
					{
						baseByName[entry.at("name").get<std::string>()] = entry;
					}
				}
			}
			for (const auto& entry : incoming)
			{
				if (!entry.contains("name"))
				{
					continue;
				}

				const auto name = entry.at("name").get<std::string>();
				result.push_back(entry);
				baseByName.erase(name);
			}

			return result;
		};

	if (!mergedData.is_object())
	{
		mergedData = nlohmann::json::object();
	}

	mergedData["editor"] = currentData.value("editor", nlohmann::json::object());
	mergedData["gameObjects"] = mergeNamedArray(mergedData.value("gameObjects", nlohmann::json::array()),
		currentData.value("gameObjects", nlohmann::json::array()));
	const nlohmann::json currentUi = currentData.value("ui", nlohmann::json::array());
	if (currentUi.is_object())
	{
		mergedData["ui"] = currentUi;
	}
	else
	{
		mergedData["ui"] = mergeNamedArray(mergedData.value("ui", nlohmann::json::array()), currentUi);
	}

	std::ofstream ofs(filePath);
	if (!ofs.is_open())
	{
		return false;
	}

	ofs << mergedData.dump(4);
	return true;
	
}

bool SceneManager::RegisterSceneFromJson(const std::filesystem::path& filePath)
{
	if (filePath.empty())
	{
		return false;
	}

	const auto sceneName = filePath.stem().string();
	if (FindScenePathByName(sceneName))
	{
		return false;
	}

	
	if (!std::filesystem::exists(filePath))
	{
		return false;
	}

	m_Scenes.emplace(sceneName, filePath);

	return true;
}

