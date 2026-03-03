//editor
#include "pch.h"
#include "EditorApplication.h"
#include "SceneManager.h"
#include "Renderer.h"
#include "Engine.h"
#include "EventDispatcher.h"
#include "InputManager.h"
#include "AssetLoader.h"
#include "SoundManager.h"
#include "GameManager.h"
#include "UIManager.h"
#include "ServiceRegistry.h"
#include "RandomMachine.h"
#include "DiceSystem.h"
#include "LogSystem.h"
#include "LootRoller.h"
#include "GameDataRepository.h"
#include "ShopRoller.h"
#include "CombatResolver.h"
#include "CombatManager.h"

namespace
{
	void WriteManualTestJson(const std::string& jsonPayload)
	{
		std::ofstream outFile("manual_test_results.json");
		if (outFile)
		{
			outFile << jsonPayload;
		}
	}
}
//namespace
//{
//	EditorApplication* g_pMainApp = nullptr;
//}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr)) {
		return -1;
	}

	ServiceRegistry services;

	auto& inputManager = services.Register<InputManager>();
	auto& assetLoader = services.Register<AssetLoader>();
	auto& soundManager = services.Register<SoundManager>();
	auto& uiManager = services.Register<UIManager>();
	auto& gameManager = services.Register<GameManager>();
	auto& randomMachine = services.Register<RandomMachine>();
	auto& diceSystem = services.Register<DiceSystem>(randomMachine);
	auto& logSystem = services.Register<LogSystem>();
	auto& lootRoller = services.Register<LootRoller>();
	services.Register<GameDataRepository>();
	services.Register<ShopRoller>();
	auto& combatResolver = services.Register<CombatResolver>();
	services.Register<CombatManager>(combatResolver, diceSystem, &logSystem);

	Renderer renderer(assetLoader);
	Engine engine(services, renderer);
	SceneManager sceneManager(services);

	 //<<-- FrameData 강제 필요 but imgui 는 필요 없음
	//Editor는 시작시 사용하는 모든 fbx load

	EditorApplication app(services, engine, renderer, sceneManager); //service Rocation 있으니 생성자에 안받아도 되지않나
	if (!app.Initialize())
	{
		CoUninitialize();
		return -1;
	}


	// 구동
	app.Run();
	app.Finalize();
	//

	sceneManager.Reset();
	engine.Reset();

	CoUninitialize();

	return 0;
}