//Client
#include "pch.h"
#include "Engine.h"
#include "GameApplication.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "UIManager.h"
#include "GameManager.h"
#include "Importer.h"
#include "AssetLoader.h"
#include "EventDispatcher.h"
#include "Device.h"
#include "InputManager.h"
#include "Renderer.h"
#include "ServiceRegistry.h"
#include "RandomMachine.h"
#include "DiceSystem.h"
#include "CombatResolver.h"
#include "CombatManager.h"
#include "LogSystem.h"
#include "LootRoller.h"
#include "GameDataRepository.h"
#include "ShopRoller.h"

namespace
{
	GameApplication* g_pMainApp = nullptr;
}

int main()
{
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	//_CrtSetBreakAlloc(597);

	HRESULT hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
	if (FAILED(hr))
		return -1;

    ServiceRegistry services;

	auto& inputManager = services.Register<InputManager>();
	auto& assetLoader = services.Register<AssetLoader>();
	auto& soundManager = services.Register<SoundManager>();
	auto& uiManager = services.Register<UIManager>();
    auto& gameManager = services.Register<GameManager>();
    auto& randomMachine = services.Register<RandomMachine>();
    auto& diceSystem = services.Register<DiceSystem>(randomMachine);
    auto& logSystem = services.Register<LogSystem>();
    auto& combatResolver = services.Register<CombatResolver>();
    services.Register<GameDataRepository>();
    services.Register<LootRoller>();
    services.Register<ShopRoller>();
    services.Register<CombatManager>(combatResolver, diceSystem, &logSystem);

	Renderer renderer(assetLoader);
	Engine engine(services, renderer);
	SceneManager sceneManager(services);
  

    g_pMainApp = new GameApplication(services, engine, renderer, sceneManager, inputManager); //service Rocation 있으니 생성자에 안받아도 되지않나
 
 	if (!g_pMainApp->Initialize())
 	{
 		std::cerr << "Failed to initialize sample code." << std::endl;
 		return -1;
 	}
   
    // 2) 메쉬/머티리얼 핸들 꺼내기
    //if (!result.meshes.empty())
    //{
    //    MeshHandle meshHandle = result.meshes[0];
    //    RenderData::MeshData* mesh = loader.GetMeshes().Get(meshHandle);
    //    if (mesh)
    //    {
    //        // vertices, indices 갯수 확인
    //        std::cout << "Vertices: " << mesh->vertices.size() << "\n";
    //        std::cout << "Indices: " << mesh->indices.size() << "\n";

    //        // SubMesh가 있는지, index 범위 검사 등
    //        for (auto& sub : mesh->subMeshes)
    //        {
    //            if (sub.indexStart + sub.indexCount > mesh->indices.size())
    //                std::cout << "Invalid submesh index range\n";
    //        }
    //    }
    //}

    //if (!result.materials.empty())
    //{
    //    auto matHandle = result.materials[0];
    //    RenderData::MaterialData* mat = loader.GetMaterials().Get(matHandle);
    //    if (mat)
    //    {
    //        std::cout << "baseColor: " << mat->baseColor.x << ", " << mat->baseColor.y << "\n";
    //        // 텍스 핸들이 유효한지 확인
    //    }
    //}

    //if (result.skeleton.IsValid())
    //{
    //    auto skel = loader.GetSkeletons().Get(result.skeleton);
    //    if (skel)
    //        std::cout << "Bones: " << skel->bones.size() << "\n";
    //}

    //if (!result.animations.empty())
    //{
    //    auto anim = loader.GetAnimations().Get(result.animations[0]);
    //    if (anim)
    //        std::cout << "Anim name: " << anim->name << ", tracks: " << anim->tracks.size() << "\n";
    //}
    //// 3) 실제 RenderData 접근 예시
    //// (필요 시 내부 데이터를 직접 꺼내 사용 가능)
    //if (!result.meshes.empty())
    //{
    //    RenderData::MeshData* meshData = loader.GetMeshes().Get(result.meshes[0]);
    //    // meshData->vertices / indices 접근 가능
    //}

    //if (result.skeleton.IsValid())
    //{
    //    RenderData::Skeleton* skeleton = loader.GetSkeletons().Get(result.skeleton);
    //    // skeleton->bones 사용 가능

    //}

//#if defined(_DEBUG)
//    // Store별로 내부 상태 출력
//    loader.GetMeshes().DebugDump(std::cout);
//    loader.GetMaterials().DebugDump(std::cout);
//    loader.GetTextures().DebugDump(std::cout);
//    loader.GetSkeletons().DebugDump(std::cout);
//    loader.GetAnimations().DebugDump(std::cout);
//#endif

 	g_pMainApp->Run();
 
 	g_pMainApp->Finalize();
 
 	delete g_pMainApp;
    
    sceneManager.Reset();
    engine.Reset();

	CoUninitialize();

	return 0;
}