	#pragma once

#include "NzWndBase.h"
#include <memory>
#include <string>
#include "EditorViewport.h"
#include "Engine.h"
#include "RenderTargetContext.h" 
#include "RenderData.h"
#include "json.hpp"
#include <filesystem>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include "UndoManager.h"
#include "Snapshot.h"

class ServiceRegistry;
class Renderer;
class SceneManager;
class Scene;
class GameObject;
class AssetLoader;
class SoundManager;
class GameManager;

enum class EditorPlayState
{
	Stop,
	Play,
	Pause
};


class EditorApplication : public NzWndBase
{

public:
	EditorApplication(ServiceRegistry& serviceRegistry, Engine& engine,Renderer& renderer, SceneManager& sceneManager) 
		: NzWndBase(), m_Services(serviceRegistry), m_Engine(engine),m_Renderer(renderer), m_SceneManager(sceneManager)
		, m_EditorViewport("Editor")
		, m_GameViewport("Game"){
	}

	virtual ~EditorApplication() = default;

	bool Initialize();
	void Run();
	void Finalize();

	bool OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) override;

private:
	void UpdateInput();
	void UpdateLogic();
	void Update();
	void UpdateSceneViewport();
	void UpdateEditorCamera();
	void HandleEditorViewportSelection();

	//Render 관련
	void Render();
	void RenderImGUI();
	void RenderSceneView();

	// 필요한 창 생성
	void DrawMainMenuBar();
	void DrawHierarchy();
	void DrawInspector();
	void DrawFolderView();
	void DrawResourceBrowser();
	void DrawGizmo();
	void DrawUIEditorPreview();

	void FocusEditorCameraOnObject(const std::shared_ptr<GameObject>& object);

	//Gui 관련
	void CreateDockSpace();
	void SetupEditorDockLayout();

	SceneStateSnapshot CaptureSceneState(const std::shared_ptr<Scene>& scene) const;
	void RestoreSceneState(const SceneStateSnapshot& snapshot);
	void ClearPendingPropertySnapshots();

	void OnResize(int width, int height) override;
	void OnClose() override;

	float m_fFrameCount = 0.0f;
	UINT64 m_FrameIndex = 0;

	ServiceRegistry&      m_Services;
	Engine&			      m_Engine;
	SceneManager&         m_SceneManager;
	Renderer&             m_Renderer;
	AssetLoader*		  m_AssetLoader;
	SoundManager*		  m_SoundManager;
	InputManager*	      m_InputManager;
	GameManager*		  m_GameManager;
	RenderData::FrameData m_FrameData;
	RenderTargetContext   m_SceneRenderTarget;
	RenderTargetContext   m_SceneRenderTarget_edit;

	EditorViewport        m_EditorViewport;
	EditorViewport        m_GameViewport;
	EditorPlayState       m_EditorState = EditorPlayState::Stop;
	// Hier
	std::string m_SelectedObjectName;
	std::unordered_set<std::string> m_SelectedObjectNames;

	//이름 변경 관련
	std::string m_LastSelectedObjectName;
	std::string m_LastSceneName;
	std::array<char, 256> m_ObjectNameBuffer;
	std::array<char, 256> m_SceneNameBuffer;

	struct PendingPropertySnapshot
	{
		ObjectSnapshot beforeSnapshot;
		bool updated = false;
	};
	std::unordered_map<size_t, PendingPropertySnapshot> m_PendingPropertySnapshots;
	std::filesystem::path m_LastPendingSnapshotScenePath;

	struct PendingUIPropertySnapshot
	{
		nlohmann::json beforeSnapshot;
		bool updated = false;
	};
	std::unordered_map<size_t, PendingUIPropertySnapshot> m_PendingUIPropertySnapshots;


	nlohmann::json m_ObjectClipboard;
	bool m_ObjectClipboardHasData = false;
	bool m_ObjectClipboardIsOpaque = true;
	nlohmann::json m_UIObjectClipboard;
	bool m_UIObjectClipboardHasData = false;

	// Floder View 변수
	// resource root 지정 // 추후 수정 필요 //작업 환경마다 다를 수 있음
	std::filesystem::path m_ResourceRoot = "../Resources/Scenes"; 
	std::filesystem::path m_SelectedResourcePath;

	std::filesystem::path m_CurrentScenePath;

	std::filesystem::path m_PendingDeletePath;
	std::filesystem::path m_PendingSavePath;

	bool m_OpenSaveConfirm = false;
	bool m_OpenDeleteConfirm = false;

	UndoManager m_UndoManager;

	std::string m_SelectedUIObjectName;
	std::unordered_set<std::string> m_SelectedUIObjectNames;
	std::string m_LastSelectedUIObjectName;
	std::array<char, 256> m_UIObjectNameBuffer{};
	std::string m_HorizontalSlotCandidate;
	std::string m_CanvasSlotCandidate;

	std::unordered_map<std::string, std::string> m_UIButtonBindingTargets;
	std::unordered_map<std::string, std::string> m_UISliderBindingTargets;
};