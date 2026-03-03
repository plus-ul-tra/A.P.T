#include "SceneChangeTestComponent.h"
#include "ReflectionMacro.h"
#include "ServiceRegistry.h"
#include "GameManager.h"
#include "Object.h"
#include "Event.h"
#include "Scene.h"
#include <iostream>


REGISTER_COMPONENT(SceneChangeTestComponent)
REGISTER_PROPERTY(SceneChangeTestComponent, TargetScene)


SceneChangeTestComponent::SceneChangeTestComponent()
{

}

SceneChangeTestComponent::~SceneChangeTestComponent()
{
	GetEventDispatcher().RemoveListener(EventType::KeyDown, this);
	GetEventDispatcher().RemoveListener(EventType::KeyUp, this);
}

void SceneChangeTestComponent::Start()
{
	GetEventDispatcher().AddListener(EventType::KeyDown, this); // 이벤트 추가
	GetEventDispatcher().AddListener(EventType::KeyUp, this);
}

void SceneChangeTestComponent::Update(float deltaTime)
{
	(void)deltaTime;
}

void SceneChangeTestComponent::OnEvent(EventType type, const void* data)
{
	if (type != EventType::KeyUp)
	{
		return;
	}

	auto keyData = static_cast<const Events::KeyEvent*>(data);
	if (!keyData)
	{
		return;
	}

	if (keyData->key == VK_SPACE)
	{
		std::cout << "Space Request : " << std::endl;
		SceneChange();
		
	}
}

void SceneChangeTestComponent::SceneChange()
{
	Scene* scene = GetOwner()->GetScene();
	if (!scene)
		return;

	std::cout << "SceneChange Request(In Component) : " << m_TargetScene << std::endl;


	auto& services = scene->GetServices();
	auto& gameManager = services.Get<GameManager>();
	gameManager.RequestSceneChange(m_TargetScene);
}
