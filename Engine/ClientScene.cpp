#include "pch.h"
#include "ClientScene.h"
#include "GameObject.h"
#include "CameraObject.h"
#include "TransformComponent.h"
#include "DirectionalLightComponent.h"




void ClientScene::Initialize()
{
	auto gamecamera = std::make_shared<CameraObject>(GetEventDispatcher(), 1920.0f, 1080.0f); //

	gamecamera->SetName("Main Camera");
	SetGameCamera(gamecamera); // Main Camera
	AddGameObject(gamecamera);

	auto lightObject = std::make_shared<GameObject>(GetEventDispatcher());
	lightObject->SetName("DirectionalLight");
	if (auto* light = lightObject->AddComponent<DirectionalLightComponent>())
	{
		light->SetColor({ 1.0f, 1.0f, 1.0f });
		light->SetIntensity(1.0f);
		light->SetDirection({ 0.0f, -1.0f, 0.0f });
	}
	AddGameObject(lightObject);
}

void ClientScene::Finalize()
{

}

void ClientScene::Leave()
{
	// Leave시 초기화 필요 -> 다른 씬으로 넘어가도 계속 유지되는 것들이 있음 <- 없애야함
}

void ClientScene::FixedUpdate()
{
	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (gameObject)
		{
			gameObject->FixedUpdate();
		}
	}
	ProcessPendingRemovals();
}

void ClientScene::Update(float dTime)
{
	for (const auto& [name, gameObject] : m_GameObjects)
	{
		if (gameObject)
		{
			gameObject->Update(dTime);
		}
	}
	ProcessPendingRemovals();
}

void ClientScene::StateUpdate(float dTime)
{
	Scene::StateUpdate(dTime);
}

