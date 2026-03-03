#pragma once
#include <unordered_map>
#include <memory>
#include <vector>
#include "RenderData.h"
#include "EventDispatcher.h"
#include "json.hpp"

class NzWndBase;
class GameObject;
class UIObject;
class GameManager;
class SceneManager;
class CameraObject;
class ServiceRegistry;
class AssetLoader;

class Scene
{
public:
	friend class Editor;

	Scene(ServiceRegistry& serviceRegistry);

	virtual ~Scene();
	virtual void Initialize () = 0;
	virtual void Finalize   () = 0;

	virtual void Enter      ();
	virtual void Leave      () = 0;

	virtual void FixedUpdate() = 0;
	virtual void Update     (float deltaTime) = 0;
	virtual void StateUpdate(float deltaTime);	// Light, Camera, Fog Update 
	virtual void Render(RenderData::FrameData& data) const;

	void AddGameObject      (std::shared_ptr<GameObject> gameObject);
	void RemoveGameObject   (std::shared_ptr<GameObject> gameObject);
	std::shared_ptr<GameObject> CreateGameObject(const std::string& name);
	void QueueGameObjectRemoval(const std::string& name);
	void ProcessPendingRemovals();


	// For Editor map 자체 Getter( 수정 불가능 상태 )
	// 수정 해야하는 경우 양 const 제거 ( Add GameObject 는 editor에서 call 하면, Scene의 add object 작동 editor map 직접 수정 X)
	const std::unordered_map<std::string, std::shared_ptr<GameObject>>& GetGameObjects     () const { return m_GameObjects;      }

	void SetGameCamera(std::shared_ptr<CameraObject> cameraObject);
	std::shared_ptr<CameraObject> GetGameCamera() { return m_GameCamera; }

	void SetEditorCamera(std::shared_ptr<CameraObject> cameraObject);
	std::shared_ptr<CameraObject> GetEditorCamera() { return m_EditorCamera; }

	bool RemoveGameObjectByName(const std::string& name);
	bool RenameGameObject(const std::string& currentName, const std::string& newName);
	bool HasGameObjectName(const std::string& name) const;

	void Serialize          (nlohmann::json& j) const;
	void Deserialize        (const nlohmann::json& j);
	void BuildFrameData(RenderData::FrameData& frameData) const;

	void EnsureAutoComponentsForSave();

	EventDispatcher& GetEventDispatcher() { return m_EventDispatcher; }

	void SetName            (std::string name) { m_Name = name; }
	std::string GetName     () const     { return m_Name;   }

	void SetGameManager     (GameManager* gameManager);
	GameManager* GetGameManager() const { return m_GameManager; }
	void SetSceneManager    (SceneManager* sceneManager);

	ServiceRegistry& GetServices() const { return m_Services; }

	void SetIsPause         (bool value) { m_Pause = value; }
	bool GetIsPause         ()           { return m_Pause;  }

protected:
	std::unordered_map<std::string, std::shared_ptr<GameObject>> m_GameObjects;

	ServiceRegistry& m_Services;
	SceneManager*    m_SceneManager = nullptr;
	GameManager*     m_GameManager = nullptr;
	std::shared_ptr<CameraObject>   m_GameCamera;
	std::shared_ptr<CameraObject>   m_EditorCamera;
	std::string      m_Name;

	bool		     m_Pause = false;
	std::vector<std::string> m_PendingRemovalNames;
	
private:
	AssetLoader*    m_AssetLoader;
	EventDispatcher m_EventDispatcher;
private:
	Scene(const Scene&)            = delete;
	Scene& operator=(const Scene&) = delete;
	Scene(Scene&&)                 = delete;
	Scene& operator=(Scene&&)      = delete;
};
