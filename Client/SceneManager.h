// Game / Client 용
#pragma once
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <string>
#include "Scene.h"
#include "EventDispatcher.h"
#include "IEventListener.h"
#include "json.hpp"


class ServiceRegistry;
class GameManager;
class UIManager;
class CameraObject;
class InputManager; 


class SceneManager : public IEventListener
{
	friend class Editor;
public:
	SceneManager(ServiceRegistry& serviceRegistry) : m_Services(serviceRegistry) { }
	~SceneManager() = default;

	void Initialize();
	void Update(float deltaTime);
	void StateUpdate(float deltaTime);
	void Render(RenderData::FrameData& frameData);

	void SetCamera(CameraObject* camera) { m_Camera = camera; }
	CameraObject* GetCamera() { return m_Camera; }

	std::shared_ptr<Scene> AddScene(const std::string& name, std::shared_ptr<Scene> scene);
	void SetCurrentScene(const std::string& name);
	std::shared_ptr<Scene> GetCurrentScene() const;

	void ChangeScene(const std::string& name);

	void ChangeScene();

	void Reset();

	void RequestQuit() { m_ShouldQuit = true; }
	bool ShouldQuit() const { return m_ShouldQuit; }

	void SetChangeScene(const std::string& name);
	void SetEventDispatcher(EventDispatcher* eventDispatcher);
	void OnEvent(EventType type, const void* data) override;

private:
	enum class SceneTransitionPhase
	{
		None,
		FadeOut,
		FadeIn,
	};

	ServiceRegistry& m_Services;
	void LoadGameScenesFromDirectory(const std::filesystem::path& directoryPath, const std::vector<std::string>& sceneNames);
	bool LoadGameSceneFromJson(const std::filesystem::path& filepath);
	void RestoreSceneUI(const std::shared_ptr<Scene>& scene);
	void RecreateSceneFromTemplate(const std::string& name);
	void StartSceneTransition(const std::string& name);
	void UpdateSceneTransition(float deltaTime);
	float GetTransitionOverlayY(float overlayHeight, float viewportHeight) const;
	float GetTransitionOverlayHeight(float viewportWidth, float viewportHeight) const;
	float GetTransitionOverlayRotation() const;
	void ResolveTransitionTextureHandle();

	std::unordered_map<std::string, std::shared_ptr<Scene>> m_Scenes;
	std::unordered_map<std::string, nlohmann::json> m_SceneUIData;
	std::unordered_map<std::string, nlohmann::json> m_SceneTemplateData;
	std::shared_ptr<Scene> m_CurrentScene;
	CameraObject*   m_Camera = nullptr;
	GameManager*	m_GameManager;
	UIManager*		m_UIManager;
	InputManager*	m_InputManager;
	EventDispatcher* m_EventDispatcher = nullptr;
	std::filesystem::path scenesPath = "../Resources/Scenes"; //Resource 파일 경로
	bool m_ShouldQuit = false;

	std::string m_ChangeSceneName = "";
	SceneTransitionPhase m_TransitionPhase = SceneTransitionPhase::None;
	float m_TransitionTimer = 0.0f;
	float m_FadeOutDuration = 0.25f;
	float m_FadeInDuration = 0.75f;
	float m_FadeOutStartOffsetY = 800.0f; // FadeOut 시작 Y 오프셋 (+면 더 아래에서 시작)
	float m_FadeOutEndOffsetY = 1600.0f;   // FadeOut 종료 Y 오프셋 (+면 더 아래에서 종료)
	float m_FadeInStartOffsetY = 0.0f; // FadeIn 시작 Y 오프셋 (+면 더 아래에서 시작)
	float m_FadeInEndOffsetY = -800.0f;   // FadeIn 종료 Y 오프셋 (+면 덜 올라감)
	TextureHandle m_TransitionTextureHandle = TextureHandle::Invalid();
	bool m_TransitionTextureResolved = false;
	bool m_SceneSwapPending = false;
};

