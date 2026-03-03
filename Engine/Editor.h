#pragma once
#include "SceneManager.h"

class Editor
{
public:
	Editor(SceneManager& sceneManager) : m_SceneManager(sceneManager) {}
	void Update();
private:
	void DrawSceneList          ();
	void DrawGameObjectHierarchy(std::shared_ptr<Scene> currentScene);
	void DrawGameObjectInspector(std::shared_ptr<Scene> currentScene);
	void SaveAndLoadFile        (std::shared_ptr<Scene> currentScene);
	void CameraControl          (std::shared_ptr<Scene> currentScene);

private:
	SceneManager& m_SceneManager; 
	std::string   m_SelectedKey;
	int           m_SelectedIndex = -1;
};

