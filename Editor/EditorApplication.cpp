#include "pch.h"
#include "EditorApplication.h"
#include "CameraObject.h"
#include "TransformComponent.h"
#include "CameraComponent.h"
#include "MeshComponent.h"
#include "MeshRenderer.h"
#include "MaterialComponent.h"
#include "SkeletalMeshComponent.h"
#include "SkeletalMeshRenderer.h"
#include "AnimationComponent.h"
#include "InputManager.h"
#include "GameObject.h"
#include "Reflection.h"
#include "ServiceRegistry.h"
#include "AssetLoader.h"
#include "Renderer.h"
#include "SceneManager.h"
#include "SoundManager.h"
#include "Scene.h"
#include "DX11.h"
#include "Importer.h"
#include "Util.h"
#include "BoxColliderComponent.h"
#include "RayHelper.h"
#include "json.hpp"
#include "ImGuizmo.h"
#include "MathHelper.h"
#include "Snapshot.h"
#include "UIManager.h"
#include "UIComponent.h"
#include "UIButtonComponent.h"
#include "UIProgressBarComponent.h"
#include "UIPrimitives.h"
#include "HorizontalBox.h"
#include "Canvas.h"
#include "UISliderComponent.h"
#include "UITextComponent.h"
#include <cmath>
#include <functional>
#include <algorithm>
#include <limits>
#include "GameManager.h"
#include <unordered_set>
#include "UIManager.h"

namespace
{
	bool IsUIComponentType(const std::string& typeName)
	{
		if (typeName == UIFSMComponent::StaticTypeName)
		{
			return true;
		}
		return ComponentRegistry::Instance().IsUIType(typeName);
	}
}

#define DRAG_SPEED 0.01f
namespace fs = std::filesystem;
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ImGUI 창그리기
bool EditorApplication::Initialize()
{
	const wchar_t* className = L"MIEditor";
	const wchar_t* windowName = L"MIEditor";

	if (false == Create(className, windowName, 2560, 1600))
	{
		return false;
	}
	///m_hwnd
	//m_Engine.GetAssetManager().Init(L"../Resource");
	//m_Engine.GetSoundAssetManager().Init(L"../Sound");
	m_Engine.CreateDevice(m_hwnd);							//엔진 Device, DXDC생성

	ImportAll();
	m_AssetLoader = &m_Services.Get<AssetLoader>();
	m_AssetLoader->LoadAll();
	m_SoundManager = &m_Services.Get<SoundManager>();
	m_SoundManager->Init();
	m_SoundManager->CreateBGMSource(m_AssetLoader->GetBGMPaths());
	m_SoundManager->CreateSFXSource(m_AssetLoader->GetSFXPaths());
	m_InputManager = &m_Services.Get<InputManager>();
	m_GameManager = &m_Services.Get<GameManager>();
	m_Renderer.InitializeTest(m_hwnd, m_width, m_height, m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());  // Device 생성
	m_SceneManager.Initialize();

	if (m_InputManager)
	{
		m_InputManager->SetUIReferenceSize(DirectX::XMFLOAT2{ 2560.0f, 1600.0f });
	}

	m_SceneRenderTarget.SetDevice(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());
	m_SceneRenderTarget_edit.SetDevice(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC());

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;				// ini 사용 안함
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
	ImGui::StyleColorsDark();
	ImGui_ImplWin32_Init(m_hwnd);
	ImGui_ImplDX11_Init(m_Engine.Get3DDevice(), m_Engine.GetD3DDXDC()); //★ 일단 임시 Renderer의 Device사용, 엔진에서 받는 걸로 수정해야됨
	//ImGui_ImplDX11_Init(m_Engine.Get3DDevice(),m_Engine.GetD3DDXDC());
	//RT 받기
	//초기 세팅 값으로 창 배치

	return true;
}


void EditorApplication::Run() {
	//실행 루프
	MSG msg = { 0 };
	while (WM_QUIT != msg.message /*&& !m_SceneManager.ShouldQuit()*/) {
		// Window Message 해석
		if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
		{
			m_InputManager->OnHandleMessage(msg);
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		else {
			m_Engine.UpdateTime();
			Update();
			m_Engine.UpdateInput();
			UpdateLogic();  //★
			Render();

		}
	}
}



void EditorApplication::Finalize() {
	__super::Destroy();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	//그외 메모리 해제
}

bool EditorApplication::OnWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wparam, lparam))
	{
		return true; // ImGui가 메시지를 처리했으면 true 반환
	}

	return false;
}



void EditorApplication::UpdateInput()
{
	ImGuiIO& io = ImGui::GetIO();

	if (m_EditorState == EditorPlayState::Play) {
		return;
	}

	if (!io.WantTextInput)
	{
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z))
		{
			ClearPendingPropertySnapshots();
			m_UndoManager.Undo();
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y))
		{
			ClearPendingPropertySnapshots();
			m_UndoManager.Redo();
		}
	}
}

void EditorApplication::UpdateLogic()
{
	if (m_EditorState == EditorPlayState::Play)
	{
		m_SceneManager.ChangeScene();
	}
}

void EditorApplication::Update()
{
	float dTime = m_Engine.GetTime(); 
	if (m_InputManager)
	{
		m_InputManager->SetEnabled(m_EditorState == EditorPlayState::Play);
	}
	UpdateInput();
	m_SceneManager.StateUpdate(dTime);
	m_SceneManager.Update(dTime);

	m_SoundManager->Update();
}

void EditorApplication::UpdateSceneViewport()
{
	// m_Veiwport = editor Viewport
	const ImVec2 editorSize = m_EditorViewport.GetViewportSize();
	const ImVec2 gameSize = m_GameViewport.GetViewportSize();
	const UINT editorWidth = static_cast<UINT>(editorSize.x);
	const UINT editorHeight = static_cast<UINT>(editorSize.y);
	const UINT gameWidth = static_cast<UINT>(gameSize.x);
	const UINT gameHeight = static_cast<UINT>(gameSize.y);

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}

	if (editorWidth != 0 && editorHeight != 0)
	{
		//return;
		if (auto editorCamera = scene->GetEditorCamera())
		{
			if (auto* cameraComponent = editorCamera->GetComponent<CameraComponent>())
			{
				cameraComponent->SetViewport({ static_cast<float>(editorWidth), static_cast<float>(editorHeight) });
			}
		}
	}

	if (gameWidth != 0 && gameHeight != 0)
	{
		if (auto gameCamera = scene->GetGameCamera())
		{
			if (auto* cameraComponent = gameCamera->GetComponent<CameraComponent>())
			{
				cameraComponent->SetViewport({ static_cast<float>(gameWidth), static_cast<float>(gameHeight) });
			}
		}
	}

}

void EditorApplication::UpdateEditorCamera()
{
	if (!m_EditorViewport.IsHovered())
	{
		return;
	}

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}

	auto camera = scene->GetEditorCamera();
	if (!camera)
	{
		return;
	}

	auto* cameraComponent = camera->GetComponent<CameraComponent>();
	if (!cameraComponent)
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	const float deltaTime = (io.DeltaTime > 0.0f) ? io.DeltaTime : m_Engine.GetTimer().DeltaTime();

	XMFLOAT3 eye = cameraComponent->GetEye();
	XMFLOAT3 look = cameraComponent->GetLook();
	XMFLOAT3 up = cameraComponent->GetUp();

	XMVECTOR eyeVec = XMLoadFloat3(&eye);
	XMVECTOR lookVec = XMLoadFloat3(&look);
	XMVECTOR upVec = XMVector3Normalize(XMLoadFloat3(&up));
	XMVECTOR forwardVec = XMVector3Normalize(XMVectorSubtract(lookVec, eyeVec));
	XMVECTOR rightVec = XMVector3Normalize(XMVector3Cross(upVec, forwardVec));

	bool updated = false;

	if (io.MouseDown[1])
	{
		const float rotationSpeed = 0.003f;
		const float yaw = io.MouseDelta.x * rotationSpeed;
		const float pitch = io.MouseDelta.y * rotationSpeed;

		if (yaw != 0.0f || pitch != 0.0f)
		{
			XMVECTOR worldUp = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
			const XMMATRIX yawRotation = XMMatrixRotationAxis(upVec, yaw);
			const XMMATRIX pitchRotation = XMMatrixRotationAxis(rightVec, pitch);
			XMMATRIX transform = pitchRotation * yawRotation;
			forwardVec = XMVector3Normalize(XMVector3TransformNormal(forwardVec, transform));
			rightVec = XMVector3Normalize(XMVector3Cross(worldUp, forwardVec));
			upVec = XMVector3Normalize(XMVector3Cross(forwardVec, rightVec));

			lookVec = XMVectorAdd(eyeVec, forwardVec);
			updated = true;

		}

		const float baseSpeed = 6.0f;
		const float speedMultiplier = io.KeyShift ? 3.0f : 1.0f;
		const float moveSpeed = baseSpeed * speedMultiplier;

		XMVECTOR moveVec = XMVectorZero();
		if (ImGui::IsKeyDown(ImGuiKey_W))
		{
			moveVec = XMVectorAdd(moveVec, forwardVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_S))
		{
			moveVec = XMVectorSubtract(moveVec, forwardVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_D))
		{
			moveVec = XMVectorAdd(moveVec, rightVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_A))
		{
			moveVec = XMVectorSubtract(moveVec, rightVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_E))
		{
			moveVec = XMVectorAdd(moveVec, upVec);
		}
		if (ImGui::IsKeyDown(ImGuiKey_Q))
		{
			moveVec = XMVectorSubtract(moveVec, upVec);
		}
		if (XMVectorGetX(XMVector3LengthSq(moveVec)) > 0.0f)
		{
			moveVec = XMVector3Normalize(moveVec);
			const XMVECTOR scaledMove = XMVectorScale(moveVec, moveSpeed * deltaTime);
			eyeVec = XMVectorAdd(eyeVec, scaledMove);
			lookVec = XMVectorAdd(lookVec, scaledMove);
			updated = true;
		}
	}

	if (io.MouseDown[2])
	{
		const float panSpeed = 0.01f;
		const XMVECTOR panRight = XMVectorScale(rightVec, -io.MouseDelta.x * panSpeed);
		const XMVECTOR panUp = XMVectorScale(upVec, io.MouseDelta.y * panSpeed);
		const XMVECTOR pan = XMVectorAdd(panRight, panUp);
		eyeVec = XMVectorAdd(eyeVec, pan);
		lookVec = XMVectorAdd(lookVec, pan);
		updated = true;
	}

	if (io.MouseWheel != 0.0f)
	{
		const float zoomSpeed = 4.0f;
		const XMVECTOR dolly = XMVectorScale(forwardVec, io.MouseWheel * zoomSpeed);
		eyeVec = XMVectorAdd(eyeVec, dolly);
		lookVec = XMVectorAdd(lookVec, dolly);
		updated = true;
	}

	if (updated)
	{
		upVec = XMVector3Normalize(XMVector3Cross(forwardVec, rightVec));
		XMStoreFloat3(&eye, eyeVec);
		XMStoreFloat3(&look, lookVec);
		XMStoreFloat3(&up, upVec);
		cameraComponent->SetEyeLookUp(eye, look, up);
	}
}

namespace
{
	bool BuildMeshWorldBounds(const GameObject& object, AssetLoader* assetLoader, XMFLOAT3& outMin, XMFLOAT3& outMax)
	{
		if (!assetLoader)
		{
			return false;
		}

		auto* transform = object.GetComponent<TransformComponent>();
		if (!transform)
		{
			return false;
		}

		const auto meshComponents = object.GetComponentsDerived<MeshComponent>();
		if (meshComponents.empty())
		{
			return false;
		}

		const auto world = DirectX::XMLoadFloat4x4(&transform->GetWorldMatrix());
		const MeshComponent* meshComponent = nullptr;

		for (const auto* component : meshComponents)
		{
			if (!component)
			{
				continue;
			}

			if (!component->GetMeshHandle().IsValid())
			{
				continue;
			}

			meshComponent = component;
			break;
		}

		if (!meshComponent)
		{
			return false;
		}

		const auto* meshData = assetLoader->GetMeshes().Get(meshComponent->GetMeshHandle());
		if (!meshData)
		{
			return false;
		}

		const XMFLOAT3 localMin = meshData->boundsMin;
		const XMFLOAT3 localMax = meshData->boundsMax;
		const XMFLOAT3 corners[8] = {
			{ localMin.x, localMin.y, localMin.z },
			{ localMax.x, localMin.y, localMin.z },
			{ localMax.x, localMax.y, localMin.z },
			{ localMin.x, localMax.y, localMin.z },
			{ localMin.x, localMin.y, localMax.z },
			{ localMax.x, localMin.y, localMax.z },
			{ localMax.x, localMax.y, localMax.z },
			{ localMin.x, localMax.y, localMax.z }
		};

		XMFLOAT3 minOut{ FLT_MAX, FLT_MAX, FLT_MAX };
		XMFLOAT3 maxOut{ -FLT_MAX, -FLT_MAX, -FLT_MAX };

		for (const auto& corner : corners)
		{
			const auto v = DirectX::XMLoadFloat3(&corner);
			const auto transformed = DirectX::XMVector3TransformCoord(v, world);
			XMFLOAT3 worldCorner{};
			DirectX::XMStoreFloat3(&worldCorner, transformed);

			minOut.x = std::min(minOut.x, worldCorner.x);
			minOut.y = std::min(minOut.y, worldCorner.y);
			minOut.z = std::min(minOut.z, worldCorner.z);

			maxOut.x = (std::max)(maxOut.x, worldCorner.x);
			maxOut.y = (std::max)(maxOut.y, worldCorner.y);
			maxOut.z = (std::max)(maxOut.z, worldCorner.z);
		}

		outMin = minOut;
		outMax = maxOut;
		return true;
	}

	bool IntersectsRayBounds(const XMFLOAT3& rayOrigin, const XMFLOAT3& rayDir, const XMFLOAT3& boundsMin, const XMFLOAT3& boundsMax, float& outT)
	{
		float tMin = 0.0f;
		float tMax = FLT_MAX;

		const float origin[3] = { rayOrigin.x, rayOrigin.y, rayOrigin.z };
		const float dir[3] = { rayDir.x, rayDir.y, rayDir.z };
		const float minB[3] = { boundsMin.x, boundsMin.y, boundsMin.z };
		const float maxB[3] = { boundsMax.x, boundsMax.y, boundsMax.z };

		for (int axis = 0; axis < 3; ++axis)
		{
			if (std::abs(dir[axis]) < 1e-6f)
			{
				if (origin[axis] < minB[axis] || origin[axis] > maxB[axis])
				{
					return false;
				}
				continue;
			}

			const float invD = 1.0f / dir[axis];
			float t0 = (minB[axis] - origin[axis]) * invD;
			float t1 = (maxB[axis] - origin[axis]) * invD;
			if (t0 > t1)
			{
				std::swap(t0, t1);
			}

			tMin = (std::max)(tMin, t0);
			tMax = (std::min)(tMax, t1);
			if (tMax < tMin)
			{
				return false;
			}
		}

		outT = tMin;
		return true;
	}
}

void EditorApplication::HandleEditorViewportSelection()
{
	/*if (m_EditorState != EditorPlayState::Stop)
	{
		return;
	}*/

	if (!m_EditorViewport.HasViewportRect() || !m_EditorViewport.IsHovered())
	{
		return;
	}

	ImGuiIO& io = ImGui::GetIO();
	if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		return;
	}

	if (ImGuizmo::IsOver() || ImGuizmo::IsUsing())
	{
		return;
	}

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}

	auto editorCamera = scene->GetEditorCamera().get();
	if (!editorCamera)
	{
		return;
	}

	const ImVec2 rectMin = m_EditorViewport.GetViewportRectMin();
	const ImVec2 rectMax = m_EditorViewport.GetViewportRectMax();
	const float width = rectMax.x - rectMin.x;
	const float height = rectMax.y - rectMin.y;
	if (width <= 0.0f || height <= 0.0f)
	{
		return;
	}

	const XMFLOAT4X4 viewMatrix = editorCamera->GetViewMatrix();
	const XMFLOAT4X4 projMatrix = editorCamera->GetProjMatrix();
	const auto viewMat = DirectX::XMLoadFloat4x4(&viewMatrix);
	const auto projMat = DirectX::XMLoadFloat4x4(&projMatrix);
	const Ray pickRay = MakePickRayLH(io.MousePos.x, io.MousePos.y, rectMin.x, rectMin.y, width, height, viewMat, projMat);

	const auto& gameObjects = scene->GetGameObjects();
	float closestT = FLT_MAX;
	GameObject* closestObject = nullptr;

	for (const auto& [name, object] : gameObjects)
	{
		if (!object)
		{
			continue;
		}

		XMFLOAT3 boundsMin{};
		XMFLOAT3 boundsMax{};
		bool hasBounds = false;

		if (auto* collider = object->GetComponent<BoxColliderComponent>())
		{
			if (collider->HasBounds())
			{
				hasBounds = collider->BuildWorldBounds(boundsMin, boundsMax);
			}
		}

		if (!hasBounds)
		{
			hasBounds = BuildMeshWorldBounds(*object, m_AssetLoader, boundsMin, boundsMax);
		}

		if (!hasBounds)
		{
			continue;
		}

		float hitT = 0.0f;
		if (!IntersectsRayBounds(pickRay.m_Pos, pickRay.m_Dir, boundsMin, boundsMax, hitT))
		{
			continue;
		}

		if (hitT >= 0.0f && hitT < closestT)
		{
			closestT = hitT;
			closestObject = object.get();
		}
	}

	if (!closestObject)
	{
		return;
	}

	const std::string& targetName = closestObject->GetName();
	if (targetName.empty())
	{
		return;
	}

	if (io.KeyCtrl)
	{
		if (m_SelectedObjectNames.erase(targetName) == 0)
		{
			m_SelectedObjectNames.insert(targetName);
			m_SelectedObjectName = targetName;
		}
		else if (m_SelectedObjectName == targetName)
		{
			if (!m_SelectedObjectNames.empty())
			{
				m_SelectedObjectName = *m_SelectedObjectNames.begin();
			}
			else
			{
				m_SelectedObjectName.clear();
			}
		}
		return;
	}

	m_SelectedObjectName = targetName;
	m_SelectedObjectNames.clear();
	m_SelectedObjectNames.insert(targetName);
}

void EditorApplication::Render() {
	if (!m_Engine.GetD3DDXDC()) return; //★


	ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
	m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
	SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());

	ClearBackBuffer(COLOR(0.12f, 0.12f, 0.12f, 1.0f), m_Engine.GetD3DDXDC(), *rtvs);

	//m_SceneManager.Render(); // Scene 전체 그리고

	RenderImGUI();

	Flip(m_Renderer.GetSwapChain().Get()); //★
}

void EditorApplication::RenderImGUI() {
	//★★
	//텍스트 그리기전 리소스 해제
	ID3D11ShaderResourceView* nullSRV[128] = {};
	m_Engine.GetD3DDXDC()->PSSetShaderResources(0, 128, nullSRV);

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	CreateDockSpace();
	DrawMainMenuBar();
	DrawHierarchy();

	DrawInspector();
	RenderSceneView();
	DrawFolderView();
	DrawResourceBrowser();
	DrawUIEditorPreview();


	//Scene그리기

   //흐름만 참조. 추후 우리 형태에 맞게 개발 필요
	const bool gameViewportChanged = m_GameViewport.Draw(m_SceneRenderTarget);
	const bool editorViewportChanged = m_EditorViewport.Draw(m_SceneRenderTarget_edit);

	if (editorViewportChanged || gameViewportChanged)
	{
		UpdateSceneViewport();
	}

	if (m_InputManager && m_GameViewport.HasViewportRect())
	{
		const ImVec2 rectMin = m_GameViewport.GetViewportRectMin();
		const ImVec2 rectMax = m_GameViewport.GetViewportRectMax();
		POINT clientMin{ static_cast<LONG>(rectMin.x), static_cast<LONG>(rectMin.y) };
		POINT clientMax{ static_cast<LONG>(rectMax.x), static_cast<LONG>(rectMax.y) };

		ScreenToClient(m_hwnd, &clientMin);
		ScreenToClient(m_hwnd, &clientMax);

		m_InputManager->SetViewportRect({ clientMin.x, clientMin.y, clientMax.x, clientMax.y });
	}

	HandleEditorViewportSelection();
	UpdateEditorCamera();
	DrawGizmo();

	// DockBuilder
	static bool dockBuilt = true;
	if (dockBuilt)
	{
		//std::cout << "Layout Init" << std::endl;
		SetupEditorDockLayout();
		dockBuilt = false;
	}

	ImGui::Render();  // Gui들그리기

	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	ImGuiIO& io = ImGui::GetIO();

	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();

		// main back buffer 상태 복구 // 함수로 묶기
		ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
		m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
		//m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
		SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());
	}

}

// 게임화면 
void EditorApplication::RenderSceneView() {

	//if (!m_SceneRenderTarget.IsValid())
	//{
	//	return;
	//}

	//auto scene = m_SceneManager.GetCurrentScene();
	//if (!scene)
	//{
	//	return;
	//}

	//scene->BuildFrameData(m_FrameData);
	m_FrameData.context.frameIndex = static_cast<UINT32>(m_FrameIndex++);
	m_FrameData.context.deltaTime = m_Engine.GetTimer().DeltaTime();

	m_SceneRenderTarget.Bind();
	m_SceneRenderTarget.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));

	m_SceneRenderTarget_edit.Bind();
	m_SceneRenderTarget_edit.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));


	auto scene = m_SceneManager.GetCurrentScene();
	if (scene)
	{
		scene->Render(m_FrameData);
		if (auto* uiManager = m_SceneManager.GetUIManager())
		{
			uiManager->BuildUIFrameData(m_FrameData);
		}
	}

	//m_Renderer.RenderFrame(m_FrameData, m_SceneRenderTarget, m_SceneRenderTarget_edit);



	if (!scene)
	{
		return;
	}

	auto editorCamera = scene->GetEditorCamera().get();
	if (!editorCamera)
	{
		return;
	}

	if (auto* cameraComponent = editorCamera->GetComponent<CameraComponent>())
	{
		const ImVec2 editorSize = m_EditorViewport.GetViewportSize();
		if (editorSize.x > 0.0f && editorSize.y > 0.0f)
		{
			cameraComponent->SetViewport({ editorSize.x, editorSize.y });
		}
	}
	//scene->Render(m_FrameData);

	m_SceneRenderTarget.Bind();
	m_SceneRenderTarget.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));

	m_SceneRenderTarget_edit.Bind();
	m_SceneRenderTarget_edit.Clear(COLOR(0.1f, 0.1f, 0.1f, 1.0f));

	m_Renderer.RenderFrame(m_FrameData, m_SceneRenderTarget, m_SceneRenderTarget_edit);
	scene->Render(m_FrameData);
	// 복구
	ID3D11RenderTargetView* rtvs[] = { m_Renderer.GetRTView().Get() };
	m_Engine.GetD3DDXDC()->OMSetRenderTargets(1, rtvs, nullptr);
	SetViewPort(m_width, m_height, m_Engine.GetD3DDXDC());
}


void EditorApplication::DrawMainMenuBar()
{
	if (ImGui::BeginMainMenuBar())
	{

		// ---- 중앙 정렬 계산 ----
		float buttonWidth = 60.0f;
		float spacing = ImGui::GetStyle().ItemSpacing.x;
		float totalButtonWidth = buttonWidth * 2 + spacing;

		float windowWidth = ImGui::GetWindowWidth();
		float centerX = (windowWidth - totalButtonWidth) * 0.5f;

		ImGui::SetCursorPosX(centerX);
		//Play = True일 때 비활성

		bool disablePlay = (m_EditorState == EditorPlayState::Play);
		bool disablePause = (m_EditorState != EditorPlayState::Play);
		bool disableStop = (m_EditorState == EditorPlayState::Stop);

		// Pause -> Play 는 저장되지 않도록
		ImGui::BeginDisabled(disablePlay);
		if (ImGui::Button("Play", ImVec2(buttonWidth, 0)))
		{
			if (m_EditorState == EditorPlayState::Stop)
			{
				m_SceneManager.SaveSceneToJson(m_CurrentScenePath);
				if (m_GameManager)
				{
					m_GameManager->TurnReset();
				}
			}
			// Pause -> Play: 단순 재개여야 하므로 TurnReset 금지
			if (auto* currentScene = m_SceneManager.GetCurrentScene().get())
			{
				currentScene->SetIsPause(false);
			}
			//기존 Pause -> Play 시 TurnReset ban
			/*m_GameManager->TurnReset();
			m_SceneManager.GetCurrentScene()->SetIsPause(false);*/
			m_EditorState = EditorPlayState::Play;
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::BeginDisabled(disablePause);
		if (ImGui::Button("Pause", ImVec2(buttonWidth, 0)))
		{
			m_SceneManager.GetCurrentScene()->SetIsPause(true);
			m_EditorState = EditorPlayState::Pause;
		}
		ImGui::EndDisabled();

		ImGui::SameLine();

		ImGui::BeginDisabled(disableStop);
		if (ImGui::Button("Stop", ImVec2(buttonWidth, 0)))
		{
			m_SceneManager.LoadSceneFromJson(m_CurrentScenePath);
			if (m_GameManager)
			{
				m_GameManager->TurnReset();
			}
			m_EditorState = EditorPlayState::Stop;
		}
		ImGui::EndDisabled();

		ImGui::EndMainMenuBar();
	}
}


void EditorApplication::DrawHierarchy() {
	ImGui::Begin("Hierarchy");

	auto scene = m_SceneManager.GetCurrentScene();

	// Scene이 없는 경우는 이젠 없음
	if (!scene) {
		ImGui::Text("Scene Loading Fail");
		ImGui::End();
		return;
	}

	if (scene->GetName() != m_LastSceneName)
	{
		CopyStringToBuffer(scene->GetName(), m_SceneNameBuffer);
		m_LastSceneName = scene->GetName();
	}
	// Scene 이름 변경
	ImGui::Text("Scene");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##SceneName", m_SceneNameBuffer.data(), m_SceneNameBuffer.size());

	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		std::string oldName = scene->GetName();
		std::string newName = m_SceneNameBuffer.data();
		if (!newName.empty() && newName != scene->GetName())
		{
			scene->SetName(newName);
			m_LastSceneName = newName;
			const fs::path oldScenePath = m_CurrentScenePath;
			const fs::path oldSelectedPath = m_SelectedResourcePath;
			fs::path renamedPath = m_CurrentScenePath;
			fs::path renamedSeletedPath = m_SelectedResourcePath;

			if (!m_CurrentScenePath.empty() && m_CurrentScenePath.stem() == oldName)
			{
				renamedPath = m_CurrentScenePath.parent_path() / (newName + m_CurrentScenePath.extension().string());
				if (m_SelectedResourcePath == m_CurrentScenePath)
				{
					renamedSeletedPath = renamedPath;
				}
			}
			m_SelectedResourcePath = renamedSeletedPath;
			m_CurrentScenePath = renamedPath;

			Scene* scenePtr = scene.get();
			m_UndoManager.Push(UndoManager::Command{
				"Rename Scene",
				[this, scenePtr, oldName, oldScenePath, oldSelectedPath]()
				{
					if (!scenePtr)
						return;
					scenePtr->SetName(oldName);
					m_LastSceneName = oldName;
					CopyStringToBuffer(oldName, m_SceneNameBuffer);
					m_CurrentScenePath = oldScenePath;
					m_SelectedResourcePath = oldSelectedPath;
				},
				[this, scenePtr, newName, renamedPath, renamedSeletedPath]()
				{
					if (!scenePtr)
						return;
					scenePtr->SetName(newName);
					m_LastSceneName = newName;
					CopyStringToBuffer(newName, m_SceneNameBuffer);
					m_CurrentScenePath = renamedPath;
					m_SelectedResourcePath = renamedSeletedPath;
				}
				});
		}
		else
		{
			CopyStringToBuffer(scene->GetName(), m_SceneNameBuffer);
		}
	}

	// hier창 우클릭 생성, 오브젝트에서 우클릭 Delete 필요( 추후 수정) 
	if (ImGui::Button("Add GameObject")) // Button
	{
		const std::string name = MakeUniqueObjectName(*scene, "GameObject");
		auto createdObject = scene->CreateGameObject(name); // GameObject 생성 후 바꾸는 걸로 변경했음
		m_SelectedObjectName = name;

		m_SelectedObjectNames.clear();
		m_SelectedObjectNames.insert(name);

		if (createdObject)
		{
			ObjectSnapshot snapshot;
			createdObject->Serialize(snapshot.data);
			Scene* scenePtr = scene.get();

			m_UndoManager.Push(UndoManager::Command{
				"Create GameObject",
				[scenePtr, snapshot]()
				{
					if (!scenePtr)
						return;

					scenePtr->RemoveGameObjectByName(snapshot.data.value("name", ""));

				},
				[scenePtr, snapshot]()
				{
					ApplySnapshot(scenePtr, snapshot);
				}
				});
		}
	}

	std::unordered_map<std::string, std::shared_ptr<GameObject>> objectLookup;

	auto collectObjects = [&](const auto& objects)
		{
			for (const auto& [name, object] : objects)
			{
				if (!object)
				{
					continue;
				}
				objectLookup[name] = object;
			}
		};

	collectObjects(scene->GetGameObjects());

	auto findObjectByName = [&](const std::string& name) -> std::shared_ptr<GameObject>
		{
			if (const auto it = objectLookup.find(name); it != objectLookup.end())
			{
				return it->second;
			}
			return nullptr;
		};
	// 그룹 Select
	auto setPrimarySelection = [&](const std::string& name)
		{
			m_SelectedObjectName = name;
			m_SelectedObjectNames.clear();
			if (!name.empty())
			{
				m_SelectedObjectNames.insert(name);
			}
		};

	auto toggleSelection = [&](const std::string& name)
		{
			if (name.empty())
			{
				return;
			}

			const auto erased = m_SelectedObjectNames.erase(name);
			if (erased > 0)
			{
				if (m_SelectedObjectName == name)
				{
					if (!m_SelectedObjectNames.empty())
					{
						m_SelectedObjectName = *m_SelectedObjectNames.begin();
					}
					else
					{
						m_SelectedObjectName.clear();
					}
				}
				return;
			}

			m_SelectedObjectNames.insert(name);
			m_SelectedObjectName = name;
		};

	// copy
	const std::shared_ptr<GameObject>* selectedObject = nullptr;
	if (const auto it = objectLookup.find(m_SelectedObjectName); it != objectLookup.end())
	{
		selectedObject = &it->second;
	}

	if (!m_SelectedObjectName.empty() && m_SelectedObjectNames.empty())
	{
		if (objectLookup.find(m_SelectedObjectName) != objectLookup.end())
		{
			m_SelectedObjectNames.insert(m_SelectedObjectName);
		}
	}
	for (auto it = m_SelectedObjectNames.begin(); it != m_SelectedObjectNames.end();)
	{
		if (objectLookup.find(*it) == objectLookup.end())
		{
			it = m_SelectedObjectNames.erase(it);
		}
		else
		{
			++it;
		}
	}
	if (!m_SelectedObjectNames.empty() && m_SelectedObjectName.empty())
	{
		m_SelectedObjectName = *m_SelectedObjectNames.begin();
	}

	auto copySelectedObjects = [&](const std::vector<std::shared_ptr<GameObject>>& objects)
		{

			nlohmann::json clipboard = nlohmann::json::object();
			clipboard["objects"] = nlohmann::json::array();


			std::unordered_map<GameObject*, int> objectIds;
			int nextId = 0;
			for (const auto& root : objects)
			{
				if (!root)
				{
					continue;
				}
				std::string rootParentName;
				if (auto* rootTransform = root->GetComponent<TransformComponent>())
				{
					if (auto* rootParent = rootTransform->GetParent())
					{
						if (auto* rootParentOwner = dynamic_cast<GameObject*>(rootParent->GetOwner()))
						{
							if (m_SelectedObjectNames.find(rootParentOwner->GetName()) == m_SelectedObjectNames.end())
							{
								rootParentName = rootParentOwner->GetName();
							}
						}
					}
				}
					std::vector<GameObject*> stack;
					stack.push_back(root.get());
					while (!stack.empty())
					{
						GameObject* current = stack.back();
						stack.pop_back();

						if (!current)
						{
							continue;
						}
						const bool isRoot = (current == root.get());
						auto* currentTransform = current->GetComponent<TransformComponent>();
						GameObject* parentObject = (!isRoot && currentTransform && currentTransform->GetParent())
							? dynamic_cast<GameObject*>(currentTransform->GetParent()->GetOwner())
							: nullptr;

					if (objectIds.find(current) == objectIds.end())
					{
						objectIds[current] = nextId++;
					}


					const int currentId = objectIds[current];
					int parentId = -1;
					if (parentObject)
					{
						if (objectIds.find(parentObject) == objectIds.end())
						{
							objectIds[parentObject] = nextId++;
						}
						parentId = objectIds[parentObject];
					}

					nlohmann::json objectJson;
					current->Serialize(objectJson);
					objectJson["clipboardId"] = currentId;
					objectJson["parentId"] = parentId;

					if (isRoot && !rootParentName.empty())
					{
						objectJson["externalParentName"] = rootParentName;
					}
					clipboard["objects"].push_back(std::move(objectJson));

					if (!currentTransform)
					{
						continue;
					}

					for (auto* childTransform : currentTransform->GetChildrens())
					{
					if (!childTransform)
					{
						continue;
					}
					auto* childObject = dynamic_cast<GameObject*>(childTransform->GetOwner());
					if (childObject)
					{
						stack.push_back(childObject);
					}
				}
			}
		}
			m_ObjectClipboard = std::move(clipboard);
			m_ObjectClipboardHasData = true;
		};

	struct PendingAdd
	{
		nlohmann::json data;
		std::string label;
	};

	std::vector<PendingAdd> pendingAdds;

	auto queuePasteObject = [&](nlohmann::json objectJson, std::string label)
		{
			pendingAdds.push_back(PendingAdd{ std::move(objectJson), std::move(label) });
		};
	auto copySelectedObject = [&](const std::shared_ptr<GameObject>& object)
		{
			if (!object)
			{
				return;
			}
			copySelectedObjects({ object });
		};

	auto hasSelectedAncestor = [&](const std::shared_ptr<GameObject>& object) -> bool
		{
			if (!object)
			{
				return false;
			}
			auto* transform = object->GetComponent<TransformComponent>();
			auto* parent = transform ? transform->GetParent() : nullptr;
			while (parent)
			{
				auto* parentOwner = dynamic_cast<GameObject*>(parent->GetOwner());
				if (parentOwner && m_SelectedObjectNames.find(parentOwner->GetName()) != m_SelectedObjectNames.end())
				{
					return true;
				}
				parent = parent->GetParent();
			}
			return false;
		};

	auto gatherSelectedRoots = [&]() -> std::vector<std::shared_ptr<GameObject>>
		{
			std::vector<std::shared_ptr<GameObject>> roots;
			roots.reserve(m_SelectedObjectNames.size());
			for (const auto& name : m_SelectedObjectNames)
			{
				auto object = findObjectByName(name);
				if (!object)
				{
					continue;
				}
				if (hasSelectedAncestor(object))
				{
					continue;
				}
				roots.push_back(std::move(object));
			}
			return roots;
		};
	auto pasteClipboardObject = [&]()
		{
			if (!m_ObjectClipboardHasData)
			{
				return false;
			}
			if (!m_ObjectClipboard.is_object())
			{
				m_ObjectClipboardHasData = false;
				return false;
			}
			queuePasteObject(m_ObjectClipboard, "Paste GameObject");
			return true;
		};

	std::vector<std::string> pendingDeletes;



	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		ImGuiIO& io = ImGui::GetIO();
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
		{
			auto roots = gatherSelectedRoots();
			if (!roots.empty())
			{
				copySelectedObjects(roots);
			}
			else if (selectedObject && *selectedObject)
			{
				copySelectedObject(*selectedObject);
			}
		}
		if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
		{
			pasteClipboardObject();
		}
		if (ImGui::IsKeyPressed(ImGuiKey_Delete))
		{
			if (!m_SelectedObjectNames.empty())
			{
				pendingDeletes.insert(pendingDeletes.end(), m_SelectedObjectNames.begin(), m_SelectedObjectNames.end());
			}
			else if (selectedObject && *selectedObject)
			{
				pendingDeletes.push_back((*selectedObject)->GetName());
			}
		}
	}


	ImGui::Separator();

	if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (!m_ObjectClipboardHasData)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::MenuItem("Paste"))
		{
			pasteClipboardObject();
		}
		if (!m_ObjectClipboardHasData)
		{
			ImGui::EndDisabled();
		}
		ImGui::EndPopup();
	}


	auto isDescendant = [&](TransformComponent* child, TransformComponent* potentialParent) -> bool
		{
			for (auto* current = potentialParent; current != nullptr; current = current->GetParent())
			{
				if (current == child)
				{
					return true;
				}
			}
			return false;
		};

	auto reparentObject = [&](const std::string& childName, const std::string& parentName) {
		if (childName == parentName) {
			return;
		}
		auto childObject = findObjectByName(childName);
		auto parentObject = findObjectByName(parentName);

		if (!childObject || !parentObject)
		{
			return;
		}

		auto* childTransform = childObject->GetComponent<TransformComponent>();
		auto* parentTransform = parentObject->GetComponent<TransformComponent>();
		if (!childTransform || !parentTransform)
		{
			return;
		}
		if (childTransform->GetParent() == parentTransform)
		{
			return;
		}
		if (isDescendant(childTransform, parentTransform))
		{
			return;
		}

		SceneStateSnapshot beforeState = CaptureSceneState(scene);

		if (childTransform->GetParent())
		{
			childTransform->DetachFromParentKeepWorld();
		}
		childTransform->SetParentKeepWorld(parentTransform);

		SceneStateSnapshot afterState = CaptureSceneState(scene);
		m_UndoManager.Push(UndoManager::Command{
			"Reparent GameObject",
			[this, beforeState]()
			{
				RestoreSceneState(beforeState);
			},
			[this, afterState]()
			{
				RestoreSceneState(afterState);
			}
			});
		};

	auto detachObject = [&](const std::string& childName)
		{
			auto childObject = findObjectByName(childName);
			if (!childObject)
			{
				return;
			}

			auto* childTransform = childObject->GetComponent<TransformComponent>();
			if (!childTransform || !childTransform->GetParent())
			{
				return;
			}

			SceneStateSnapshot beforeState = CaptureSceneState(scene);
			childTransform->DetachFromParentKeepWorld();
			SceneStateSnapshot afterState = CaptureSceneState(scene);
			m_UndoManager.Push(UndoManager::Command{
				"Detach GameObject",
				[this, beforeState]()
				{
					RestoreSceneState(beforeState);
				},
				[this, afterState]()
				{
					RestoreSceneState(afterState);
				}
				});
		};


	std::vector<GameObject*> rootObjects;
	rootObjects.reserve(objectLookup.size());
	for (const auto& [name, object] : objectLookup)
	{
		auto* transform = object->GetComponent<TransformComponent>();
		if (!transform || transform->GetParent() == nullptr)
		{
			rootObjects.push_back(object.get());
		}
	}
	std::sort(rootObjects.begin(), rootObjects.end(),
		[](const GameObject* a, const GameObject* b)
		{
			if (!a || !b)
			{
				return a < b;
			}
			return a->GetName() < b->GetName();
		});

	std::vector<std::string> hierarchyOrder;
	hierarchyOrder.reserve(objectLookup.size());
	auto collectHierarchyOrder = [&](auto&& self, GameObject* object) -> void
		{
			if (!object)
			{
				return;
			}
			hierarchyOrder.push_back(object->GetName());
			auto* transform = object->GetComponent<TransformComponent>();
			if (!transform)
			{
				return;
			}
			for (auto* childTransform : transform->GetChildrens())
			{
				if (!childTransform)
				{
					continue;
				}
				auto* childOwner = dynamic_cast<GameObject*>(childTransform->GetOwner());
				if (!childOwner)
				{
					continue;
				}
				self(self, childOwner);
			}
		};

	for (auto* root : rootObjects)
	{
		collectHierarchyOrder(collectHierarchyOrder, root);
	}

	std::unordered_map<std::string, size_t> hierarchyIndex;
	hierarchyIndex.reserve(hierarchyOrder.size());
	for (size_t i = 0; i < hierarchyOrder.size(); ++i)
	{
		hierarchyIndex[hierarchyOrder[i]] = i;
	}

	auto selectRange = [&](const std::string& startName, const std::string& endName)
		{
			const auto startIt = hierarchyIndex.find(startName);
			const auto endIt = hierarchyIndex.find(endName);
			if (startIt == hierarchyIndex.end() || endIt == hierarchyIndex.end())
			{
				setPrimarySelection(endName);
				return;
			}

			const size_t startIndex = startIt->second;
			const size_t endIndex = endIt->second;
			const size_t rangeStart = std::min(startIndex, endIndex);
			const size_t rangeEnd = (std::max)(startIndex, endIndex);

			m_SelectedObjectNames.clear();
			for (size_t i = rangeStart; i <= rangeEnd && i < hierarchyOrder.size(); ++i)
			{
				m_SelectedObjectNames.insert(hierarchyOrder[i]);
			}
			m_SelectedObjectName = endName;
		};

	auto handleSelectionClick = [&](const std::string& name, ImGuiIO& io)
		{
			if (io.KeyShift && !m_SelectedObjectName.empty())
			{
				selectRange(m_SelectedObjectName, name);
			}
			else if (io.KeyCtrl)
			{
				toggleSelection(name);
			}
			else if (m_SelectedObjectNames.size() > 1
				&& m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end())
			{
				m_SelectedObjectName = name;
			}
			else
			{
				setPrimarySelection(name);
			}
		};

	auto splitDragPayloadNames = [&](const char* payloadData) -> std::vector<std::string>
		{
			std::vector<std::string> names;
			if (!payloadData)
			{
				return names;
			}
			std::string data(payloadData);
			size_t start = 0;
			while (start <= data.size())
			{
				const size_t end = data.find('\n', start);
				const size_t length = (end == std::string::npos) ? data.size() - start : end - start;
				if (length > 0)
				{
					names.emplace_back(data.substr(start, length));
				}
				if (end == std::string::npos)
				{
					break;
				}
				start = end + 1;
			}
			return names;
		};

	auto drawHierarchyNode = [&](auto&& self, GameObject* object) -> void
		{
			if (!object) { return; }
			const std::string& name = object->GetName();
			auto* transform = object->GetComponent<TransformComponent>();
			auto* children = transform ? &transform->GetChildrens() : nullptr;
			const bool hasChildren = children && !children->empty();
			ImGuiIO& io = ImGui::GetIO();

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth;
			if (m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end())
			{
				flags |= ImGuiTreeNodeFlags_Selected;
			}

			ImGui::PushID(object);

			if (!hasChildren) {
				flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

				ImGui::TreeNodeEx(name.c_str(), flags);
				if (ImGui::IsItemClicked())
				{
					handleSelectionClick(name, io);
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					setPrimarySelection(name);
					auto selected = findObjectByName(name);
					if (selected)
					{
						FocusEditorCameraOnObject(selected);
					}
				}

				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
				{
					if (m_SelectedObjectNames.find(name) == m_SelectedObjectNames.end() && !io.KeyCtrl)
					{
						setPrimarySelection(name);
					}
				}

				if (ImGui::BeginPopupContextItem("ObjectContext"))
				{
					auto selected = findObjectByName(name);
					if (ImGui::MenuItem("Copy") && selected)
					{
						auto roots = gatherSelectedRoots();
						if (!roots.empty())
						{
							copySelectedObjects(roots);
						}
						else
						{
							copySelectedObject(selected);
						}
					}
					if (ImGui::MenuItem("Duplicate") && selected)
					{
						auto roots = gatherSelectedRoots();
						if (!roots.empty())
						{
							copySelectedObjects(roots);
						}
						else
						{
							copySelectedObject(selected);
						}

						if (m_ObjectClipboardHasData)
						{
							queuePasteObject(m_ObjectClipboard, "Duplicate GameObject");
						}
					}
					if (ImGui::MenuItem("Delete"))
					{
						if (m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end() && m_SelectedObjectNames.size() > 1)
						{
							pendingDeletes.insert(pendingDeletes.end(), m_SelectedObjectNames.begin(), m_SelectedObjectNames.end());
						}
						else
						{
							pendingDeletes.push_back(name);
						}
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					std::string payloadNames;
					const bool isMulti = m_SelectedObjectNames.size() > 1
						&& m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end();
					if (isMulti)
					{
						auto roots = gatherSelectedRoots();
						for (size_t i = 0; i < roots.size(); ++i)
						{
							if (!roots[i])
							{
								continue;
							}
							if (!payloadNames.empty())
							{
								payloadNames.push_back('\n');
							}
							payloadNames += roots[i]->GetName();
						}
						ImGui::SetDragDropPayload("HIERARCHY_OBJECTS", payloadNames.c_str(), payloadNames.size() + 1);
					}
					else
					{
						ImGui::SetDragDropPayload("HIERARCHY_OBJECT", name.c_str(), name.size() + 1);
					}
					ImGui::TextUnformatted(name.c_str());
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECTS"))
					{
						const char* payloadData = static_cast<const char*>(payload->Data);
						auto names = splitDragPayloadNames(payloadData);
						for (const auto& childName : names)
						{
							reparentObject(childName, name);
						}
					}
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECT"))
					{
						const char* payloadName = static_cast<const char*>(payload->Data);
						if (payloadName)
						{
							reparentObject(payloadName, name);
						}
					}
					ImGui::EndDragDropTarget();
				}
			}
			else {
				const bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), flags);
				if (ImGui::IsItemClicked())
				{
					handleSelectionClick(name, io);
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					setPrimarySelection(name);
					auto selected = findObjectByName(name);
					if (selected)
					{
						FocusEditorCameraOnObject(selected);
					}
				}
				if (ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
				{
					if (m_SelectedObjectNames.find(name) == m_SelectedObjectNames.end() && !io.KeyCtrl)
					{
						setPrimarySelection(name);
					}
				}
				if (ImGui::BeginPopupContextItem("ObjectContext"))
				{
					auto selected = findObjectByName(name);
					if (ImGui::MenuItem("Copy") && selected)
					{
						auto roots = gatherSelectedRoots();
						if (!roots.empty())
						{
							copySelectedObjects(roots);
						}
						else
						{
							copySelectedObject(selected);
						}
					}
					if (ImGui::MenuItem("Duplicate") && selected)
					{
						auto roots = gatherSelectedRoots();
						if (!roots.empty())
						{
							copySelectedObjects(roots);
						}
						else
						{
							copySelectedObject(selected);
						}
						if (m_ObjectClipboardHasData)
						{
							queuePasteObject(m_ObjectClipboard, "Duplicate GameObject");
						}
					}
					if (ImGui::MenuItem("Delete"))
					{
						if (m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end() && m_SelectedObjectNames.size() > 1)
						{
							pendingDeletes.insert(pendingDeletes.end(), m_SelectedObjectNames.begin(), m_SelectedObjectNames.end());
						}
						else
						{
							pendingDeletes.push_back(name);
						}
					}
					ImGui::EndPopup();
				}

				if (ImGui::BeginDragDropSource())
				{
					std::string payloadNames;
					const bool isMulti = m_SelectedObjectNames.size() > 1
						&& m_SelectedObjectNames.find(name) != m_SelectedObjectNames.end();
					if (isMulti)
					{
						auto roots = gatherSelectedRoots();
						for (size_t i = 0; i < roots.size(); ++i)
						{
							if (!roots[i])
							{
								continue;
							}
							if (!payloadNames.empty())
							{
								payloadNames.push_back('\n');
							}
							payloadNames += roots[i]->GetName();
						}
						ImGui::SetDragDropPayload("HIERARCHY_OBJECTS", payloadNames.c_str(), payloadNames.size() + 1);
					}
					else
					{
						ImGui::SetDragDropPayload("HIERARCHY_OBJECT", name.c_str(), name.size() + 1);
					}
					ImGui::TextUnformatted(name.c_str());
					ImGui::EndDragDropSource();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECTS"))
					{
						const char* payloadData = static_cast<const char*>(payload->Data);
						auto names = splitDragPayloadNames(payloadData);
						for (const auto& childName : names)
						{
							reparentObject(childName, name);
						}
					}
					if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECT"))
					{
						const char* payloadName = static_cast<const char*>(payload->Data);
						if (payloadName)
						{
							reparentObject(payloadName, name);
						}
					}
					ImGui::EndDragDropTarget();
				}

				if (nodeOpen)
				{
					for (auto* childTransform : *children)
					{
						if (!childTransform)
						{
							continue;
						}
						auto* childOwner = dynamic_cast<GameObject*>(childTransform->GetOwner());
						if (!childOwner)
						{
							continue;
						}
						self(self, childOwner);
					}
					ImGui::TreePop();
				}
			}

			ImGui::PopID();
		};

	for (auto* root : rootObjects)
	{
		drawHierarchyNode(drawHierarchyNode, root);
	}

	const ImVec2 dropRegion = ImGui::GetContentRegionAvail();
	if (dropRegion.x > 0.0f && dropRegion.y > 0.0f)
	{
		ImGui::InvisibleButton("##HierarchyDropTarget", dropRegion);
		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECTS"))
			{
				const char* payloadData = static_cast<const char*>(payload->Data);
				auto names = splitDragPayloadNames(payloadData);
				for (const auto& childName : names)
				{
					detachObject(childName);
				}
			}
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_OBJECT"))
			{
				const char* payloadName = static_cast<const char*>(payload->Data);
				if (payloadName)
				{
					detachObject(payloadName);
				}
			}
			ImGui::EndDragDropTarget();
		}
	}

	for (auto& pendingAdd : pendingAdds)
	{
		bool didAdd = false;
		SceneStateSnapshot beforeState = CaptureSceneState(scene);

		if (pendingAdd.data.contains("objects") && pendingAdd.data["objects"].is_array())
		{
			std::unordered_map<int, std::shared_ptr<GameObject>> createdObjects;

			for (auto& objectJson : pendingAdd.data["objects"])
			{
				const int clipboardId = objectJson.value("clipboardId", static_cast<int>(createdObjects.size()));
				const std::string baseName = objectJson.value("name", "GameObject");
				const std::string uniqueName = MakeUniqueObjectName(*scene, baseName);

				objectJson["name"] = uniqueName;
				auto newObject = std::make_shared<GameObject>(scene->GetEventDispatcher());
				newObject->Deserialize(objectJson);
				scene->AddGameObject(newObject);
				createdObjects[clipboardId] = newObject;
				m_SelectedObjectName = uniqueName;
				m_SelectedObjectNames.clear();
				m_SelectedObjectNames.insert(uniqueName);
				didAdd = true;
			}
			for (const auto& objectJson : pendingAdd.data["objects"])
				{
				const int clipboardId = objectJson.value("clipboardId", -1);
				const int parentId = objectJson.value("parentId", -1);
				if (clipboardId < 0)
				{
					continue;
				}

				auto childIt = createdObjects.find(clipboardId);
				if (childIt == createdObjects.end())
				{
					continue;
				}

				auto* childTransform = childIt->second->GetComponent<TransformComponent>();
				if (!childTransform)
				{
					continue;
				}

				TransformComponent* parentTransform = nullptr;
				if (parentId >= 0)
				{
					auto parentIt = createdObjects.find(parentId);
					if (parentIt != createdObjects.end())
					{
						parentTransform = parentIt->second->GetComponent<TransformComponent>();
					}
				}
				if (!parentTransform)
				{
					const std::string parentName = objectJson.value("externalParentName", "");
					if (!parentName.empty())
					{
						auto parentObject = findObjectByName(parentName);
						if (parentObject)
						{
							parentTransform = parentObject->GetComponent<TransformComponent>();
						}
					}
				}
				if (!parentTransform)
				{
					continue;
				}

				if (childTransform->GetParent())
				{
					childTransform->DetachFromParentKeepLocal();
				}
				childTransform->SetParentKeepLocal(parentTransform);
			}
				
		}
		else if (pendingAdd.data.contains("components") && pendingAdd.data["components"].is_array())
		{
			const std::string baseName = pendingAdd.data.value("name", "GameObject");
			const std::string uniqueName = MakeUniqueObjectName(*scene, baseName);
			pendingAdd.data["name"] = uniqueName;

			auto newObject = std::make_shared<GameObject>(scene->GetEventDispatcher());
			newObject->Deserialize(pendingAdd.data);
			scene->AddGameObject(newObject);
			m_SelectedObjectName = uniqueName;
			m_SelectedObjectNames.clear();
			m_SelectedObjectNames.insert(uniqueName);
			didAdd = true;
		}

		if (didAdd)
		{
			SceneStateSnapshot afterState = CaptureSceneState(scene);
			m_UndoManager.Push(UndoManager::Command{
				pendingAdd.label,
				[this, beforeState]()
				{
					RestoreSceneState(beforeState);
				},
				[this, afterState]()
				{
					RestoreSceneState(afterState);
				}
				});
		}
	}

	for (const auto& name : pendingDeletes)
	{
		SceneStateSnapshot beforeState = CaptureSceneState(scene);
		scene->RemoveGameObjectByName(name);
		if (m_SelectedObjectName == name)
		{
			m_SelectedObjectName.clear();
		}
		m_SelectedObjectNames.erase(name);
		SceneStateSnapshot afterState = CaptureSceneState(scene);

		m_UndoManager.Push(UndoManager::Command{
			"Delete GameObject",
			[this, beforeState]()
			{
				RestoreSceneState(beforeState);
			},
			[this, afterState]()
			{
				RestoreSceneState(afterState);
			}
			});
	}

	if (m_SelectedObjectName.empty() && !m_SelectedObjectNames.empty())
	{
		m_SelectedObjectName = *m_SelectedObjectNames.begin();
	}

	ImGui::End();

}




void EditorApplication::DrawInspector() {

	ImGui::Begin("Inspector");
	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene) {
		ImGui::Text("No Selected Object");
		ImGui::End();
		return;
	}

	// hierarchy 에서 선택한 object 
	const auto& gameObjects = scene->GetGameObjects();
	const auto  it = gameObjects.find(m_SelectedObjectName);

	if (m_SelectedObjectNames.size() > 1)
	{
		ImGui::Text("Multiple objects selected");
		ImGui::End();
		return;
	}

	// 선택된 오브젝트가 없거나, 실체가 없는 경우
	//second == Object 포인터
	if (it == gameObjects.end() || !it->second)
	{
		ImGui::Text("No Selected GameObject");
		ImGui::End();
		return;
	}

	auto selectedObject = it->second;

	if (m_LastPendingSnapshotScenePath != m_CurrentScenePath)
	{
		m_PendingPropertySnapshots.clear();
		m_LastPendingSnapshotScenePath = m_CurrentScenePath;
	}

	if (m_LastSelectedObjectName != m_SelectedObjectName)
	{
		m_PendingPropertySnapshots.clear();
		CopyStringToBuffer(selectedObject->GetName(), m_ObjectNameBuffer);
		m_LastSelectedObjectName = m_SelectedObjectName;
	}

	ImGui::Text("Name");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(-1);
	ImGui::InputText("##ObjectName", m_ObjectNameBuffer.data(), m_ObjectNameBuffer.size());
	if (ImGui::IsItemDeactivatedAfterEdit())
	{
		std::string newName = m_ObjectNameBuffer.data();
		if (!newName.empty() && newName != selectedObject->GetName())
		{
			const std::string oldName = it->second->GetName();
			if (scene->RenameGameObject(oldName, newName))
			{
				m_SelectedObjectName = newName;
				m_SelectedObjectNames.erase(oldName);
				m_SelectedObjectNames.insert(newName);
				m_LastSelectedObjectName = newName;

				Scene* scenePtr = scene.get();
				m_UndoManager.Push(UndoManager::Command{
					"Rename GameObject",
					[this, scenePtr, oldName, newName]()
					{
						if (!scenePtr)
							return;
						scenePtr->RenameGameObject(newName, oldName);
						m_SelectedObjectName = oldName;
						m_SelectedObjectNames.erase(newName);
						m_SelectedObjectNames.insert(oldName);
						m_LastSelectedObjectName = oldName;
						CopyStringToBuffer(oldName, m_ObjectNameBuffer);
					},
					[this, scenePtr, oldName, newName]()
					{
						if (!scenePtr)
							return;
						scenePtr->RenameGameObject(oldName, newName);
						m_SelectedObjectName = newName;
						m_SelectedObjectNames.erase(oldName);
						m_SelectedObjectNames.insert(newName);
						m_LastSelectedObjectName = newName;
						CopyStringToBuffer(newName, m_ObjectNameBuffer);
					}
					});
			}
			else
			{
				CopyStringToBuffer(selectedObject->GetName(), m_ObjectNameBuffer);
			}
		}
		else
		{
			CopyStringToBuffer(selectedObject->GetName(), m_ObjectNameBuffer);
		}
	}
	ImGui::Separator();
	ImGui::Text("Components");
	ImGui::Separator();


	for (const auto& typeName : selectedObject->GetComponentTypeNames()) {
		if (IsUIComponentType(typeName))
		{
			continue;
		}
		// 여기서 각 Component별 Property와 조작까지 생성?
		ImGui::PushID(typeName.c_str());
		const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
		const bool nodeOpen = ImGui::TreeNodeEx(typeName.c_str(), flags);
		if (ImGui::BeginPopupContextItem("ComponentContext"))
		{
			//const bool canRemove = (typeName != "TransformComponent"); //일단 Trasnsform은 삭제 막아둠
			/*if (!canRemove)
			{
				ImGui::BeginDisabled();
			}*/

			//삭제
			if (ImGui::MenuItem("Remove Component"))
			{
				ObjectSnapshot beforeSnapshot;
				selectedObject->Serialize(beforeSnapshot.data);

				if (selectedObject->RemoveComponentByTypeName(typeName))
				{
					ObjectSnapshot afterSnapshot;
					selectedObject->Serialize(afterSnapshot.data);

					Scene* scenePtr = scene.get();
					const std::string label = "Remove Component " + typeName;
					m_UndoManager.Push(UndoManager::Command{
						label,
						[scenePtr, beforeSnapshot]()
						{
							ApplySnapshot(scenePtr, beforeSnapshot);
						},
						[scenePtr, afterSnapshot]()
						{
							ApplySnapshot(scenePtr, afterSnapshot);
						}
						});
				}
			}
			/*	if (!canRemove)
				{
					ImGui::EndDisabled();
				}*/
			ImGui::EndPopup();
		}
		if (nodeOpen)
		{
			Component* component = selectedObject->GetComponentByTypeName(typeName);
			auto* typeInfo = ComponentRegistry::Instance().Find(typeName);

			if (component && typeInfo)
			{
				auto props = ComponentRegistry::Instance().CollectProperties(typeInfo);

				for (const auto& prop : props)
				{
					const PropertyEditResult editResult = DrawComponentPropertyEditor(component, *prop, *m_AssetLoader);
					const bool updated = editResult.updated;
					const bool activated = editResult.activated;
					const bool deactivated = editResult.deactivated;
					const size_t propertyKey = MakePropertyKey(component, prop->GetName());

					if (activated && m_PendingPropertySnapshots.find(propertyKey) == m_PendingPropertySnapshots.end())
					{
						PendingPropertySnapshot pendingSnapshot;
						selectedObject->Serialize(pendingSnapshot.beforeSnapshot.data);
						m_PendingPropertySnapshots.emplace(propertyKey, std::move(pendingSnapshot));
					}

					if (updated)
					{
						auto itSnapshot = m_PendingPropertySnapshots.find(propertyKey);
						if (itSnapshot != m_PendingPropertySnapshots.end())
						{
							itSnapshot->second.updated = true;
						}
					}

					if (deactivated)
					{
						auto itSnapshot = m_PendingPropertySnapshots.find(propertyKey);
						if (itSnapshot != m_PendingPropertySnapshots.end())
						{
							if (itSnapshot->second.updated)
							{
								ObjectSnapshot beforeSnapshot = itSnapshot->second.beforeSnapshot;
								ObjectSnapshot afterSnapshot;
								selectedObject->Serialize(afterSnapshot.data);

								Scene* scenePtr = scene.get();
								const std::string label = "Edit " + prop->GetName();
								m_UndoManager.Push(UndoManager::Command{
									label,
									[scenePtr, beforeSnapshot]()
									{
										ApplySnapshot(scenePtr, beforeSnapshot);
									},
									[scenePtr, afterSnapshot]()
									{
										ApplySnapshot(scenePtr, afterSnapshot);
									}
									});
							}
							m_PendingPropertySnapshots.erase(itSnapshot);
						}
					}
				}

				if (auto* meshComponent = dynamic_cast<MeshComponent*>(component))
				{
					ObjectSnapshot beforeSnapshot;
					selectedObject->Serialize(beforeSnapshot.data);

					const bool updated = DrawSubMeshOverridesEditor(*meshComponent, *m_AssetLoader);

					if (updated)
					{
						ObjectSnapshot afterSnapshot;
						selectedObject->Serialize(afterSnapshot.data);

						Scene* scenePtr = scene.get();
						m_UndoManager.Push(UndoManager::Command{
							"Edit SubMesh Overrides",
							[scenePtr, beforeSnapshot]()
							{
								ApplySnapshot(scenePtr, beforeSnapshot);
							},
							[scenePtr, afterSnapshot]()
							{
								ApplySnapshot(scenePtr, afterSnapshot);
							}
							});
					}
				}
				ImGui::Separator();
			}
			else
			{
				ImGui::TextDisabled("Component data unavailable.");
			}

			ImGui::TreePop();
		}
		ImGui::PopID();
	}



	if (ImGui::Button("Add Component"))
	{
		ImGui::OpenPopup("AddComponentPopup");
	}

	if (ImGui::BeginPopup("AddComponentPopup"))
	{
		// Logic 
		const auto existingTypes = selectedObject->GetComponentTypeNames(); //
		const auto typeNames = ComponentRegistry::Instance().GetTypeNames();
		// 숨길Components
		static const std::unordered_set<std::string> kHiddenTypes = {
			"TransformComponent",
			"MeshComponent",
			"MaterialComponent",
			"SkeletalMeshComponent",
			"PlayerStatComponent",
			"PlayerMovementComponent",
			"PlayerFSMComponent",
			"PlayerMoveFSMComponent",
			"PlayerPushFSMComponent",
			"PlayerCombatFSMComponent",
			"PlayerInventoryFSMComponent",
			"PlayerShopFSMComponent",
			"PlayerDoorFSMComponent",
			"EnemyStatComponent",
			"EnemyMovementComponent",
			"LightComponent",
			"AnimationComponent",
			"AnimFSMComponent",
			"StatComponent",
			"FSMComponent"
		};
		for (const auto& typeName : typeNames)
		{
			if (kHiddenTypes.contains(typeName))
			{
				continue;
			}
			if (IsUIComponentType(typeName))
			{
				continue;
			}

			const bool hasType = std::find(existingTypes.begin(), existingTypes.end(), typeName) != existingTypes.end();
			//const bool disallowDuplicate = (typeName == "TransformComponent");
			const bool disabled = hasType /*&& disallowDuplicate*/;

			if (disabled)
			{
				ImGui::BeginDisabled();
			}

			if (ImGui::MenuItem(typeName.c_str()))
			{
				ObjectSnapshot beforeSnapshot;
				selectedObject->Serialize(beforeSnapshot.data);

				auto comp = ComponentFactory::Instance().Create(typeName);
				if (comp)
				{
					selectedObject->AddComponent(std::move(comp));

					ObjectSnapshot afterSnapshot;
					selectedObject->Serialize(afterSnapshot.data);

					Scene* scenePtr = scene.get();
					const std::string label = "Add Component " + typeName;
					m_UndoManager.Push(UndoManager::Command{
						label,
						[scenePtr, beforeSnapshot]()
						{
							ApplySnapshot(scenePtr, beforeSnapshot);
						},
						[scenePtr, afterSnapshot]()
						{
							ApplySnapshot(scenePtr, afterSnapshot);
						}
						});
				}
			}

			if (disabled)
			{
				ImGui::EndDisabled();
			}
		}
		ImGui::EndPopup();
	} // 


	ImGui::End();
}
void EditorApplication::DrawFolderView()
{
	ImGui::Begin("Folder");

	//ImGui::Text("Need Logic");
	//logic
	if (!std::filesystem::exists(m_ResourceRoot)) {
		// resource folder 인식 문제방지
		ImGui::Text("Resources folder not found: %s", m_ResourceRoot.string().c_str());
		ImGui::End();
		return;
	}

	// new Scene 생성 -> Directional light / Main Camera Default 생성
	auto createNewScene = [&]()
		{
			const std::string baseName = "NewScene";
			std::filesystem::path newPath;
			for (int index = 0; index < 10000; ++index)
			{
				std::string name = (index == 0) ? baseName : baseName + std::to_string(index);
				newPath = m_ResourceRoot / (name + ".json");
				if (!std::filesystem::exists(newPath))
				{
					break;
				}
			}

			SceneStateSnapshot beforeState = CaptureSceneState(m_SceneManager.GetCurrentScene());
			SceneFileSnapshot beforeFile = CaptureFileSnapshot(newPath);

			if (m_SceneManager.CreateNewScene(newPath))
			{
				m_CurrentScenePath = newPath;
				m_SelectedResourcePath = newPath;
				m_SelectedObjectName.clear();
				m_SelectedObjectNames.clear();
				m_LastSelectedObjectName.clear();
				m_ObjectNameBuffer.fill('\0');

				SceneStateSnapshot afterState = CaptureSceneState(m_SceneManager.GetCurrentScene());
				SceneFileSnapshot  afterFile = CaptureFileSnapshot(newPath);

				m_UndoManager.Push(UndoManager::Command{
					"Create Scene",
					[this, beforeState, beforeFile]()
					{
						RestoreSceneState(beforeState);
						RestoreFileSnapshot(beforeFile);
					},
					[this, afterState, afterFile]()
					{
						RestoreSceneState(afterState);
						RestoreFileSnapshot(afterFile);
					}
					});
			}
		};
	ImGui::BeginDisabled(m_EditorState == EditorPlayState::Play || m_EditorState == EditorPlayState::Pause);
	if (ImGui::Button("New Scene"))
	{
		createNewScene();
	}
	//ImGui::EndDisabled();

	ImGui::SameLine();
	// Play 중일때 저장 불가
	//ImGui::BeginDisabled(m_EditorState == EditorPlayState::Play || m_EditorState == EditorPlayState::Pause); 
	if (ImGui::Button("Save")) {
		auto scene = m_SceneManager.GetCurrentScene();
		if (scene)
		{
			std::filesystem::path savePath = m_CurrentScenePath;

			if (savePath.empty())
			{
				savePath = m_ResourceRoot / (scene->GetName() + ".json");
			}
			m_PendingSavePath = savePath;
			m_OpenSaveConfirm = true;
			m_PendingSavePath = savePath;
			m_OpenSaveConfirm = true;
		}
	}



	ImGui::SameLine();
	const bool canDelete = !m_SelectedResourcePath.empty() && m_SelectedResourcePath.extension() == ".json";
	if (!canDelete)
	{
		ImGui::BeginDisabled();
	}
	if (ImGui::Button("Delete"))
	{
		m_PendingDeletePath = m_SelectedResourcePath;
		m_OpenDeleteConfirm = true;
	}
	if (!canDelete)
	{
		ImGui::EndDisabled();
	}
	
	ImGui::SameLine();
	if (m_CurrentScenePath.empty())
	{
		ImGui::Text("Current Scene: None");
	}
	else
	{
		ImGui::Text("Current Scene: %s", m_CurrentScenePath.filename().string().c_str());
	}

	if (m_OpenSaveConfirm)
	{
		ImGui::OpenPopup("Confirm Save");
		m_OpenSaveConfirm = false;
	}

	if (m_OpenDeleteConfirm)
	{
		ImGui::OpenPopup("Confirm Delete");
		m_OpenDeleteConfirm = false;
	}

	if (ImGui::BeginPopupContextWindow("FolderContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
	{
		if (ImGui::MenuItem("New Scene"))
		{
			createNewScene();
		}
		ImGui::EndPopup();
	}

	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Confirm Save", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("Save scene \"%s\"?", m_PendingSavePath.filename().string().c_str());
		if (ImGui::Button("Save"))
		{
			const fs::path beforeCurrentPath = m_CurrentScenePath;
			const fs::path beforeSelectedPath = m_SelectedResourcePath;
			SceneFileSnapshot beforeFile = CaptureFileSnapshot(m_PendingSavePath);

			if (m_SceneManager.SaveSceneToJson(m_PendingSavePath))
			{
				m_CurrentScenePath = m_PendingSavePath;
				m_SelectedResourcePath = m_PendingSavePath;

				const fs::path afterCurrentPath = m_CurrentScenePath;
				const fs::path afterSelectedPath = m_SelectedResourcePath;
				SceneFileSnapshot afterFile = CaptureFileSnapshot(m_PendingSavePath);

				m_UndoManager.Push(UndoManager::Command{
					"Save Scene",
					[this, beforeCurrentPath, beforeSelectedPath, beforeFile]()
					{
						RestoreFileSnapshot(beforeFile);
						m_CurrentScenePath = beforeCurrentPath;
						m_SelectedResourcePath = beforeSelectedPath;
					},
					[this, afterCurrentPath, afterSelectedPath, afterFile]()
					{
						RestoreFileSnapshot(afterFile);
						m_CurrentScenePath = afterCurrentPath;
						m_SelectedResourcePath = afterSelectedPath;
					}
					});
			}
			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal("Confirm Delete", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{

		if (ImGui::Button("Delete"))
		{
			const fs::path targetPath = m_PendingDeletePath;
			const bool wasCurrent = (m_CurrentScenePath == targetPath);
			const bool wasSelected = (m_SelectedResourcePath == targetPath);
			SceneFileSnapshot beforeFile = CaptureFileSnapshot(targetPath);

			std::error_code error;
			std::filesystem::remove(targetPath, error);
			if (!error)
			{
				if (wasCurrent)
				{
					m_CurrentScenePath.clear();
				}
				if (wasSelected)
				{
					m_SelectedResourcePath.clear();
				}
			}

			SceneFileSnapshot afterFile;
			afterFile.path = targetPath;
			afterFile.existed = false;

			m_UndoManager.Push(UndoManager::Command{
				"Delete Scene File",
				[this, beforeFile, wasCurrent, wasSelected]()
				{
					RestoreFileSnapshot(beforeFile);
					if (wasCurrent)
					{
						m_CurrentScenePath = beforeFile.path;
					}
					if (wasSelected)
					{
						m_SelectedResourcePath = beforeFile.path;
					}
				},
				[this, afterFile, wasCurrent, wasSelected]()
				{
					RestoreFileSnapshot(afterFile);
					if (wasCurrent)
					{
						m_CurrentScenePath = afterFile.path;
					}
					if (wasSelected)
					{
						m_SelectedResourcePath = afterFile.path;
					}
				}
				});

			ImGui::CloseCurrentPopup();
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel"))
		{
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}

	ImGui::Separator();

	const auto drawDirectory = [&](const auto& self, const std::filesystem::path& dir) -> void
		{
			std::vector<std::filesystem::directory_entry> entries;
			for (const auto& entry : std::filesystem::directory_iterator(dir))
			{
				entries.push_back(entry);
			}

			std::sort(entries.begin(), entries.end(),
				[](const auto& a, const auto& b)
				{
					if (a.is_directory() != b.is_directory())
					{
						return a.is_directory() > b.is_directory();
					}
					return a.path().filename().string() < b.path().filename().string();
				});

			for (const auto& entry : entries)
			{
				const auto name = entry.path().filename().string();
				if (entry.is_directory())
				{
					const std::string label = name;
					const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
					if (ImGui::TreeNodeEx(label.c_str(), flags))
					{
						self(self, entry.path());
						ImGui::TreePop();
					}
				}
				else
				{
					const std::string label = name;
					const bool selected = (m_SelectedResourcePath == entry.path());
					const bool isSceneFile = entry.path().extension() == ".json";

					if (isSceneFile) {
						m_SceneManager.RegisterSceneFromJson(entry.path()); //Scene 모두 등록
					}
					ImGui::PushID(label.c_str());
					if (isSceneFile && selected)
					{
						ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
						ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
					}

					bool clicked = false;
					if (isSceneFile)
					{
						clicked = ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0.0f));
					}
					else
					{
						clicked = ImGui::Selectable(label.c_str(), selected);
					}

					if (isSceneFile && selected)
					{
						ImGui::PopStyleColor(3);
					}

					if (ImGui::BeginPopupContextItem("SceneItemContext"))
					{
						if (isSceneFile && ImGui::MenuItem("Delete"))
						{
							m_PendingDeletePath = entry.path();
							m_OpenDeleteConfirm = true;
						}
						ImGui::EndPopup();
					}

					if (clicked)
					{
						SceneStateSnapshot beforeState = CaptureSceneState(m_SceneManager.GetCurrentScene());
						m_SelectedResourcePath = entry.path();
						if (entry.path().extension() == ".json")
						{
							if (m_SceneManager.LoadSceneFromJson(entry.path()))
							{
								m_CurrentScenePath = entry.path();
								m_SelectedObjectName.clear();
								m_SelectedObjectNames.clear();
								m_LastSelectedObjectName.clear();
								m_ObjectNameBuffer.fill('\0');

								SceneStateSnapshot afterState = CaptureSceneState(m_SceneManager.GetCurrentScene());

								m_UndoManager.Push(UndoManager::Command{
									"Load Scene",
									[this, beforeState]()
									{
										if (beforeState.hasScene)
										{
											m_SceneManager.LoadSceneFromJsonData(beforeState.data, beforeState.currentPath);
										}
										m_CurrentScenePath = beforeState.currentPath;
										m_SelectedResourcePath = beforeState.selectedPath;
										m_SelectedObjectName = beforeState.selectedObjectName;
										m_LastSelectedObjectName = beforeState.lastSelectedObjectName;
										m_SelectedObjectNames.clear();
										if (!m_SelectedObjectName.empty())
										{
											m_SelectedObjectNames.insert(m_SelectedObjectName);
										}
										m_ObjectNameBuffer = beforeState.objectNameBuffer;
										m_LastSceneName = beforeState.lastSceneName;
										m_SceneNameBuffer = beforeState.sceneNameBuffer;
									},
									[this, afterState]()
									{
										if (afterState.hasScene)
										{
											m_SceneManager.LoadSceneFromJsonData(afterState.data, afterState.currentPath);
										}
										m_CurrentScenePath = afterState.currentPath;
										m_SelectedResourcePath = afterState.selectedPath;
										m_SelectedObjectName = afterState.selectedObjectName;
										m_LastSelectedObjectName = afterState.lastSelectedObjectName;
										m_SelectedObjectNames.clear();
										if (!m_SelectedObjectName.empty())
										{
											m_SelectedObjectNames.insert(m_SelectedObjectName);
										}
										m_ObjectNameBuffer = afterState.objectNameBuffer;
										m_LastSceneName = afterState.lastSceneName;
										m_SceneNameBuffer = afterState.sceneNameBuffer;
									}
									});
							}
						}
						
					}
					ImGui::PopID();
				}
			}
			ImGui::EndDisabled();
		};

	drawDirectory(drawDirectory, m_ResourceRoot);
	ImGui::End();
}


void EditorApplication::DrawResourceBrowser()
{
	ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_Once);
	ImGui::Begin("Resource Browser");

	auto buildSortedEntries = [](const auto& store)
		{
			using HandleType = std::decay_t<decltype(store.GetKeyToHandle().begin()->second)>;
			std::vector<std::pair<std::string, HandleType>> entries;
			const auto& keyToHandle = store.GetKeyToHandle();
			entries.reserve(keyToHandle.size());
			for (const auto& [key, handle] : keyToHandle)
			{
				const std::string* displayName = store.GetDisplayName(handle);
				std::string name = (displayName && !displayName->empty()) ? *displayName : key;
				entries.emplace_back(std::move(name), handle);
			}
			std::sort(entries.begin(), entries.end(), [](const auto& left, const auto& right)
				{
					return left.first < right.first;
				});
			return entries;
		};

	if (ImGui::BeginTabBar("ResourceTabs"))
	{
		ImVec2 avail = ImGui::GetContentRegionAvail();
		//Meshes
		if (ImGui::BeginTabItem("Meshes"))
		{
			ImGui::BeginChild("MeshesScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto meshes = buildSortedEntries(m_AssetLoader->GetMeshes());
			for (const auto& [name, handle] : meshes)
			{
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name.c_str());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_MESH", &handle, sizeof(MeshHandle));
					ImGui::Text("Mesh : %s", name.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		//Materials
		if (ImGui::BeginTabItem("Materials"))
		{
			ImGui::BeginChild("MaterialsScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto materials = buildSortedEntries(m_AssetLoader->GetMaterials());
			for (const auto& [name, handle] : materials)
			{
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name.c_str());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_MATERIAL", &handle, sizeof(MaterialHandle));
					ImGui::Text("Material : %s", name);
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		//Textures
		if (ImGui::BeginTabItem("Textures"))
		{
			ImGui::BeginChild("TexturesScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto textures = buildSortedEntries(m_AssetLoader->GetTextures());
			for (const auto& [name, handle] : textures) 
			{
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name.c_str());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_TEXTURE", &handle, sizeof(TextureHandle));
					ImGui::Text("Texture : %s", name.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		//Skeletons
		if (ImGui::BeginTabItem("Skeletons"))
		{
			ImGui::BeginChild("SkeletonsScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto skeletons = buildSortedEntries(m_AssetLoader->GetSkeletons());
			for (const auto& [name, handle] : skeletons)
			{
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name.c_str());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_SKELETON", &handle, sizeof(SkeletonHandle));
					ImGui::Text("Skeleton : %s", name.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		//Animations
		if (ImGui::BeginTabItem("Animations"))
		{
			ImGui::BeginChild("AnimationsScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto animations = buildSortedEntries(m_AssetLoader->GetAnimations());
			for (const auto& [name, handle] : animations) 
			{
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name.c_str());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_ANIMATION", &handle, sizeof(AnimationHandle));
					ImGui::Text("Animation : %s", name.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}


		//Vertex Shaders
		if (ImGui::BeginTabItem("Vertex Shaders"))
		{
			ImGui::BeginChild("VertexShadersScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto shaders = buildSortedEntries(m_AssetLoader->GetVertexShaders());
			for (const auto& [name, handle] : shaders) 
			{
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name.c_str());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_VERTEX_SHADER", &handle, sizeof(VertexShaderHandle));
					ImGui::Text("Vertex Shader : %s", name.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		//Pixel Shaders
		if (ImGui::BeginTabItem("Pixel Shaders"))
		{
			ImGui::BeginChild("PixelShadersScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto shaders = buildSortedEntries(m_AssetLoader->GetPixelShaders());
			for (const auto& [name, handle] : shaders) 
			{
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name.c_str());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_PIXEL_SHADER", &handle, sizeof(PixelShaderHandle));
					ImGui::Text("Pixel Shader : %s", name.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		//Shader Assets
		if (ImGui::BeginTabItem("Shader Assets"))
		{
			static char shaderAssetName[128] = "";
			static char shaderVertexPath[256] = "";
			static char shaderPixelPath[256] = "";

			if (ImGui::Button("New Shader Asset"))
			{
				ImGui::OpenPopup("CreateShaderAssetPopup");
			}

			if (ImGui::BeginPopup("CreateShaderAssetPopup"))
			{
				ImGui::InputText("Name", shaderAssetName, sizeof(shaderAssetName));
				ImGui::InputText("VS Path", shaderVertexPath, sizeof(shaderVertexPath));
				ImGui::InputText("PS Path", shaderPixelPath, sizeof(shaderPixelPath));

				if (ImGui::Button("Create"))
				{
					const fs::path resourceRoot = "../ResourceOutput";
					const fs::path assetDir = resourceRoot / "ShaderAsset";
					const fs::path metaDir = assetDir / "Meta";
					const fs::path metaPath = metaDir / (std::string(shaderAssetName) + ".shader.json");
					// “실제 shader 파일들이 있는 폴더(절대/상대 상관없음, 하지만 path로 관리)”
					const fs::path shaderDirAbsOrRel = "../MRenderer/fx/";

					// meta 기준 상대경로 prefix 생성 (Meta -> shaderDir)
					const fs::path shaderDirFromMeta = fs::relative(shaderDirAbsOrRel, metaDir).lexically_normal();

					// 최종 vs/ps 경로(메타 기준 상대경로)
					const fs::path vsRel = (shaderDirFromMeta / shaderVertexPath).lexically_normal();
					const fs::path psRel = (shaderDirFromMeta / shaderPixelPath).lexically_normal();

					if (!shaderAssetName[0])
					{
						ImGui::CloseCurrentPopup();
					}
					else
					{
						auto makeShaderPath = [&](const char* input, const char* fallbackSuffix) -> std::string
							{
								std::string path = input && input[0] ? input : "";
								if (path.empty())
								{
									path = std::string(shaderAssetName) + fallbackSuffix;
								}

								const fs::path parsed = fs::path(path);
								if (parsed.extension().empty())
								{
									return (parsed.string() + ".hlsl");
								}
								return parsed.string();
							};
						fs::create_directories(metaDir);
						json meta = json::object();
						meta["name"] = shaderAssetName;
						meta["vs"] = makeShaderPath(vsRel.generic_string().c_str(), "_VS");
						meta["ps"] = makeShaderPath(psRel.generic_string().c_str(), "_PS");

						std::ofstream out(metaPath);
						if (out)
						{
							out << meta.dump(4);
							out.close();
							if (m_AssetLoader)
							{
								m_AssetLoader->LoadShaderAsset(metaPath.string());
							}
						}
						shaderAssetName[0] = '\0';
						shaderVertexPath[0] = '\0';
						shaderPixelPath[0] = '\0';
						ImGui::CloseCurrentPopup();
					}
				}

				ImGui::SameLine();
				if (ImGui::Button("Cancel"))
				{
					ImGui::CloseCurrentPopup();
				}

				ImGui::EndPopup();
			}

			ImGui::Separator();

			ImGui::BeginChild("ShaderAssetsScroll", ImVec2(avail.x, avail.y), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

			const auto shaderAssets = buildSortedEntries(m_AssetLoader->GetShaderAssets());
			for (const auto& [name, handle] : shaderAssets) 
			{
				ImGui::PushID((int)handle.id); // 충돌 방지
				ImGui::Selectable(name.c_str());

				if (ImGui::BeginDragDropSource())
				{
					ImGui::SetDragDropPayload("RESOURCE_SHADER_ASSET", &handle, sizeof(ShaderAssetHandle));
					ImGui::Text("Shader Asset : %s", name.c_str());
					ImGui::EndDragDropSource();
				}
				ImGui::Separator();
				ImGui::PopID();
			}

			ImGui::EndChild();

			ImGui::EndTabItem();
		}

		ImGui::EndTabBar();
	}

	ImGui::End();
}

void EditorApplication::DrawGizmo()
{
	if (!m_EditorViewport.HasViewportRect())
	{
		return;
	}

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}

	if (m_SelectedObjectName.empty())
	{
		return;
	}

	auto editorCamera = scene->GetEditorCamera().get();
	if (!editorCamera)
	{
		return;
	}

	const auto& gameObjects = scene->GetGameObjects();

	std::shared_ptr<GameObject> selectedObject;
	if (const auto it = gameObjects.find(m_SelectedObjectName); it != gameObjects.end())
	{
		selectedObject = it->second;
	}

	if (!selectedObject)
	{
		return;
	}

	auto* transform = selectedObject->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	static ImGuizmo::OPERATION currentOperation = ImGuizmo::TRANSLATE;
	static ImGuizmo::MODE currentMode = ImGuizmo::WORLD;
	static bool useSnap = false;
	static float snapValues[3]{ 1.0f, 1.0f, 1.0f };

	ImGui::Begin("Gizmo");
	if (ImGui::RadioButton("Translate", currentOperation == ImGuizmo::TRANSLATE) || ImGui::IsKeyPressed(ImGuiKey_T))
	{
		currentOperation = ImGuizmo::TRANSLATE;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Rotate", currentOperation == ImGuizmo::ROTATE) || ImGui::IsKeyPressed(ImGuiKey_R))
	{
		currentOperation = ImGuizmo::ROTATE;
	}
	ImGui::SameLine();
	if (ImGui::RadioButton("Scale", currentOperation == ImGuizmo::SCALE) || ImGui::IsKeyPressed(ImGuiKey_S))
	{
		currentOperation = ImGuizmo::SCALE;
	}

	if (currentOperation != ImGuizmo::SCALE)
	{
		if (ImGui::RadioButton("Local", currentMode == ImGuizmo::LOCAL))
		{
			currentMode = ImGuizmo::LOCAL;
		}
		ImGui::SameLine();
		if (ImGui::RadioButton("World", currentMode == ImGuizmo::WORLD))
		{
			currentMode = ImGuizmo::WORLD;
		}
	}
	ImGui::Checkbox("Snap", &useSnap);
	if (useSnap)
	{
		ImGui::InputFloat3("Snap Value", snapValues);
	}
	ImGui::End();

	const ImVec2 rectMin = m_EditorViewport.GetViewportRectMin();
	const ImVec2 rectMax = m_EditorViewport.GetViewportRectMax();
	const ImVec2 rectSize = ImVec2(rectMax.x - rectMin.x, rectMax.y - rectMin.y);

	XMFLOAT4X4 view = editorCamera->GetViewMatrix();
	XMFLOAT4X4 proj = editorCamera->GetProjMatrix();
	XMFLOAT4X4 world = transform->GetWorldMatrix();

	ImGuizmo::SetOrthographic(false);
	ImGuizmo::BeginFrame();
	if (auto* drawList = m_EditorViewport.GetDrawList())
	{
		ImGuizmo::SetDrawlist(drawList);
	}
	ImGuizmo::SetRect(rectMin.x, rectMin.y, rectSize.x, rectSize.y);

	ImGuizmo::Manipulate(
		&view._11,
		&proj._11,
		currentOperation,
		currentMode,
		&world._11,
		nullptr,
		useSnap ? snapValues : nullptr);

	static bool wasUsing = false;
	static bool gizmoUpdated = false;
	static bool hasSnapshot = false;
	static ObjectSnapshot beforeSnapshot{};
	static std::string pendingObjectName;

	const bool usingNow = ImGuizmo::IsUsing();

	if (usingNow && !wasUsing)
	{
		selectedObject->Serialize(beforeSnapshot.data);
		pendingObjectName = selectedObject->GetName();
		gizmoUpdated = false;
		hasSnapshot = true;
	}

	if (usingNow)
	{
		XMFLOAT4X4 local = world;
		if (auto* parent = transform->GetParent())
		{
			local = MathUtils::Mul(world, parent->GetInverseWorldMatrix());
		}

		XMFLOAT3 position{};
		XMFLOAT4 rotation{};
		XMFLOAT3 scale{};
		MathUtils::DecomposeMatrix(local, position, rotation, scale);

		transform->SetPosition(position);
		transform->SetRotation(rotation);
		transform->SetScale(scale);
		gizmoUpdated = true;
	}

	if (!usingNow && wasUsing && hasSnapshot && gizmoUpdated)
	{
		if (auto targetObject = FindSceneObject(scene.get(), pendingObjectName))
		{
			ObjectSnapshot afterSnapshot;
			targetObject->Serialize(afterSnapshot.data);

			const char* opLabel = "Gizmo Edit";
			switch (currentOperation)
			{
			case ImGuizmo::TRANSLATE:
				opLabel = "Gizmo Translate";
				break;
			case ImGuizmo::ROTATE:
				opLabel = "Gizmo Rotate";
				break;
			case ImGuizmo::SCALE:
				opLabel = "Gizmo Scale";
				break;
			default:
				break;
			}

			Scene* scenePtr = scene.get();
			ObjectSnapshot undoSnapshot = beforeSnapshot;
			m_UndoManager.Push(UndoManager::Command{
				opLabel,
				[scenePtr, undoSnapshot]()
				{
					ApplySnapshot(scenePtr, undoSnapshot);
				},
				[scenePtr, afterSnapshot]()
				{
					ApplySnapshot(scenePtr, afterSnapshot);
				}
				});
		}
	}

	if (!usingNow && wasUsing)
	{
		hasSnapshot = false;
		gizmoUpdated = false;
		pendingObjectName.clear();
	}

	wasUsing = usingNow;
}

void EditorApplication::DrawUIEditorPreview()
{
	ImGui::SetNextWindowSize(ImVec2(2560, 1600), ImGuiCond_Once);
	ImGui::Begin("UI Editor");
	ImGui::TextDisabled("UI Editor Preview");
	ImGui::Separator();

	ImGui::BeginDisabled(m_EditorState == EditorPlayState::Play || m_EditorState == EditorPlayState::Pause);
	if (ImGui::Button("Save Scene"))
	{
		auto scene = m_SceneManager.GetCurrentScene();
		if (scene)
		{
			std::filesystem::path savePath = m_CurrentScenePath;
			if (savePath.empty())
			{
				savePath = m_ResourceRoot / (scene->GetName() + ".json");
			}
			m_PendingSavePath = savePath;
			m_OpenSaveConfirm = true;
		}
	}
	ImGui::EndDisabled();
	ImGui::Separator();

	ImGui::Columns(2, "UIDemoColumns", true);

	ImGui::BeginChild("WidgetTreePanel", ImVec2(0, 0), true);
	ImGui::Text("UI Objects");
	ImGui::Separator();

	auto scene = m_SceneManager.GetCurrentScene();
	UIManager* uiManager = m_SceneManager.GetUIManager();
	if (!scene || !uiManager)
	{
		ImGui::TextDisabled("UIManager Or Scene Not Ready.");
	}
	else
	{
		const auto& uiObjectsByScene = uiManager->GetUIObjects();
		const auto sceneName = scene->GetName();
		const auto it = uiObjectsByScene.find(sceneName);

		auto getUIObjectByName = [&](const std::string& name) -> std::shared_ptr<UIObject>
			{
				if (it == uiObjectsByScene.end())
				{
					return nullptr;
				}

				const auto found = it->second.find(name);
				if (found == it->second.end())
				{
					return nullptr;
				}

				return found->second;
			};

		auto pushUISnapshotUndo = [&](const std::string& label, const nlohmann::json& beforeSnapshot, const nlohmann::json& afterSnapshot)
			{
				m_UndoManager.Push(UndoManager::Command{
					label,
					[this, uiManager, sceneName, beforeSnapshot]()
					{
						if (!uiManager)
							return;

						auto& map = uiManager->GetUIObjects();
						auto itScene = map.find(sceneName);
						if (itScene == map.end())
							return;

						const std::string& name = beforeSnapshot.value("name", "");
						auto itObj = itScene->second.find(name);
						if (itObj != itScene->second.end() && itObj->second)
						{
							itObj->second->Deserialize(beforeSnapshot);
							itObj->second->UpdateInteractableFlags();
						}
					},
					[this, uiManager, sceneName, afterSnapshot]()
					{
						if (!uiManager)
							return;

						auto& map = uiManager->GetUIObjects();
						auto itScene = map.find(sceneName);
						if (itScene == map.end())
							return;

						const std::string& name = afterSnapshot.value("name", "");
						auto itObj = itScene->second.find(name);
						if (itObj != itScene->second.end() && itObj->second)
						{
							itObj->second->Deserialize(afterSnapshot);
							itObj->second->UpdateInteractableFlags();
						}
					}
					});
			};

		auto captureUISnapshots = [&](const std::unordered_set<std::string>& names,
			std::unordered_map<std::string, nlohmann::json>& outSnapshots)
			{
				outSnapshots.clear();
				if (it == uiObjectsByScene.end())
				{
					return;
				}
				for (const auto& name : names)
				{
					auto itObj = it->second.find(name);
					if (itObj != it->second.end() && itObj->second)
					{
						nlohmann::json snapshot;
						itObj->second->Serialize(snapshot);
						outSnapshots.emplace(name, std::move(snapshot));
					}
				}
			};

		auto applyUISnapshots = [&](const std::unordered_map<std::string, nlohmann::json>& snapshots)
			{
				if (!uiManager)
				{
					return;
				}
				auto& map = uiManager->GetUIObjects();
				auto itScene = map.find(sceneName);
				if (itScene == map.end())
				{
					return;
				}
				for (const auto& [name, snapshot] : snapshots)
				{
					auto itObj = itScene->second.find(name);
					if (itObj != itScene->second.end() && itObj->second)
					{
						itObj->second->Deserialize(snapshot);
						itObj->second->UpdateInteractableFlags();
					}
				}
			};

		auto pushUIGroupSnapshotUndo = [&](const std::string& label,
			const std::unordered_map<std::string, nlohmann::json>& beforeSnapshots,
			const std::unordered_map<std::string, nlohmann::json>& afterSnapshots)
			{
				m_UndoManager.Push(UndoManager::Command{
					label,
					[applyUISnapshots, beforeSnapshots]()
					{
						applyUISnapshots(beforeSnapshots);
					},
					[applyUISnapshots, afterSnapshots]()
					{
						applyUISnapshots(afterSnapshots);
					}
					});
			};

		auto reparentUIObject = [&](const std::string& childName, const std::string& newParentName)
			{
				if (!uiManager)
				{
					return;
				}

				auto child = getUIObjectByName(childName);
				if (!child)
				{
					return;
				}

				const std::string oldParentName = child->GetParentName();
				if (oldParentName == newParentName)
				{
					return;
				}

				std::unordered_set<std::string> snapshotTargets{ childName };
				if (!oldParentName.empty())
				{
					snapshotTargets.insert(oldParentName);
				}
				if (!newParentName.empty())
				{
					snapshotTargets.insert(newParentName);
				}

				std::unordered_map<std::string, nlohmann::json> beforeSnapshots;
				captureUISnapshots(snapshotTargets, beforeSnapshots);

				if (!oldParentName.empty())
				{
					auto oldParent = getUIObjectByName(oldParentName);
					if (oldParent)
					{
						if (oldParent->GetComponent<HorizontalBox>())
						{
							uiManager->RemoveHorizontalSlot(sceneName, oldParentName, childName);
						}
						else if (oldParent->GetComponent<Canvas>())
						{
							uiManager->RemoveCanvasSlot(sceneName, oldParentName, childName);
						}
					}
				}

				if (newParentName.empty())
				{
					child->ClearParentName();
				}
				else
				{
					auto newParent = getUIObjectByName(newParentName);
					if (newParent)
					{
						if (newParent->GetComponent<HorizontalBox>())
						{
							const auto bounds = child->GetBounds();
							HorizontalBoxSlot slot;
							slot.child = child.get();
							slot.desiredSize = UISize{ bounds.width, bounds.height };
							slot.padding = UIPadding{};
							slot.fillWeight = 1.0f;
							slot.alignment = UIHorizontalAlignment::Fill;
							uiManager->RegisterHorizontalSlot(sceneName, newParentName, childName, slot);
						}
						else if (newParent->GetComponent<Canvas>())
						{
							CanvasSlot slot;
							slot.child = child.get();
							slot.rect = child->GetBounds();
							uiManager->RegisterCanvasSlot(sceneName, newParentName, childName, slot);
						}
						else
						{
							child->SetParentName(newParentName);
						}
					}
				}

				uiManager->RefreshUIListForCurrentScene();

				std::unordered_map<std::string, nlohmann::json> afterSnapshots;
				captureUISnapshots(snapshotTargets, afterSnapshots);
				pushUIGroupSnapshotUndo("Change UI Parent", beforeSnapshots, afterSnapshots);
			};

		auto buildUIChildIndex = [&](const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
			{
				std::unordered_map<std::string, std::vector<std::string>> children;
				for (const auto& [name, uiObject] : uiMap)
				{
					if (!uiObject)
					{
						continue;
					}
					const std::string& parentName = uiObject->GetParentName();
					if (!parentName.empty() && uiMap.find(parentName) != uiMap.end())
					{
						children[parentName].push_back(name);
					}
				}

				for (auto& [parentName, childList] : children)
				{
					std::sort(childList.begin(), childList.end());
				}
				return children;
			};

		static std::unordered_map<std::string, std::unordered_map<std::string, UIRect>> lastUIBoundsByScene;
		if (m_EditorState != EditorPlayState::Play && it != uiObjectsByScene.end())
		{
			auto& lastBounds = lastUIBoundsByScene[sceneName];
			const auto children = buildUIChildIndex(it->second);
			std::unordered_map<std::string, UIRect> movedDeltas;
			std::unordered_set<std::string> currentNames;

			for (const auto& [name, uiObject] : it->second)
			{
				if (!uiObject || !uiObject->HasBounds())
				{
					continue;
				}

				const UIRect bounds = uiObject->GetBounds();
				currentNames.insert(name);
				auto itLast = lastBounds.find(name);
				if (itLast != lastBounds.end())
				{
					if (bounds.x != itLast->second.x || bounds.y != itLast->second.y)
					{
						UIRect delta{};
						delta.x = bounds.x - itLast->second.x;
						delta.y = bounds.y - itLast->second.y;
						movedDeltas.emplace(name, delta);
					}
				}
			}

			std::unordered_set<std::string> movedRoots;
			movedRoots.reserve(movedDeltas.size());
			for (const auto& [name, delta] : movedDeltas)
			{
				auto itNode = it->second.find(name);
				if (itNode == it->second.end() || !itNode->second)
				{
					continue;
				}
				std::string parentName = itNode->second->GetParentName();
				bool hasMovedAncestor = false;
				while (!parentName.empty())
				{
					if (movedDeltas.find(parentName) != movedDeltas.end())
					{
						hasMovedAncestor = true;
						break;
					}
					auto itParent = it->second.find(parentName);
					if (itParent == it->second.end() || !itParent->second)
					{
						break;
					}
					parentName = itParent->second->GetParentName();
				}
				if (!hasMovedAncestor)
				{
					movedRoots.insert(name);
				}
			}

			auto applyDeltaToDescendants = [&](const std::string& rootName, const UIRect& delta, auto&& applyRef) -> void
				{
					auto itChildren = children.find(rootName);
					if (itChildren == children.end())
					{
						return;
					}
					for (const auto& childName : itChildren->second)
					{
						auto itChild = it->second.find(childName);
						if (itChild != it->second.end() && itChild->second && itChild->second->HasBounds())
						{
							UIRect childBounds = itChild->second->GetBounds();
							childBounds.x += delta.x;
							childBounds.y += delta.y;
							itChild->second->SetBounds(childBounds);
						}
						applyRef(childName, delta, applyRef);
					}
				};

			for (const auto& rootName : movedRoots)
			{
				auto itDelta = movedDeltas.find(rootName);
				if (itDelta != movedDeltas.end())
				{
					applyDeltaToDescendants(rootName, itDelta->second, applyDeltaToDescendants);
				}
			}

			for (auto& [name, uiObject] : it->second)
			{
				if (!uiObject || !uiObject->HasBounds())
				{
					continue;
				}
				lastBounds[name] = uiObject->GetBounds();
			}

			if (currentNames.size() != lastBounds.size())
			{
				std::vector<std::string> stale;
				stale.reserve(lastBounds.size());
				for (const auto& [name, bounds] : lastBounds)
				{
					if (currentNames.find(name) == currentNames.end())
					{
						stale.push_back(name);
					}
				}
				for (const auto& name : stale)
				{
					lastBounds.erase(name);
				}
			}
		}

		auto hasSelectedUIAncestor = [&](const std::string& name) -> bool
			{
				auto itNode = it->second.find(name);
				if (itNode == it->second.end() || !itNode->second)
				{
					return false;
				}
				std::string parentName = itNode->second->GetParentName();
				while (!parentName.empty())
				{
					if (m_SelectedUIObjectNames.find(parentName) != m_SelectedUIObjectNames.end())
					{
						return true;
					}
					auto itParent = it->second.find(parentName);
					if (itParent == it->second.end() || !itParent->second)
					{
						break;
					}
					parentName = itParent->second->GetParentName();
				}
				return false;
			};

		auto copySelectedUIObjects = [&]()
			{
				if (it == uiObjectsByScene.end() || m_SelectedUIObjectNames.empty())
				{
					return;
				}

				auto children = buildUIChildIndex(it->second);
				std::vector<std::string> roots;
				roots.reserve(m_SelectedUIObjectNames.size());
				for (const auto& name : m_SelectedUIObjectNames)
				{
					if (!hasSelectedUIAncestor(name))
					{
						roots.push_back(name);
					}
				}

				nlohmann::json clipboard = nlohmann::json::object();
				clipboard["objects"] = nlohmann::json::array();

				std::vector<std::string> stack = roots;
				while (!stack.empty())
				{
					std::string current = std::move(stack.back());
					stack.pop_back();

					auto itNode = it->second.find(current);
					if (itNode == it->second.end() || !itNode->second)
					{
						continue;
					}

					nlohmann::json snapshot;
					itNode->second->Serialize(snapshot);
					clipboard["objects"].push_back(std::move(snapshot));

					auto childIt = children.find(current);
					if (childIt != children.end())
					{
						for (const auto& childName : childIt->second)
						{
							stack.push_back(childName);
						}
					}
				}

				m_UIObjectClipboard = std::move(clipboard);
				m_UIObjectClipboardHasData = true;
			};

		auto pasteUIClipboardObjects = [&]()
			{
				if (!m_UIObjectClipboardHasData || !m_UIObjectClipboard.is_object())
				{
					m_UIObjectClipboardHasData = false;
					return false;
				}
				if (!m_UIObjectClipboard.contains("objects") || !m_UIObjectClipboard["objects"].is_array())
				{
					return false;
				}

				auto& uiMap = uiManager->GetUIObjects();
				auto itScene = uiMap.find(sceneName);
				if (itScene == uiMap.end())
				{
					return false;
				}

				std::unordered_map<std::string, std::string> nameRemap;
				std::unordered_set<std::string> reservedNames;
				std::unordered_set<std::string> clipboardNames;
				std::vector<std::string> clipboardRootNames;
				for (const auto& [name, uiObject] : itScene->second)
				{
					reservedNames.insert(name);
				}

				auto makeUniqueName = [&](const std::string& baseName)
					{
						std::string name = baseName;
						int suffix = 1;
						while (reservedNames.find(name) != reservedNames.end())
						{
							name = baseName + "_" + std::to_string(suffix++);
						}
						reservedNames.insert(name);
						return name;
					};

				for (const auto& entry : m_UIObjectClipboard["objects"])
				{
					const std::string originalName = entry.value("name", "");
					if (originalName.empty())
					{
						continue;
					}
					clipboardNames.insert(originalName);
					std::string baseName = originalName;
					if (reservedNames.find(baseName) != reservedNames.end())
					{
						baseName = originalName + "_Dex";
					}
					nameRemap[originalName] = makeUniqueName(baseName);
				}

				clipboardRootNames.reserve(clipboardNames.size());
				for (const auto& entry : m_UIObjectClipboard["objects"])
				{
					const std::string originalName = entry.value("name", "");
					if (originalName.empty())
					{
						continue;
					}
					const std::string parentName = entry.value("parent", "");
					if (parentName.empty() || clipboardNames.find(parentName) == clipboardNames.end())
					{
						clipboardRootNames.push_back(originalName);
					}
				}

				std::vector<std::shared_ptr<UIObject>> createdObjects;
				createdObjects.reserve(nameRemap.size());

				auto updateSlotChildNames = [&](nlohmann::json& data)
					{
						if (!data.contains("Slots") || !data["Slots"].is_array())
						{
							return;
						}
						for (auto& slot : data["Slots"])
						{
							if (!slot.contains("child"))
							{
								continue;
							}
							const std::string childName = slot.value("child", "");
							if (childName.empty())
							{
								continue;
							}
							auto itChild = nameRemap.find(childName);
							if (itChild != nameRemap.end())
							{
								slot["child"] = itChild->second;
							}
							else
							{
								slot["child"] = "";
							}
						}
					};

				for (const auto& entry : m_UIObjectClipboard["objects"])
				{
					if (!entry.is_object())
					{
						continue;
					}

					const std::string originalName = entry.value("name", "");
					if (originalName.empty())
					{
						continue;
					}

					nlohmann::json objectJson = entry;
					objectJson["name"] = nameRemap[originalName];

					const std::string parentName = entry.value("parent", "");
					auto itParent = nameRemap.find(parentName);
					if (itParent != nameRemap.end())
					{
						objectJson["parent"] = itParent->second;
					}
					else if (!parentName.empty() && itScene->second.find(parentName) != itScene->second.end())
					{
						objectJson["parent"] = parentName;
					}
					else
					{
						objectJson["parent"] = "";
					}

					if (objectJson.contains("components") && objectJson["components"].is_array())
					{
						for (auto& componentJson : objectJson["components"])
						{
							const std::string typeName = componentJson.value("type", "");
							if (typeName == "HorizontalBox" || typeName == "Canvas")
							{
								if (componentJson.contains("data") && componentJson["data"].is_object())
								{
									updateSlotChildNames(componentJson["data"]);
								}
							}
						}
					}

					auto uiObject = std::make_shared<UIObject>(scene->GetEventDispatcher());
					uiObject->Deserialize(objectJson);
					uiObject->SetScene(scene.get());
					uiObject->UpdateInteractableFlags();
					uiManager->AddUI(sceneName, uiObject);
					createdObjects.push_back(uiObject);
				}

				for (const auto& uiObject : createdObjects)
				{
					if (!uiObject)
					{
						continue;
					}

					const std::string& name = uiObject->GetName();
					if (auto* horizontal = uiObject->GetComponent<HorizontalBox>())
					{
						for (auto& slot : horizontal->GetSlotsRef())
						{
							if (!slot.child && !slot.childName.empty())
							{
								auto itChild = itScene->second.find(slot.childName);
								if (itChild != itScene->second.end())
								{
									slot.child = itChild->second.get();
								}
							}
							if (slot.child)
							{
								slot.child->SetParentName(name);
							}
						}
						uiManager->ApplyHorizontalLayout(sceneName, name);
					}

					if (auto* canvas = uiObject->GetComponent<Canvas>())
					{
						for (auto& slot : canvas->GetSlotsRef())
						{
							if (!slot.child && !slot.childName.empty())
							{
								auto itChild = itScene->second.find(slot.childName);
								if (itChild != itScene->second.end())
								{
									slot.child = itChild->second.get();
								}
							}
							if (slot.child)
							{
								slot.child->SetParentName(name);
							}
						}
						uiManager->ApplyCanvasLayout(sceneName, name);
					}
				}

				uiManager->RefreshUIListForCurrentScene();
				m_SelectedUIObjectNames.clear();
				m_SelectedUIObjectName.clear();
				std::string focusName;

				for (const auto& uiObject : createdObjects)
				{
					if (uiObject)
					{
						m_SelectedUIObjectNames.insert(uiObject->GetName());
					}
				}
				for (const auto& rootName : clipboardRootNames)
				{
					auto itName = nameRemap.find(rootName);
					if (itName == nameRemap.end())
					{
						continue;
					}
					focusName = itName->second;
				}

				if (!focusName.empty())
				{
					m_SelectedUIObjectName = focusName;
				}
				if (!createdObjects.empty())
				{
					m_UndoManager.Push(UndoManager::Command{
						"Paste UI Objects",
						[this, uiManager, sceneName, createdObjects]()
						{
							if (!uiManager)
								return;
							for (const auto& uiObject : createdObjects)
							{
								if (uiObject)
								{
									uiManager->RemoveUI(sceneName, uiObject);
								}
							}
							uiManager->RefreshUIListForCurrentScene();
						},
						[this, uiManager, sceneName, createdObjects]()
						{
							if (!uiManager)
								return;
							for (const auto& uiObject : createdObjects)
							{
								if (uiObject)
								{
									uiManager->AddUI(sceneName, uiObject);
								}
							}
							uiManager->RefreshUIListForCurrentScene();
						}
						});
				}

				return true;
			};

		if (!m_SelectedUIObjectName.empty() && !getUIObjectByName(m_SelectedUIObjectName))
		{
			m_SelectedUIObjectName.clear();
		}

		if (ImGui::Button("Add UI Object"))
		{
			std::string baseName = "UIObject";
			std::string name = baseName;
			int suffix = 1;
			if (it != uiObjectsByScene.end())
			{
				while (it->second.find(name) != it->second.end())
				{
					name = baseName + std::to_string(suffix++);
				}
			}

			auto uiObject = std::make_shared<UIObject>(scene->GetEventDispatcher());
			uiObject->SetName(name);
			uiObject->SetBounds(UIRect{ 20.0f, 20.0f, 200.0f, 80.0f });
			uiObject->SetAnchorMin(UIAnchor{ 0.0f, 0.0f });
			uiObject->SetAnchorMax(UIAnchor{ 0.0f, 0.0f });
			uiObject->SetPivot(UIAnchor{ 0.0f, 0.0f });
			uiObject->SetScene(scene.get());
			uiObject->UpdateInteractableFlags();
			uiManager->AddUI(sceneName, uiObject);
			uiManager->RefreshUIListForCurrentScene();
			m_SelectedUIObjectName = name;
			m_SelectedUIObjectNames.clear();
			m_SelectedUIObjectNames.insert(name);

			auto uiObjectRef = uiObject;
			m_UndoManager.Push(UndoManager::Command{
				"Create UI Object",
				[this, uiManager, sceneName, uiObjectRef]()
				{
					if (!uiManager || !uiObjectRef)
						return;

					uiManager->RemoveUI(sceneName, uiObjectRef);
					uiManager->RefreshUIListForCurrentScene();
					m_SelectedUIObjectNames.erase(uiObjectRef->GetName());
					if (m_SelectedUIObjectName == uiObjectRef->GetName())
					{
						m_SelectedUIObjectName.clear();
					}
				},
				[this, uiManager, sceneName, uiObjectRef]()
				{
					if (!uiManager || !uiObjectRef)
						return;

					uiManager->AddUI(sceneName, uiObjectRef);
					uiManager->RefreshUIListForCurrentScene();
					m_SelectedUIObjectName = uiObjectRef->GetName();
					m_SelectedUIObjectNames.clear();
					m_SelectedUIObjectNames.insert(uiObjectRef->GetName());
				}
				});
		}
		
		if (ImGui::Button("Copy"))
		{
			copySelectedUIObjects();
		}
		ImGui::SameLine();
		if (!m_UIObjectClipboardHasData)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Paste"))
		{
			pasteUIClipboardObjects();
		}
		if (!m_UIObjectClipboardHasData)
		{
			ImGui::EndDisabled();
		}
		ImGui::SameLine();

		if (ImGui::Button("Remove Selected"))
		{
			if (it != uiObjectsByScene.end())
			{
				auto collectRemovalNames = [&](const std::string& rootName,
					const std::unordered_map<std::string, std::vector<std::string>>& childrenIndex,
					std::unordered_set<std::string>& outNames)
					{
						std::vector<std::string> stack;
						stack.push_back(rootName);
						while (!stack.empty())
						{
							std::string current = std::move(stack.back());
							stack.pop_back();
							if (!outNames.insert(current).second)
							{
								continue;
							}
							auto childIt = childrenIndex.find(current);
							if (childIt == childrenIndex.end())
							{
								continue;
							}
							for (const auto& childName : childIt->second)
							{
								stack.push_back(childName);
							}
						}
					};

				std::unordered_set<std::string> removalNames;
				auto childrenIndex = buildUIChildIndex(it->second);
				if (!m_SelectedUIObjectNames.empty())
				{
					for (const auto& name : m_SelectedUIObjectNames)
					{
						collectRemovalNames(name, childrenIndex, removalNames);
					}
				}
				else if (!m_SelectedUIObjectName.empty())
				{
					collectRemovalNames(m_SelectedUIObjectName, childrenIndex, removalNames);
				}

				if (!removalNames.empty())
				{
					std::vector<std::shared_ptr<UIObject>> removedObjects;
					removedObjects.reserve(removalNames.size());
					for (const auto& name : removalNames)
					{
						auto itObj = it->second.find(name);
						if (itObj != it->second.end() && itObj->second)
						{
							removedObjects.push_back(itObj->second);
						}
					}

					for (const auto& removedObject : removedObjects)
					{
						uiManager->RemoveUI(sceneName, removedObject);
					}
					uiManager->RefreshUIListForCurrentScene();

					m_SelectedUIObjectNames.clear();
					m_SelectedUIObjectName.clear();

					m_UndoManager.Push(UndoManager::Command{
						"Remove UI Objects",
						[this, uiManager, sceneName, removedObjects]()
						{
							if (!uiManager)
								return;
							for (const auto& removedObject : removedObjects)
							{
								if (removedObject)
								{
									uiManager->AddUI(sceneName, removedObject);
								}
							}
							uiManager->RefreshUIListForCurrentScene();
						},
						[this, uiManager, sceneName, removedObjects]()
						{
							if (!uiManager)
								return;
							for (const auto& removedObject : removedObjects)
							{
								if (removedObject)
								{
									uiManager->RemoveUI(sceneName, removedObject);
								}
							}
							uiManager->RefreshUIListForCurrentScene();
						}
						});
				}
			}
		}

		ImGui::Separator();

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
		{
			ImGuiIO& io = ImGui::GetIO();
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C))
			{
				copySelectedUIObjects();
			}
			if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))
			{
				pasteUIClipboardObjects();
			}
		}

		if (ImGui::BeginPopupContextWindow("UIHierarchyContext", ImGuiPopupFlags_MouseButtonRight | ImGuiPopupFlags_NoOpenOverItems))
		{
			const bool hasSelection = !m_SelectedUIObjectNames.empty();
			if (!hasSelection)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::MenuItem("Copy"))
			{
				copySelectedUIObjects();
			}
			if (!hasSelection)
			{
				ImGui::EndDisabled();
			}
			if (!m_UIObjectClipboardHasData)
			{
				ImGui::BeginDisabled();
			}
			if (ImGui::MenuItem("Paste"))
			{
				pasteUIClipboardObjects();
			}
			if (!m_UIObjectClipboardHasData)
			{
				ImGui::EndDisabled();
			}
			ImGui::EndPopup();
		}


		if (it == uiObjectsByScene.end() || it->second.empty())
		{
			ImGui::TextDisabled("No UI Objects in Scene.");
		}
		else
		{
			auto buildChildren = [&](const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap)
				{
					std::unordered_map<std::string, std::vector<std::string>> children;
					std::vector<std::string> roots;

					for (const auto& [name, uiObject] : uiMap)
					{
						if (!uiObject)
						{
							continue;
						}

						const std::string& parentName = uiObject->GetParentName();
						if (parentName.empty() || uiMap.find(parentName) == uiMap.end())
						{
							roots.push_back(name);
						}
						else
						{
							children[parentName].push_back(name);
						}
					}

					for (auto& [parentName, childList] : children)
					{
						std::sort(childList.begin(), childList.end());
					}
					std::sort(roots.begin(), roots.end());

					return std::make_pair(roots, children);
				};

			auto isDescendant = [&](const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
				const std::string& candidate, const std::string& possibleAncestor) -> bool
				{
					std::unordered_set<std::string> visited;
					std::string current = candidate;
					while (!current.empty())
					{
						if (current == possibleAncestor)
						{
							return true;
						}
						if (!visited.insert(current).second)
						{
							break;
						}
						auto itNode = uiMap.find(current);
						if (itNode == uiMap.end() || !itNode->second)
						{
							break;
						}
						current = itNode->second->GetParentName();
					}
					return false;
				};

			auto [roots, children] = buildChildren(it->second);
			const std::string payloadType = "UIOBJECT_NAME";

			std::function<void(const std::string&)> drawNode = [&](const std::string& name)
				{
					auto itNode = it->second.find(name);
					if (itNode == it->second.end() || !itNode->second)
					{
						return;
					}

					const bool selected = (m_SelectedUIObjectNames.find(name) != m_SelectedUIObjectNames.end());
					ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_OpenOnArrow;
					if (children.find(name) == children.end())
					{
						flags |= ImGuiTreeNodeFlags_Leaf;
					}
					if (selected)
					{
						flags |= ImGuiTreeNodeFlags_Selected;
					}

					bool opened = ImGui::TreeNodeEx(name.c_str(), flags);
					if (ImGui::IsItemClicked())
					{
						const bool append = ImGui::GetIO().KeyShift;
						if (!append)
						{
							m_SelectedUIObjectNames.clear();
						}
						if (!m_SelectedUIObjectNames.insert(name).second && append)
						{
							m_SelectedUIObjectNames.erase(name);
						}
						m_SelectedUIObjectName = name;
					}

					if (ImGui::BeginDragDropSource())
					{
						ImGui::SetDragDropPayload(payloadType.c_str(), name.c_str(), name.size() + 1);
						ImGui::Text("Move: %s", name.c_str());
						ImGui::EndDragDropSource();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(payloadType.c_str()))
						{
							const char* droppedName = static_cast<const char*>(payload->Data);
							if (droppedName && name != droppedName)
							{
								auto itDrop = it->second.find(droppedName);
								if (itDrop != it->second.end() && itDrop->second)
								{
									if (!isDescendant(it->second, name, droppedName))
									{
										reparentUIObject(droppedName, name);
									}
								}
							}
						}
						ImGui::EndDragDropTarget();
					}

					if (opened)
					{
						auto childIt = children.find(name);
						if (childIt != children.end())
						{
							for (const auto& childName : childIt->second)
							{
								drawNode(childName);
							}
						}
						ImGui::TreePop();
					}
				};

			ImGui::SeparatorText("Hierarchy");
			if (!roots.empty())
			{
				for (const auto& root : roots)
				{
					drawNode(root);
				}
			}
			else
			{
				for (const auto& [name, uiObject] : it->second)
				{
					if (uiObject)
					{
						drawNode(name);
					}
				}
			}
		}

		auto isUIComponentType = [](const std::string& typeName) -> bool
			{
				return IsUIComponentType(typeName);
			};

		auto selectedObject = getUIObjectByName(m_SelectedUIObjectName);
		if (selectedObject)
		{
			if (m_LastSelectedUIObjectName != selectedObject->GetName())
			{
				m_PendingUIPropertySnapshots.clear();
				m_LastSelectedUIObjectName = selectedObject->GetName();
				std::snprintf(m_UIObjectNameBuffer.data(), m_UIObjectNameBuffer.size(), "%s", selectedObject->GetName().c_str());
			}

			auto recordUILongEdit = [&](size_t key, bool updated, const std::string& label)
				{
					if (ImGui::IsItemActivated() && m_PendingUIPropertySnapshots.find(key) == m_PendingUIPropertySnapshots.end())
					{
						PendingUIPropertySnapshot pendingSnapshot;
						selectedObject->Serialize(pendingSnapshot.beforeSnapshot);
						m_PendingUIPropertySnapshots.emplace(key, std::move(pendingSnapshot));
					}

					if (updated)
					{
						auto itSnapshot = m_PendingUIPropertySnapshots.find(key);
						if (itSnapshot != m_PendingUIPropertySnapshots.end())
						{
							itSnapshot->second.updated = true;
						}
					}

					if (ImGui::IsItemDeactivatedAfterEdit())
					{
						auto itSnapshot = m_PendingUIPropertySnapshots.find(key);
						if (itSnapshot != m_PendingUIPropertySnapshots.end())
						{
							if (itSnapshot->second.updated)
							{
								nlohmann::json afterSnapshot;
								selectedObject->Serialize(afterSnapshot);
								pushUISnapshotUndo(label, itSnapshot->second.beforeSnapshot, afterSnapshot);
							}
							m_PendingUIPropertySnapshots.erase(itSnapshot);
						}
					}
				};

			auto makeUILayoutKey = [&](const std::string& propertyName)
				{
					const size_t pointerHash = std::hash<const void*>{}(selectedObject.get());
					const size_t nameHash = std::hash<std::string>{}(propertyName);
					return pointerHash ^ (nameHash + 0x9e3779b97f4a7c15ULL + (pointerHash << 6) + (pointerHash >> 2));
				};

			struct GroupEditSnapshot
			{
				std::unordered_map<std::string, nlohmann::json> beforeSnapshots;
				std::unordered_map<std::string, UIRect> startWorldBounds;
				UIRect startBounds{};
				bool updated = false;
			};
			static std::unordered_map<size_t, GroupEditSnapshot> pendingGroupEdits;
			static size_t lastGroupSelectionHash = 0;

			auto getWorldBoundsForLayout = [&](const std::string& name,
				const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
				auto&& getWorldBoundsRef,
				std::unordered_map<std::string, UIRect>& cache,
				std::unordered_set<std::string>& visiting) -> UIRect
				{
					auto cached = cache.find(name);
					if (cached != cache.end())
					{
						return cached->second;
					}

					auto itObj = uiMap.find(name);
					if (itObj == uiMap.end() || !itObj->second)
					{
						return UIRect{};
					}

					if (!visiting.insert(name).second)
					{
						return itObj->second->GetBounds();
					}

					const auto& uiObject = itObj->second;
					UIRect local = uiObject->GetBounds();
					const std::string& parentName = uiObject->GetParentName();
					if (parentName.empty() || uiMap.find(parentName) == uiMap.end())
					{
						cache[name] = local;
						visiting.erase(name);
						return local;
					}

					UIRect parentBounds = getWorldBoundsRef(parentName, uiMap, getWorldBoundsRef, cache, visiting);
					const UIAnchor anchorMin = uiObject->GetAnchorMin();
					const UIAnchor anchorMax = uiObject->GetAnchorMax();
					const UIAnchor pivot = uiObject->GetPivot();

					const float anchorLeft = parentBounds.x + parentBounds.width * anchorMin.x;
					const float anchorTop = parentBounds.y + parentBounds.height * anchorMin.y;
					const float anchorRight = parentBounds.x + parentBounds.width * anchorMax.x;
					const float anchorBottom = parentBounds.y + parentBounds.height * anchorMax.y;

					const bool stretchX = anchorMin.x != anchorMax.x;
					const bool stretchY = anchorMin.y != anchorMax.y;
					const float baseWidth = stretchX ? (anchorRight - anchorLeft) : 0.0f;
					const float baseHeight = stretchY ? (anchorBottom - anchorTop) : 0.0f;

					const float width = stretchX ? (baseWidth + local.width) : local.width;
					const float height = stretchY ? (baseHeight + local.height) : local.height;

					UIRect world;
					world.width = width;
					world.height = height;
					world.x = anchorLeft + local.x - width * pivot.x;
					world.y = anchorTop + local.y - height * pivot.y;

					cache[name] = world;
					visiting.erase(name);
					return world;
				};

			auto setLocalFromWorldForLayout = [&](UIObject& uiObject, const UIRect& worldBounds, const UIRect& parentBounds)
				{
					const UIAnchor anchorMin = uiObject.GetAnchorMin();
					const UIAnchor anchorMax = uiObject.GetAnchorMax();
					const UIAnchor pivot = uiObject.GetPivot();

					const float anchorLeft = parentBounds.x + parentBounds.width * anchorMin.x;
					const float anchorTop = parentBounds.y + parentBounds.height * anchorMin.y;
					const float anchorRight = parentBounds.x + parentBounds.width * anchorMax.x;
					const float anchorBottom = parentBounds.y + parentBounds.height * anchorMax.y;

					const bool stretchX = anchorMin.x != anchorMax.x;
					const bool stretchY = anchorMin.y != anchorMax.y;
					const float baseWidth = stretchX ? (anchorRight - anchorLeft) : 0.0f;
					const float baseHeight = stretchY ? (anchorBottom - anchorTop) : 0.0f;

					UIRect local = uiObject.GetBounds();
					local.width = stretchX ? (worldBounds.width - baseWidth) : worldBounds.width;
					local.height = stretchY ? (worldBounds.height - baseHeight) : worldBounds.height;
					local.x = worldBounds.x - anchorLeft + worldBounds.width * pivot.x;
					local.y = worldBounds.y - anchorTop + worldBounds.height * pivot.y;
					uiObject.SetBounds(local);
				};

			auto applyUIObjectRename = [&](const std::string& oldName, const std::string& newName) -> bool
				{
					if (!uiManager)
					{
						return false;
					}

					if (!uiManager->RenameUIObject(sceneName, oldName, newName))
					{
						return false;
					}

					if (m_SelectedUIObjectName == oldName)
					{
						m_SelectedUIObjectName = newName;
					}

					if (auto itName = m_SelectedUIObjectNames.find(oldName); itName != m_SelectedUIObjectNames.end())
					{
						m_SelectedUIObjectNames.erase(itName);
						m_SelectedUIObjectNames.insert(newName);
					}

					if (m_LastSelectedUIObjectName == oldName)
					{
						m_LastSelectedUIObjectName = newName;
					}

					if (auto itButton = m_UIButtonBindingTargets.find(oldName); itButton != m_UIButtonBindingTargets.end())
					{
						m_UIButtonBindingTargets[newName] = itButton->second;
						m_UIButtonBindingTargets.erase(itButton);
					}
					for (auto& [key, value] : m_UIButtonBindingTargets)
					{
						if (value == oldName)
						{
							value = newName;
						}
					}

					if (auto itSlider = m_UISliderBindingTargets.find(oldName); itSlider != m_UISliderBindingTargets.end())
					{
						m_UISliderBindingTargets[newName] = itSlider->second;
						m_UISliderBindingTargets.erase(itSlider);
					}
					for (auto& [key, value] : m_UISliderBindingTargets)
					{
						if (value == oldName)
						{
							value = newName;
						}
					}

					uiManager->RefreshUIListForCurrentScene();
					std::snprintf(m_UIObjectNameBuffer.data(), m_UIObjectNameBuffer.size(), "%s", newName.c_str());
					return true;
				};

			ImGui::Text("Name");
			ImGui::SameLine();
			ImGui::SetNextItemWidth(-1);
			ImGui::InputText("##UIObjectName", m_UIObjectNameBuffer.data(), m_UIObjectNameBuffer.size());
			if (ImGui::IsItemDeactivatedAfterEdit())
			{
				const std::string oldName = selectedObject->GetName();
				const std::string newName = m_UIObjectNameBuffer.data();
				if (!newName.empty() && newName != oldName)
				{
					if (applyUIObjectRename(oldName, newName))
					{
						m_UndoManager.Push(UndoManager::Command{
							"Rename UI Object",
							[this, applyUIObjectRename, oldName, newName]()
							{
								applyUIObjectRename(newName, oldName);
							},
							[this, applyUIObjectRename, oldName, newName]()
							{
								applyUIObjectRename(oldName, newName);
							}
							});
					}
					else
					{
						std::snprintf(m_UIObjectNameBuffer.data(), m_UIObjectNameBuffer.size(), "%s", oldName.c_str());
					}
				}
				else
				{
					std::snprintf(m_UIObjectNameBuffer.data(), m_UIObjectNameBuffer.size(), "%s", oldName.c_str());
				}
			}

			ImGui::SeparatorText("Layout");
			ImGui::Text("Parent");
			ImGui::SameLine();
			if (ImGui::BeginCombo("##ParentName", selectedObject->GetParentName().empty() ? "<None>" : selectedObject->GetParentName().c_str()))
			{
				if (ImGui::Selectable("<None>", selectedObject->GetParentName().empty()))
				{
					reparentUIObject(selectedObject->GetName(), "");
				}
				for (const auto& [name, uiObject] : it->second)
				{
					if (!uiObject || name == selectedObject->GetName())
					{
						continue;
					}
					const bool isSelected = (selectedObject->GetParentName() == name);
					if (ImGui::Selectable(name.c_str(), isSelected))
					{
						reparentUIObject(selectedObject->GetName(), name);
					}
					if (isSelected)
					{
						ImGui::SetItemDefaultFocus();
					}
				}
				ImGui::EndCombo();
			}

			auto clampAnchor = [](UIAnchor anchor)
				{
					anchor.x = std::clamp(anchor.x, 0.0f, 1.0f);
					anchor.y = std::clamp(anchor.y, 0.0f, 1.0f);
					return anchor;
				};

			UIAnchor anchorMin = selectedObject->GetAnchorMin();
			UIAnchor anchorMax = selectedObject->GetAnchorMax();
			UIAnchor pivot = selectedObject->GetPivot();
			float rotation = selectedObject->GetRotationDegrees();
			UIRect bounds = selectedObject->GetBounds();
			float anchorMinValues[2] = { anchorMin.x, anchorMin.y };
			float anchorMaxValues[2] = { anchorMax.x, anchorMax.y };
			float pivotValues[2] = { pivot.x, pivot.y };
			float positionValues[2] = { bounds.x, bounds.y };
			float sizeValues[2] = { bounds.width, bounds.height };

			auto captureWorldBounds = [&]() -> UIRect
				{
					if (it != uiObjectsByScene.end())
					{
						std::unordered_map<std::string, UIRect> boundsCache;
						std::unordered_set<std::string> visiting;
						return getWorldBoundsForLayout(selectedObject->GetName(), it->second, getWorldBoundsForLayout, boundsCache, visiting);
					}
					return selectedObject->GetBounds();
				};

			auto captureParentBounds = [&]() -> UIRect
				{
					UIRect parentBounds{};
					const std::string parentName = selectedObject->GetParentName();
					if (!parentName.empty() && it != uiObjectsByScene.end())
					{
						auto itParent = it->second.find(parentName);
						if (itParent != it->second.end())
						{
							std::unordered_map<std::string, UIRect> boundsCache;
							std::unordered_set<std::string> visiting;
							parentBounds = getWorldBoundsForLayout(parentName, it->second, getWorldBoundsForLayout, boundsCache, visiting);
						}
					}
					return parentBounds;
				};


			const bool anchorMinChanged = ImGui::DragFloat2("Anchor Min", anchorMinValues, 0.01f, 0.0f, 1.0f);
			if (anchorMinChanged)
			{
				UIRect worldBounds = captureWorldBounds();
				selectedObject->SetAnchorMin(clampAnchor(UIAnchor{ anchorMinValues[0], anchorMinValues[1] }));
				setLocalFromWorldForLayout(*selectedObject, worldBounds, captureParentBounds());
			}
			recordUILongEdit(makeUILayoutKey("AnchorMin"), anchorMinChanged, "Edit UI Anchor Min");

			const bool anchorMaxChanged = ImGui::DragFloat2("Anchor Max", anchorMaxValues, 0.01f, 0.0f, 1.0f);
			if (anchorMaxChanged)
			{
				UIRect worldBounds = captureWorldBounds();
				selectedObject->SetAnchorMax(clampAnchor(UIAnchor{ anchorMaxValues[0], anchorMaxValues[1] }));
				setLocalFromWorldForLayout(*selectedObject, worldBounds, captureParentBounds());
			}
			recordUILongEdit(makeUILayoutKey("AnchorMax"), anchorMaxChanged, "Edit UI Anchor Max");

			const bool pivotChanged = ImGui::DragFloat2("Pivot", pivotValues, 0.01f, 0.0f, 1.0f);
			if (pivotChanged)
			{
				UIRect worldBounds = captureWorldBounds();
				selectedObject->SetPivot(clampAnchor(UIAnchor{ pivotValues[0], pivotValues[1] }));
				setLocalFromWorldForLayout(*selectedObject, worldBounds, captureParentBounds());
			}
			recordUILongEdit(makeUILayoutKey("Pivot"), pivotChanged, "Edit UI Pivot");

			const bool rotationChanged = ImGui::DragFloat("Rotation", &rotation, 0.5f, -180.0f, 180.0f);
			if (rotationChanged)
			{
				selectedObject->SetRotationDegrees(rotation);
			}
			recordUILongEdit(makeUILayoutKey("Rotation"), rotationChanged, "Edit UI Rotation");

			const bool positionChanged = ImGui::DragFloat2("Position", positionValues, 1.0f, -10000.0f, 10000.0f);
			if (positionChanged)
			{
				const UIRect previousBounds = bounds;
				bounds.x = positionValues[0];
				bounds.y = positionValues[1];
				selectedObject->SetBounds(bounds);

				const float deltaX = bounds.x - previousBounds.x;
				const float deltaY = bounds.y - previousBounds.y;
				if ((deltaX != 0.0f || deltaY != 0.0f) && it != uiObjectsByScene.end())
				{
					const std::string selectedName = selectedObject->GetName();
					for (auto& [name, uiObject] : it->second)
					{
						if (!uiObject || name == selectedName)
						{
							continue;
						}

						std::string parentName = uiObject->GetParentName();
						bool isDescendant = false;
						while (!parentName.empty())
						{
							if (parentName == selectedName)
							{
								isDescendant = true;
								break;
							}
							auto itParent = it->second.find(parentName);
							if (itParent == it->second.end() || !itParent->second)
							{
								break;
							}
							parentName = itParent->second->GetParentName();
						}

						if (!isDescendant)
						{
							continue;
						}

						UIRect childBounds = uiObject->GetBounds();
						childBounds.x += deltaX;
						childBounds.y += deltaY;
						uiObject->SetBounds(childBounds);
					}
				}
			}
			recordUILongEdit(makeUILayoutKey("Position"), positionChanged, "Edit UI Position");

			bool hasHorizontalParent = false;
			const std::string parentName = selectedObject->GetParentName();
			if (!parentName.empty() && it != uiObjectsByScene.end())
			{
				auto itParent = it->second.find(parentName);
				if (itParent != it->second.end() && itParent->second)
				{
					hasHorizontalParent = itParent->second->GetComponent<HorizontalBox>() != nullptr;
				}
			}

			ImGui::BeginDisabled(hasHorizontalParent);

			const bool sizeChanged = ImGui::DragFloat2("Size", sizeValues, 1.0f, -10000.0f, 10000.0f);
			if (sizeChanged)
			{
				bounds.width = sizeValues[0];
				bounds.height = sizeValues[1];
				selectedObject->SetBounds(bounds);
			}
			recordUILongEdit(makeUILayoutKey("Size"), sizeChanged, "Edit UI Size");
			ImGui::EndDisabled();

			size_t selectionHash = 0;
			if (m_SelectedUIObjectNames.size() > 1)
			{
				for (const auto& name : m_SelectedUIObjectNames)
				{
					const size_t nameHash = std::hash<std::string>{}(name);
					selectionHash ^= nameHash + 0x9e3779b97f4a7c15ULL + (selectionHash << 6) + (selectionHash >> 2);
				}
				if (selectionHash != lastGroupSelectionHash)
				{
					pendingGroupEdits.clear();
					lastGroupSelectionHash = selectionHash;
				}
			}
			else
			{
				pendingGroupEdits.clear();
				lastGroupSelectionHash = 0;
			}

			if (m_SelectedUIObjectNames.size() > 1)
			{
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;
				UIRect combined{};
				bool first = true;
				for (const auto& name : m_SelectedUIObjectNames)
				{
					const auto bounds = getWorldBoundsForLayout(name, it->second, getWorldBoundsForLayout, boundsCache, visiting);
					if (first)
					{
						combined = bounds;
						first = false;
					}
					else
					{
						const float minX = min(combined.x, bounds.x);
						const float minY = min(combined.y, bounds.y);
						const float maxX = max(combined.x + combined.width, bounds.x + bounds.width);
						const float maxY = max(combined.y + combined.height, bounds.y + bounds.height);
						combined.x = minX;
						combined.y = minY;
						combined.width = maxX - minX;
						combined.height = maxY - minY;
					}
				}

				float groupPosition[2] = { combined.x, combined.y };
				float groupSize[2] = { combined.width, combined.height };
				const size_t groupPositionKey = makeUILayoutKey("GroupPosition") ^ selectionHash;
				const size_t groupSizeKey = makeUILayoutKey("GroupSize") ^ selectionHash;

				auto collectGroupTargets = [&]() -> std::unordered_set<std::string>
					{
						std::unordered_set<std::string> targets;
						if (it == uiObjectsByScene.end())
						{
							return targets;
						}

						for (const auto& name : m_SelectedUIObjectNames)
						{
							auto itNode = it->second.find(name);
							if (itNode == it->second.end() || !itNode->second)
							{
								continue;
							}
							std::string parentName = itNode->second->GetParentName();
							bool hasSelectedAncestor = false;
							while (!parentName.empty())
							{
								if (m_SelectedUIObjectNames.find(parentName) != m_SelectedUIObjectNames.end())
								{
									hasSelectedAncestor = true;
									break;
								}
								auto itParent = it->second.find(parentName);
								if (itParent == it->second.end() || !itParent->second)
								{
									break;
								}
								parentName = itParent->second->GetParentName();
							}
							if (!hasSelectedAncestor)
							{
								targets.insert(name);
							}
						}

						return targets;
					};

				auto collectGroupMoveTargets = [&](const std::unordered_set<std::string>& roots) -> std::unordered_set<std::string>
					{
						std::unordered_set<std::string> targets;
						if (it == uiObjectsByScene.end() || roots.empty())
						{
							return targets;
						}

						for (const auto& [name, uiObject] : it->second)
						{
							if (!uiObject)
							{
								continue;
							}
							if (roots.find(name) != roots.end())
							{
								targets.insert(name);
								continue;
							}

							std::string parentName = uiObject->GetParentName();
							while (!parentName.empty())
							{
								if (roots.find(parentName) != roots.end())
								{
									targets.insert(name);
									break;
								}
								auto itParent = it->second.find(parentName);
								if (itParent == it->second.end() || !itParent->second)
								{
									break;
								}
								parentName = itParent->second->GetParentName();
							}
						}

						return targets;
					};

				auto ensureGroupSnapshot = [&](size_t key, const UIRect& startBounds, const std::unordered_set<std::string>& targets)
					{
						if (pendingGroupEdits.find(key) != pendingGroupEdits.end())
						{
							return;
						}
						GroupEditSnapshot snapshot;
						captureUISnapshots(targets, snapshot.beforeSnapshots);
						snapshot.startBounds = startBounds;
						snapshot.startWorldBounds.clear();
						if (it != uiObjectsByScene.end())
						{
							for (const auto& name : targets)
							{
								std::unordered_map<std::string, UIRect> localCache;
								std::unordered_set<std::string> localVisiting;
								snapshot.startWorldBounds[name] = getWorldBoundsForLayout(name, it->second, getWorldBoundsForLayout, localCache, localVisiting);
							}
						}
						pendingGroupEdits.emplace(key, std::move(snapshot));
					};

				const auto groupTargets = collectGroupTargets();
				const auto groupMoveTargets = collectGroupMoveTargets(groupTargets);
				const bool groupPositionChanged = ImGui::DragFloat2("Group Position", groupPosition, 1.0f, -10000.0f, 10000.0f);
				if (ImGui::IsItemActivated())
				{
					ensureGroupSnapshot(groupPositionKey, combined, groupMoveTargets);
				}
				if (groupPositionChanged)
				{
					auto itPending = pendingGroupEdits.find(groupPositionKey);
					UIRect baseBounds = (itPending != pendingGroupEdits.end()) ? itPending->second.startBounds : combined;
					const float deltaX = groupPosition[0] - baseBounds.x;
					const float deltaY = groupPosition[1] - baseBounds.y;

					for (const auto& name : groupMoveTargets)
					{
						auto itObj = it->second.find(name);
						if (itObj == it->second.end() || !itObj->second)
						{
							continue;
						}
						UIRect worldBounds = baseBounds;
						if (itPending != pendingGroupEdits.end())
						{
							auto itWorld = itPending->second.startWorldBounds.find(name);
							if (itWorld != itPending->second.startWorldBounds.end())
							{
								worldBounds = itWorld->second;
							}
						}
						else
						{
							std::unordered_map<std::string, UIRect> localCache;
							std::unordered_set<std::string> localVisiting;
							worldBounds = getWorldBoundsForLayout(name, it->second, getWorldBoundsForLayout, localCache, localVisiting);
						}
						worldBounds.x += deltaX;
						worldBounds.y += deltaY;

						UIRect parentBounds{};
						const std::string parentName = itObj->second->GetParentName();
						if (!parentName.empty() && it->second.find(parentName) != it->second.end())
						{
							if (itPending != pendingGroupEdits.end())
							{
								auto itParent = itPending->second.startWorldBounds.find(parentName);
								if (itParent != itPending->second.startWorldBounds.end())
								{
									parentBounds = itParent->second;
									parentBounds.x += deltaX;
									parentBounds.y += deltaY;
								}
								else
								{
									std::unordered_map<std::string, UIRect> localCache;
									std::unordered_set<std::string> localVisiting;
									parentBounds = getWorldBoundsForLayout(parentName, it->second, getWorldBoundsForLayout, localCache, localVisiting);
								}
							}
							else
							{
								std::unordered_map<std::string, UIRect> localCache;
								std::unordered_set<std::string> localVisiting;
								parentBounds = getWorldBoundsForLayout(parentName, it->second, getWorldBoundsForLayout, localCache, localVisiting);
							}
						}
						setLocalFromWorldForLayout(*itObj->second, worldBounds, parentBounds);
					}
					if (itPending != pendingGroupEdits.end())
					{
						itPending->second.updated = true;
					}
				}
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					auto itPending = pendingGroupEdits.find(groupPositionKey);
					if (itPending != pendingGroupEdits.end())
					{
						if (itPending->second.updated)
						{
							std::unordered_map<std::string, nlohmann::json> afterSnapshots;
							captureUISnapshots(groupMoveTargets, afterSnapshots);
							pushUIGroupSnapshotUndo("Move UI Group", itPending->second.beforeSnapshots, afterSnapshots);
						}
						pendingGroupEdits.erase(itPending);
					}
				}

				const bool groupSizeChanged = ImGui::DragFloat2("Group Size", groupSize, 1.0f, 1.0f, 100000.0f);
				if (ImGui::IsItemActivated())
				{
					ensureGroupSnapshot(groupSizeKey, combined, groupTargets);
				}
				if (groupSizeChanged)
				{
					auto itPending = pendingGroupEdits.find(groupSizeKey);
					const UIRect baseBounds = (itPending != pendingGroupEdits.end()) ? itPending->second.startBounds : combined;
					const float safeWidth = (baseBounds.width != 0.0f) ? baseBounds.width : 1.0f;
					const float safeHeight = (baseBounds.height != 0.0f) ? baseBounds.height : 1.0f;
					const float scaleX = groupSize[0] / safeWidth;
					const float scaleY = groupSize[1] / safeHeight;

					for (const auto& name : groupTargets)
					{
						auto itObj = it->second.find(name);
						if (itObj == it->second.end() || !itObj->second)
						{
							continue;
						}
						UIRect startBounds = baseBounds;
						if (itPending != pendingGroupEdits.end())
						{
							auto itWorld = itPending->second.startWorldBounds.find(name);
							if (itWorld != itPending->second.startWorldBounds.end())
							{
								startBounds = itWorld->second;
							}
						}
						else
						{
							startBounds = getWorldBoundsForLayout(name, it->second, getWorldBoundsForLayout, boundsCache, visiting);
						}

						UIRect worldBounds = startBounds;
						worldBounds.x = baseBounds.x + (startBounds.x - baseBounds.x) * scaleX;
						worldBounds.y = baseBounds.y + (startBounds.y - baseBounds.y) * scaleY;
						worldBounds.width = max(1.0f, startBounds.width * scaleX);
						worldBounds.height = max(1.0f, startBounds.height * scaleY);

						UIRect parentBounds{};
						const std::string parentName = itObj->second->GetParentName();
						if (!parentName.empty() && it->second.find(parentName) != it->second.end())
						{
							parentBounds = getWorldBoundsForLayout(parentName, it->second, getWorldBoundsForLayout, boundsCache, visiting);
						}
						setLocalFromWorldForLayout(*itObj->second, worldBounds, parentBounds);
					}
					if (itPending != pendingGroupEdits.end())
					{
						itPending->second.updated = true;
					}
				}
				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					auto itPending = pendingGroupEdits.find(groupSizeKey);
					if (itPending != pendingGroupEdits.end())
					{
						if (itPending->second.updated)
						{
							std::unordered_map<std::string, nlohmann::json> afterSnapshots;
							captureUISnapshots(groupTargets, afterSnapshots);
							pushUIGroupSnapshotUndo("Resize UI Group", itPending->second.beforeSnapshots, afterSnapshots);
						}
						pendingGroupEdits.erase(itPending);
					}
				}
			}

			ImGui::SeparatorText("Align");
			if (m_SelectedUIObjectNames.size() > 1)
			{
				auto reference = getUIObjectByName(m_SelectedUIObjectName);
				if (reference)
				{
					const UIRect referenceBounds = reference->GetBounds();
					auto applyAlignment = [&](const std::function<void(UIRect&)>& action)
						{
							for (const auto& name : m_SelectedUIObjectNames)
							{
								auto target = getUIObjectByName(name);
								if (!target)
								{
									continue;
								}
								UIRect bounds = target->GetBounds();
								action(bounds);
								target->SetBounds(bounds);
							}
						};

					if (ImGui::Button("Left"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.x = referenceBounds.x; });
					}
					ImGui::SameLine();
					if (ImGui::Button("Right"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.x = referenceBounds.x + referenceBounds.width - bounds.width; });
					}
					ImGui::SameLine();
					if (ImGui::Button("H Center"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.x = referenceBounds.x + (referenceBounds.width - bounds.width) * 0.5f; });
					}

					if (ImGui::Button("Top"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.y = referenceBounds.y; });
					}
					ImGui::SameLine();
					if (ImGui::Button("Bottom"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.y = referenceBounds.y + referenceBounds.height - bounds.height; });
					}
					ImGui::SameLine();
					if (ImGui::Button("V Center"))
					{
						applyAlignment([&](UIRect& bounds) { bounds.y = referenceBounds.y + (referenceBounds.height - bounds.height) * 0.5f; });
					}
				}
			}
			else
			{
				ImGui::TextDisabled("Select multiple UI objects to align.");
			}

			ImGui::SeparatorText("Components");

			auto componentTypes = selectedObject->GetComponentTypeNames();
			for (const auto& componentType : componentTypes)
			{
				if (!isUIComponentType(componentType))
				{
					continue;
				}

				ImGui::PushID(componentType.c_str());
				ImGui::Text("%s", componentType.c_str());
				ImGui::SameLine();
				if (ImGui::Button("Remove"))
				{
					nlohmann::json beforeSnapshot;
					selectedObject->Serialize(beforeSnapshot);
					selectedObject->RemoveComponentByTypeName(componentType);
					selectedObject->UpdateInteractableFlags();
					if (uiManager)
					{
						uiManager->RefreshUIListForCurrentScene();
					}
					nlohmann::json afterSnapshot;
					selectedObject->Serialize(afterSnapshot);
					pushUISnapshotUndo("Remove UI Component", beforeSnapshot, afterSnapshot);
				}
				ImGui::PopID();
			}

			std::vector<std::string> uiComponentTypes;
			for (const auto& name : ComponentRegistry::Instance().GetTypeNames())
			{
				if (isUIComponentType(name))
				{
					uiComponentTypes.push_back(name);
				}
			}

			if (!uiComponentTypes.empty())
			{
				static int selectedTypeIndex = 0;
				selectedTypeIndex = std::clamp(selectedTypeIndex, 0, static_cast<int>(uiComponentTypes.size() - 1));
				if (ImGui::BeginCombo("Add UI Component", uiComponentTypes[selectedTypeIndex].c_str()))
				{
					for (int i = 0; i < static_cast<int>(uiComponentTypes.size()); ++i)
					{
						const bool isSelected = (selectedTypeIndex == i);
						if (ImGui::Selectable(uiComponentTypes[i].c_str(), isSelected))
						{
							selectedTypeIndex = i;
						}
						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}
				const std::string& selectedTypeName = uiComponentTypes[selectedTypeIndex];
				const bool alreadyHasComponent = !selectedObject->GetComponentsByTypeName(selectedTypeName).empty();
				ImGui::BeginDisabled(alreadyHasComponent);
				if (ImGui::Button("Add Component"))
				{
					nlohmann::json beforeSnapshot;
					selectedObject->Serialize(beforeSnapshot);
					selectedObject->AddComponentByTypeName(selectedTypeName);
					selectedObject->UpdateInteractableFlags();
					if (uiManager)
					{
						uiManager->RefreshUIListForCurrentScene();
					}
					nlohmann::json afterSnapshot;
					selectedObject->Serialize(afterSnapshot);
					pushUISnapshotUndo("Add UI Component", beforeSnapshot, afterSnapshot);
				}
				ImGui::EndDisabled();
				if (alreadyHasComponent)
				{
					ImGui::SameLine();
					ImGui::TextDisabled("Already added");
				}
			}
			else
			{
				ImGui::TextDisabled("No UI component types registered.");
			}

			if (m_AssetLoader)
			{
				ImGui::SeparatorText("Properties");
				bool uiPropertiesUpdated = false;
				auto componentTypesForEdit = selectedObject->GetComponentTypeNames();
				for (const auto& componentType : componentTypesForEdit)
				{
					if (!isUIComponentType(componentType))
					{
						continue;
					}

					ImGui::PushID(componentType.c_str());
					const ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
					if (ImGui::TreeNodeEx(componentType.c_str(), flags))
					{
						Component* component = selectedObject->GetComponentByTypeName(componentType);
						auto* typeInfo = ComponentRegistry::Instance().Find(componentType);
						if (component && typeInfo)
						{
							auto props = ComponentRegistry::Instance().CollectProperties(typeInfo);
							for (const auto& prop : props)
							{
								const PropertyEditResult editResult = DrawComponentPropertyEditor(component, *prop, *m_AssetLoader);
								const bool updated = editResult.updated;
								const bool activated = editResult.activated;
								const bool deactivated = editResult.deactivated;
								uiPropertiesUpdated = uiPropertiesUpdated || updated;
								const size_t propertyKey = MakePropertyKey(component, prop->GetName());

								if (activated && m_PendingUIPropertySnapshots.find(propertyKey) == m_PendingUIPropertySnapshots.end())
								{
									PendingUIPropertySnapshot pendingSnapshot;
									selectedObject->Serialize(pendingSnapshot.beforeSnapshot);
									m_PendingUIPropertySnapshots.emplace(propertyKey, std::move(pendingSnapshot));
								}

								if (updated)
								{
									auto itSnapshot = m_PendingUIPropertySnapshots.find(propertyKey);
									if (itSnapshot != m_PendingUIPropertySnapshots.end())
									{
										itSnapshot->second.updated = true;
									}
								}

								if (deactivated)
								{
									auto itSnapshot = m_PendingUIPropertySnapshots.find(propertyKey);
									if (itSnapshot != m_PendingUIPropertySnapshots.end())
									{
										if (itSnapshot->second.updated)
										{
											nlohmann::json afterSnapshot;
											selectedObject->Serialize(afterSnapshot);
											const std::string label = "Edit UI " + prop->GetName();
											pushUISnapshotUndo(label, itSnapshot->second.beforeSnapshot, afterSnapshot);
										}
										m_PendingUIPropertySnapshots.erase(itSnapshot);
									}
								}
							}
						}
						else
						{
							ImGui::TextDisabled("Component data unavailable.");
						}
						ImGui::TreePop();
					}
					ImGui::PopID();
				}
				if (uiPropertiesUpdated && uiManager)
				{
					uiManager->RefreshUIListForCurrentScene();
					if (selectedObject->GetComponent<HorizontalBox>())
					{
						uiManager->ApplyHorizontalLayout(sceneName, selectedObject->GetName());
					}
					if (selectedObject->GetComponent<Canvas>())
					{
						uiManager->ApplyCanvasLayout(sceneName, selectedObject->GetName());
					}

					const std::string parentName = selectedObject->GetParentName();
					if (!parentName.empty() && it != uiObjectsByScene.end())
					{
						auto itParent = it->second.find(parentName);
						if (itParent != it->second.end() && itParent->second)
						{
							if (itParent->second->GetComponent<HorizontalBox>())
							{
								uiManager->ApplyHorizontalLayout(sceneName, parentName);
							}
							if (itParent->second->GetComponent<Canvas>())
							{
								uiManager->ApplyCanvasLayout(sceneName, parentName);
							}
						}
					}
				}
			}
			ImGui::SeparatorText("Editor Bindings");
			if (uiManager)
			{
				const std::string selectedName = selectedObject->GetName();
				auto buildTargetList = [&](const std::function<bool(const std::shared_ptr<UIObject>&)>& predicate)
					{
						std::vector<std::string> targets;
						if (it != uiObjectsByScene.end())
						{
							for (const auto& [name, uiObject] : it->second)
							{
								if (!uiObject || name == selectedName)
								{
									continue;
								}
								if (predicate(uiObject))
								{
									targets.push_back(name);
								}
							}
						}
						std::sort(targets.begin(), targets.end());
						return targets;
					};

				auto resolveTargetName = [&](std::unordered_map<std::string, std::string>& bindings,
					const std::vector<std::string>& targets,
					const std::unordered_map<std::string, std::string>& activeBindings) -> std::string&
					{
						auto& targetName = bindings[selectedName];
						auto itBinding = activeBindings.find(selectedName);
						if (itBinding != activeBindings.end())
						{
							targetName = itBinding->second;
						}
						if (targets.empty())
						{
							targetName.clear();
							return targetName;
						}
						if (targetName.empty() || std::find(targets.begin(), targets.end(), targetName) == targets.end())
						{
							targetName = targets.front();
						}
						return targetName;
					};

				if (selectedObject->GetComponent<UIButtonComponent>())
				{
					const auto& activeBindings = uiManager->GetButtonBindings(sceneName);
					const auto buttonTargets = buildTargetList([](const std::shared_ptr<UIObject>&)
						{
							return true;
						});

					auto& buttonTarget = resolveTargetName(m_UIButtonBindingTargets, buttonTargets, activeBindings);

					ImGui::Text("Button Target");
					ImGui::SameLine();
					const char* buttonPreview = buttonTarget.empty() ? "<None>" : buttonTarget.c_str();
					if (ImGui::BeginCombo("##ButtonTarget", buttonPreview))
					{
						for (const auto& name : buttonTargets)
						{
							const bool isSelected = (buttonTarget == name);
							if (ImGui::Selectable(name.c_str(), isSelected))
							{
								buttonTarget = name;
							}
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}
					ImGui::BeginDisabled(buttonTarget.empty());
					if (ImGui::Button("Bind Button Toggle Visibility"))
					{
						uiManager->BindButtonToggleVisibility(sceneName, selectedName, buttonTarget);
					}
					ImGui::EndDisabled();

					ImGui::SameLine();
					if (ImGui::Button("Clear Button Binding"))
					{
						uiManager->ClearButtonBinding(sceneName, selectedName);
					}
				}

				if (selectedObject->GetComponent<UISliderComponent>())
				{
					const auto& activeBindings = uiManager->GetSliderBindings(sceneName);
					const auto sliderTargets = buildTargetList([](const std::shared_ptr<UIObject>& target)
						{
							return target->GetComponent<UIProgressBarComponent>() != nullptr;
						});
					auto& sliderTarget = resolveTargetName(m_UISliderBindingTargets, sliderTargets, activeBindings);

					ImGui::Text("Slider Target");
					ImGui::SameLine();
					const char* sliderPreview = sliderTarget.empty() ? "<None>" : sliderTarget.c_str();
					if (ImGui::BeginCombo("##SliderTarget", sliderPreview))
					{
						for (const auto& name : sliderTargets)
						{
							const bool isSelected = (sliderTarget == name);
							if (ImGui::Selectable(name.c_str(), isSelected))
							{
								sliderTarget = name;
							}
							if (isSelected)
							{
								ImGui::SetItemDefaultFocus();
							}
						}
						ImGui::EndCombo();
					}

					ImGui::BeginDisabled(sliderTarget.empty());
					if (ImGui::Button("Bind Slider -> Loading Progress"))
					{
						uiManager->BindSliderToProgress(sceneName, selectedName, sliderTarget);
					}
					ImGui::EndDisabled();
					ImGui::SameLine();
					if (ImGui::Button("Clear Slider Binding"))
					{
						uiManager->ClearSliderBinding(sceneName, selectedName);
					}
				}

				if (selectedObject->GetComponent<HorizontalBox>())
				{
					auto alignmentLabel = [](UIHorizontalAlignment alignment)
						{
							switch (alignment)
							{
							case UIHorizontalAlignment::Left:
								return "Left";
							case UIHorizontalAlignment::Center:
								return "Center";
							case UIHorizontalAlignment::Right:
								return "Right";
							case UIHorizontalAlignment::Fill:
								return "Fill";
							default:
								return "Left";
							}
						};

					if (ImGui::Button("Register Selected As Horizontal Slots"))
					{
						for (const auto& name : m_SelectedUIObjectNames)
						{
							if (name == selectedName)
							{
								continue;
							}
							auto child = uiManager->FindUIObject(sceneName, name);
							if (!child)
							{
								continue;
							}
							const auto bounds = child->GetBounds();
							HorizontalBoxSlot slot;
							slot.child = child.get();
							slot.desiredSize = UISize{ bounds.width, bounds.height };
							slot.padding = UIPadding{};
							slot.fillWeight = 1.0f;
							slot.alignment = UIHorizontalAlignment::Fill;
							uiManager->RegisterHorizontalSlot(sceneName, selectedName, name, slot);
						}
						uiManager->RefreshUIListForCurrentScene();
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove Selected From Horizontal"))
					{
						for (const auto& name : m_SelectedUIObjectNames)
						{
							if (name == selectedName)
							{
								continue;
							}
							uiManager->RemoveHorizontalSlot(sceneName, selectedName, name);
						}
						uiManager->RefreshUIListForCurrentScene();
					}
					if (ImGui::Button("Clear Horizontal Slots"))
					{
						uiManager->ClearHorizontalSlots(sceneName, selectedName);
						uiManager->RefreshUIListForCurrentScene();
					}

					auto* horizontalBox = selectedObject->GetComponent<HorizontalBox>();
					if (horizontalBox)
					{
						std::unordered_set<std::string> registeredNames;
						for (const auto& slot : horizontalBox->GetSlots())
						{
							const std::string slotName = slot.child ? slot.child->GetName() : slot.childName;
							if (!slotName.empty())
							{
								registeredNames.insert(slotName);
							}
						}

						std::vector<std::string> availableNames;
						for (const auto& [name, uiObject] : it->second)
						{
							if (!uiObject || name == selectedName)
							{
								continue;
							}
							if (registeredNames.find(name) != registeredNames.end())
							{
								continue;
							}
							availableNames.push_back(name);
						}

						if (!m_HorizontalSlotCandidate.empty()
							&& std::find(availableNames.begin(), availableNames.end(), m_HorizontalSlotCandidate) == availableNames.end())
						{
							m_HorizontalSlotCandidate.clear();
						}

						ImGui::SeparatorText("Add Horizontal Slot");
						const char* preview = m_HorizontalSlotCandidate.empty() ? "<Select UI Object>" : m_HorizontalSlotCandidate.c_str();
						if (ImGui::BeginCombo("##HorizontalSlotCandidate", preview))
						{
							for (const auto& name : availableNames)
							{
								const bool isSelected = (m_HorizontalSlotCandidate == name);
								if (ImGui::Selectable(name.c_str(), isSelected))
								{
									m_HorizontalSlotCandidate = name;
								}
								if (isSelected)
								{
									ImGui::SetItemDefaultFocus();
								}
							}
							ImGui::EndCombo();
						}

						ImGui::BeginDisabled(m_HorizontalSlotCandidate.empty());
						if (ImGui::Button("Add Slot"))
						{
							auto child = uiManager->FindUIObject(sceneName, m_HorizontalSlotCandidate);
							if (child)
							{
								const auto bounds = child->GetBounds();
								HorizontalBoxSlot slot;
								slot.child = child.get();
								slot.desiredSize = UISize{ bounds.width, bounds.height };
								slot.padding = UIPadding{};
								slot.fillWeight = 1.0f;
								slot.alignment = UIHorizontalAlignment::Fill;
								uiManager->RegisterHorizontalSlot(sceneName, selectedName, m_HorizontalSlotCandidate, slot);
								uiManager->RefreshUIListForCurrentScene();
							}
						}
						ImGui::EndDisabled();

						ImGui::SeparatorText("Horizontal Slots");
						auto& slots = horizontalBox->GetSlotsRef();
						if (slots.empty())
						{
							ImGui::TextDisabled("No slots registered.");
						}
						else if (ImGui::BeginTable("HorizontalSlotTable", 6, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
						{
							ImGui::TableSetupColumn("Child");
							ImGui::TableSetupColumn("Desired Size");
							ImGui::TableSetupColumn("Padding");
							ImGui::TableSetupColumn("Fill Weight");
							ImGui::TableSetupColumn("Alignment");
							ImGui::TableSetupColumn("Order");
							ImGui::TableHeadersRow();

							bool reordered = false;
							for (size_t i = 0; i < slots.size(); ++i)
							{
								HorizontalBoxSlot& slot = slots[i];
								ImGui::PushID(static_cast<int>(i));
								ImGui::TableNextRow();

								ImGui::TableSetColumnIndex(0);
								std::string displayName = slot.child ? slot.child->GetName() : slot.childName;
								if (displayName.empty())
								{
									displayName = "<Missing>";
								}
								ImGui::TextUnformatted(displayName.c_str());

								ImGui::TableSetColumnIndex(1);
								float desiredValues[2] = { slot.desiredSize.width, slot.desiredSize.height };
								const bool desiredChanged = ImGui::DragFloat2("##DesiredSize", desiredValues, 1.0f, 0.0f, 100000.0f);
								if (desiredChanged)
								{
									slot.desiredSize.width = max(0.0f, desiredValues[0]);
									slot.desiredSize.height = max(0.0f, desiredValues[1]);
									uiManager->ApplyHorizontalLayout(sceneName, selectedName);
								}
								recordUILongEdit(makeUILayoutKey("HorizontalSlotDesired" + std::to_string(i)), desiredChanged, "Edit Horizontal Slot Desired Size");

								ImGui::TableSetColumnIndex(2);
								float paddingValues[4] = {
									slot.padding.left,
									slot.padding.right,
									slot.padding.top,
									slot.padding.bottom
								};
								const bool paddingChanged = ImGui::DragFloat4("##Padding", paddingValues, 0.5f, 0.0f, 10000.0f);
								if (paddingChanged)
								{
									slot.padding.left   = max(0.0f, paddingValues[0]);
									slot.padding.right  = max(0.0f, paddingValues[1]);
									slot.padding.top    = max(0.0f, paddingValues[2]);
									slot.padding.bottom = max(0.0f, paddingValues[3]);
									uiManager->ApplyHorizontalLayout(sceneName, selectedName);
								}
								recordUILongEdit(makeUILayoutKey("HorizontalSlotPadding" + std::to_string(i)), paddingChanged, "Edit Horizontal Slot Padding");

								ImGui::TableSetColumnIndex(3);
								const bool weightChanged = ImGui::DragFloat("##FillWeight", &slot.fillWeight, 0.1f, 0.0f, 1000.0f);
								if (weightChanged)
								{
									slot.fillWeight = max(0.0f, slot.fillWeight);
									uiManager->ApplyHorizontalLayout(sceneName, selectedName);
								}
								recordUILongEdit(makeUILayoutKey("HorizontalSlotWeight" + std::to_string(i)), weightChanged, "Edit Horizontal Slot Fill Weight");

								ImGui::TableSetColumnIndex(4);
								const char* alignmentPreview = alignmentLabel(slot.alignment);
								bool alignmentChanged = false;
								if (ImGui::BeginCombo("##Alignment", alignmentPreview))
								{
									const UIHorizontalAlignment options[] = {
										UIHorizontalAlignment::Left,
										UIHorizontalAlignment::Center,
										UIHorizontalAlignment::Right,
										UIHorizontalAlignment::Fill
									};
									for (const auto& option : options)
									{
										const bool isSelected = (slot.alignment == option);
										if (ImGui::Selectable(alignmentLabel(option), isSelected))
										{
											slot.alignment = option;
											alignmentChanged = true;
											uiManager->ApplyHorizontalLayout(sceneName, selectedName);
										}
										if (isSelected)
										{
											ImGui::SetItemDefaultFocus();
										}
									}
									ImGui::EndCombo();
								}
								recordUILongEdit(makeUILayoutKey("HorizontalSlotAlignment" + std::to_string(i)), alignmentChanged, "Edit Horizontal Slot Alignment");

								ImGui::TableSetColumnIndex(5);
								bool moved = false;
								nlohmann::json beforeSnapshot;
								const bool canMoveUp = (i > 0);
								const bool canMoveDown = (i + 1 < slots.size());
								ImGui::BeginDisabled(!canMoveUp);
								if (ImGui::SmallButton("Up"))
								{
									selectedObject->Serialize(beforeSnapshot);
									moved = true;
									std::swap(slots[i], slots[i - 1]);
								}
								ImGui::EndDisabled();
								ImGui::SameLine();
								ImGui::BeginDisabled(!canMoveDown);
								if (ImGui::SmallButton("Down"))
								{
									selectedObject->Serialize(beforeSnapshot);
									moved = true;
									std::swap(slots[i], slots[i + 1]);
								}
								ImGui::EndDisabled();
								if (moved)
								{
									nlohmann::json afterSnapshot;
									selectedObject->Serialize(afterSnapshot);
									pushUISnapshotUndo("Reorder Horizontal Slot", beforeSnapshot, afterSnapshot);
									uiManager->ApplyHorizontalLayout(sceneName, selectedName);
									reordered = true;
								}

								ImGui::PopID();
								if (reordered)
								{
									break;
								}
							}
							ImGui::EndTable();
						}
					}

					if (selectedObject->GetComponent<Canvas>())
					{
						if (ImGui::Button("Register Selected As Canvas Slots"))
						{
							for (const auto& name : m_SelectedUIObjectNames)
							{
								if (name == selectedName)
								{
									continue;
								}
								auto child = uiManager->FindUIObject(sceneName, name);
								if (!child)
								{
									continue;
								}
								CanvasSlot slot;
								slot.child = child.get();
								slot.rect = child->GetBounds();
								uiManager->RegisterCanvasSlot(sceneName, selectedName, name, slot);
							}
							uiManager->RefreshUIListForCurrentScene();
						}
						ImGui::SameLine();
						if (ImGui::Button("Remove Selected From Canvas"))
						{
							for (const auto& name : m_SelectedUIObjectNames)
							{
								if (name == selectedName)
								{
									continue;
								}
								uiManager->RemoveCanvasSlot(sceneName, selectedName, name);
							}
							uiManager->RefreshUIListForCurrentScene();
						}
						if (ImGui::Button("Clear Canvas Slots"))
						{
							uiManager->ClearCanvasSlots(sceneName, selectedName);
							uiManager->RefreshUIListForCurrentScene();
						}

						auto* canvas = selectedObject->GetComponent<Canvas>();
						if (canvas)
						{
							std::unordered_set<std::string> registeredNames;
							for (const auto& slot : canvas->GetSlots())
							{
								const std::string slotName = slot.child ? slot.child->GetName() : slot.childName;
								if (!slotName.empty())
								{
									registeredNames.insert(slotName);
								}
							}

							std::vector<std::string> availableNames;
							for (const auto& [name, uiObject] : it->second)
							{
								if (!uiObject || name == selectedName)
								{
									continue;
								}
								if (registeredNames.find(name) != registeredNames.end())
								{
									continue;
								}
								availableNames.push_back(name);
							}

							if (!m_CanvasSlotCandidate.empty()
								&& std::find(availableNames.begin(), availableNames.end(), m_CanvasSlotCandidate) == availableNames.end())
							{
								m_CanvasSlotCandidate.clear();
							}

							ImGui::SeparatorText("Add Canvas Slot");
							const char* preview = m_CanvasSlotCandidate.empty() ? "<Select UI Object>" : m_CanvasSlotCandidate.c_str();
							if (ImGui::BeginCombo("##CanvasSlotCandidate", preview))
							{
								for (const auto& name : availableNames)
								{
									const bool isSelected = (m_CanvasSlotCandidate == name);
									if (ImGui::Selectable(name.c_str(), isSelected))
									{
										m_CanvasSlotCandidate = name;
									}
									if (isSelected)
									{
										ImGui::SetItemDefaultFocus();
									}
								}
								ImGui::EndCombo();
							}

							ImGui::BeginDisabled(m_CanvasSlotCandidate.empty());
							if (ImGui::Button("Add Slot"))
							{
								auto child = uiManager->FindUIObject(sceneName, m_CanvasSlotCandidate);
								if (child)
								{
									CanvasSlot slot;
									slot.child = child.get();
									slot.rect = child->GetBounds();
									uiManager->RegisterCanvasSlot(sceneName, selectedName, m_CanvasSlotCandidate, slot);
									uiManager->RefreshUIListForCurrentScene();
								}
							}
							ImGui::EndDisabled();

							ImGui::SeparatorText("Canvas Slots");
							auto& slots = canvas->GetSlotsRef();
							if (slots.empty())
							{
								ImGui::TextDisabled("No slots registered.");
							}
							else if (ImGui::BeginTable("CanvasSlotTable", 3, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_RowBg))
							{
								ImGui::TableSetupColumn("Child");
								ImGui::TableSetupColumn("Rect");
								ImGui::TableSetupColumn("Order");
								ImGui::TableHeadersRow();

								bool reordered = false;
								for (size_t i = 0; i < slots.size(); ++i)
								{
									CanvasSlot& slot = slots[i];
									ImGui::PushID(static_cast<int>(i));
									ImGui::TableNextRow();

									ImGui::TableSetColumnIndex(0);
									std::string displayName = slot.child ? slot.child->GetName() : slot.childName;
									if (displayName.empty())
									{
										displayName = "<Missing>";
									}
									ImGui::TextUnformatted(displayName.c_str());

									ImGui::TableSetColumnIndex(1);
									float rectValues[4] = { slot.rect.x, slot.rect.y, slot.rect.width, slot.rect.height };
									const bool rectChanged = ImGui::DragFloat4("##Rect", rectValues, 1.0f, -100000.0f, 100000.0f);
									if (rectChanged)
									{
										slot.rect.x = rectValues[0];
										slot.rect.y = rectValues[1];
										slot.rect.width = rectValues[2];
										slot.rect.height = rectValues[3];
										uiManager->ApplyCanvasLayout(sceneName, selectedName);
									}
									recordUILongEdit(makeUILayoutKey("CanvasSlotRect" + std::to_string(i)), rectChanged, "Edit Canvas Slot Rect");

									ImGui::TableSetColumnIndex(2);
									bool moved = false;
									nlohmann::json beforeSnapshot;
									const bool canMoveUp = (i > 0);
									const bool canMoveDown = (i + 1 < slots.size());
									ImGui::BeginDisabled(!canMoveUp);
									if (ImGui::SmallButton("Up"))
									{
										selectedObject->Serialize(beforeSnapshot);
										moved = true;
										std::swap(slots[i], slots[i - 1]);
									}
									ImGui::EndDisabled();
									ImGui::SameLine();
									ImGui::BeginDisabled(!canMoveDown);
									if (ImGui::SmallButton("Down"))
									{
										selectedObject->Serialize(beforeSnapshot);
										moved = true;
										std::swap(slots[i], slots[i + 1]);
									}
									ImGui::EndDisabled();
									if (moved)
									{
										nlohmann::json afterSnapshot;
										selectedObject->Serialize(afterSnapshot);
										pushUISnapshotUndo("Reorder Canvas Slot", beforeSnapshot, afterSnapshot);
										uiManager->ApplyCanvasLayout(sceneName, selectedName);
										reordered = true;
									}

									ImGui::PopID();
									if (reordered)
									{
										break;
									}
								}
								ImGui::EndTable();
							}
						}
					}
				}
			}
			else
			{
				ImGui::TextDisabled("Asset loader not available for property editing.");
			}
		}

		else
		{
			m_PendingUIPropertySnapshots.clear();
			m_LastSelectedUIObjectName.clear();
		}
		ImGui::EndChild();

		ImGui::NextColumn();

		ImGui::BeginChild("CanvasPanel", ImVec2(1920, 1080), true, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
		ImGui::Text("Canvas Preview");
		ImGui::Separator();
		const ImVec2 canvasPos = ImGui::GetCursorScreenPos();
		const ImVec2 canvasAvail = ImGui::GetContentRegionAvail();
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		static bool isDragging = false;
		static std::string draggingName;
		static ImVec2 dragOffset{ 0.0f, 0.0f };
		static bool snapEnabled = true;
		static float snapSize = 10.0f;
		static std::unordered_map<std::string, UIRect> dragStartWorldBounds;
		static std::unordered_map<std::string, nlohmann::json> dragStartSnapshots;
		static std::unordered_map<std::string, nlohmann::json> dragEndSnapshots;
		static std::unordered_map<std::string, float> dragStartRotations;
		static UIRect dragStartSelectionBounds{};
		static bool hasDragSelectionBounds = false;
		static std::unordered_set<std::string> dragTargetNames;
		static ImVec2 dragRotationCenter{ 0.0f, 0.0f };
		static float dragStartAngle = 0.0f;
		static float canvasZoom = 1.0f;
		static ImVec2 canvasPan{ 0.0f, 0.0f };
		static float nudgeStep = 1.0f;
		enum class HandleDragMode
		{
			None,
			Move,
			ResizeTL,
			ResizeTR,
			ResizeBL,
			ResizeBR,
			Rotate
		};
		static HandleDragMode dragMode = HandleDragMode::None;

		ImGui::Checkbox("Snap", &snapEnabled);
		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat("##SnapSize", &snapSize, 1.0f, 1.0f, 200.0f, "%.0f");

		ImGui::SameLine();
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat("Zoom", &canvasZoom, 0.01f, 0.1f, 5.0f, "%.2f");
		ImGui::SameLine();
		if (ImGui::Button("Reset View"))
		{
			canvasZoom = 1.0f;
			canvasPan = { 0.0f, 0.0f };
		}
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat("Nudge", &nudgeStep, 0.5f, 0.5f, 100.0f, "%.1f");

		if (drawList && scene && uiManager)
		{
			const auto& uiObjectsByScene = uiManager->GetUIObjects();
			const auto sceneName = scene->GetName();
			const auto it = uiObjectsByScene.find(sceneName);

			auto getWorldBounds = [&](const std::string& name,
				const std::unordered_map<std::string, std::shared_ptr<UIObject>>& uiMap,
				auto&& getWorldBoundsRef,
				std::unordered_map<std::string, UIRect>& cache,
				std::unordered_set<std::string>& visiting) -> UIRect
				{
					auto cached = cache.find(name);
					if (cached != cache.end())
					{
						return cached->second;
					}

					auto itObj = uiMap.find(name);
					if (itObj == uiMap.end() || !itObj->second)
					{
						return UIRect{};
					}

					if (!visiting.insert(name).second)
					{
						return itObj->second->GetBounds();
					}

					const auto& uiObject = itObj->second;
					UIRect local = uiObject->GetBounds();
					const std::string& parentName = uiObject->GetParentName();
					if (parentName.empty() || uiMap.find(parentName) == uiMap.end())
					{
						cache[name] = local;
						visiting.erase(name);
						return local;
					}

					UIRect parentBounds = getWorldBoundsRef(parentName, uiMap, getWorldBoundsRef, cache, visiting);
					const UIAnchor anchorMin = uiObject->GetAnchorMin();
					const UIAnchor anchorMax = uiObject->GetAnchorMax();
					const UIAnchor pivot = uiObject->GetPivot();

					const float anchorLeft = parentBounds.x + parentBounds.width * anchorMin.x;
					const float anchorTop = parentBounds.y + parentBounds.height * anchorMin.y;
					const float anchorRight = parentBounds.x + parentBounds.width * anchorMax.x;
					const float anchorBottom = parentBounds.y + parentBounds.height * anchorMax.y;

					const bool stretchX = anchorMin.x != anchorMax.x;
					const bool stretchY = anchorMin.y != anchorMax.y;
					const float baseWidth = stretchX ? (anchorRight - anchorLeft) : 0.0f;
					const float baseHeight = stretchY ? (anchorBottom - anchorTop) : 0.0f;

					const float width = stretchX ? (baseWidth + local.width) : local.width;
					const float height = stretchY ? (baseHeight + local.height) : local.height;

					UIRect world;
					world.width = width;
					world.height = height;
					world.x = anchorLeft + local.x - width * pivot.x;
					world.y = anchorTop + local.y - height * pivot.y;

					cache[name] = world;
					visiting.erase(name);
					return world;
				};

			auto setLocalFromWorld = [&](UIObject& uiObject, const UIRect& worldBounds, const UIRect& parentBounds)
				{
					const UIAnchor anchorMin = uiObject.GetAnchorMin();
					const UIAnchor anchorMax = uiObject.GetAnchorMax();
					const UIAnchor pivot = uiObject.GetPivot();

					const float anchorLeft = parentBounds.x + parentBounds.width * anchorMin.x;
					const float anchorTop = parentBounds.y + parentBounds.height * anchorMin.y;
					const float anchorRight = parentBounds.x + parentBounds.width * anchorMax.x;
					const float anchorBottom = parentBounds.y + parentBounds.height * anchorMax.y;

					const bool stretchX = anchorMin.x != anchorMax.x;
					const bool stretchY = anchorMin.y != anchorMax.y;
					const float baseWidth = stretchX ? (anchorRight - anchorLeft) : 0.0f;
					const float baseHeight = stretchY ? (anchorBottom - anchorTop) : 0.0f;

					UIRect local = uiObject.GetBounds();
					local.width = stretchX ? (worldBounds.width - baseWidth) : worldBounds.width;
					local.height = stretchY ? (worldBounds.height - baseHeight) : worldBounds.height;
					local.x = worldBounds.x - anchorLeft + worldBounds.width * pivot.x;
					local.y = worldBounds.y - anchorTop + worldBounds.height * pivot.y;
					uiObject.SetBounds(local);
				};

			auto captureUISnapshots = [&](const std::unordered_set<std::string>& names,
				std::unordered_map<std::string, nlohmann::json>& outSnapshots)
				{
					outSnapshots.clear();
					if (it == uiObjectsByScene.end())
					{
						return;
					}
					for (const auto& name : names)
					{
						auto itObj = it->second.find(name);
						if (itObj != it->second.end() && itObj->second)
						{
							nlohmann::json snapshot;
							itObj->second->Serialize(snapshot);
							outSnapshots.emplace(name, std::move(snapshot));
						}
					}
				};

			auto applyUISnapshots = [&](const std::unordered_map<std::string, nlohmann::json>& snapshots)
				{
					if (!uiManager)
					{
						return;
					}
					auto& map = uiManager->GetUIObjects();
					auto itScene = map.find(sceneName);
					if (itScene == map.end())
					{
						return;
					}
					for (const auto& [name, snapshot] : snapshots)
					{
						auto itObj = itScene->second.find(name);
						if (itObj != itScene->second.end() && itObj->second)
						{
							itObj->second->Deserialize(snapshot);
							itObj->second->UpdateInteractableFlags();
						}
					}
				};

			auto isHorizontalSlotChild = [&](const std::string& name) -> bool
				{
					if (it == uiObjectsByScene.end())
					{
						return false;
					}

					auto itObj = it->second.find(name);
					if (itObj == it->second.end() || !itObj->second)
					{
						return false;
					}

					const std::string& parentName = itObj->second->GetParentName();
					if (parentName.empty())
					{
						return false;
					}

					auto itParent = it->second.find(parentName);
					if (itParent == it->second.end() || !itParent->second)
					{
						return false;
					}

					auto* horizontal = itParent->second->GetComponent<HorizontalBox>();
					if (!horizontal)
					{
						return false;
					}

					for (const auto& slot : horizontal->GetSlots())
					{
						if (slot.child == itObj->second.get())
						{
							return true;
						}
						if (!slot.childName.empty() && slot.childName == name)
						{
							return true;
						}
					}

					return false;
				};

			auto selectionHasHorizontalSlotChild = [&](const std::unordered_set<std::string>& selection) -> bool
				{
					for (const auto& name : selection)
					{
						if (isHorizontalSlotChild(name))
						{
							return true;
						}
					}
					return false;
				};

			auto resolveDragTargets = [&](const std::unordered_set<std::string>& selection) -> std::unordered_set<std::string>
				{
					std::unordered_set<std::string> targets;
					if (it == uiObjectsByScene.end())
					{
						return targets;
					}

					for (const auto& name : selection)
					{
						auto itObj = it->second.find(name);
						if (itObj == it->second.end() || !itObj->second)
						{
							continue;
						}

						const std::string& parentName = itObj->second->GetParentName();
						if (!parentName.empty())
						{
							auto itParent = it->second.find(parentName);
							if (itParent != it->second.end() && itParent->second)
							{
								if (auto* horizontal = itParent->second->GetComponent<HorizontalBox>())
								{
									for (const auto& slot : horizontal->GetSlots())
									{
										if (slot.child == itObj->second.get() || (!slot.childName.empty() && slot.childName == name))
										{
											targets.insert(parentName);
											break;
										}
									}
									if (targets.find(parentName) != targets.end())
									{
										continue;
									}
								}
							}
						}

						targets.insert(name);
					}

					return targets;
				};

			auto computeDragOffset = [&](const ImVec2& localPos, const std::unordered_set<std::string>& targets) -> ImVec2
				{
					if (targets.size() != 1)
					{
						return localPos;
					}

					const std::string& targetName = *targets.begin();
					std::unordered_map<std::string, UIRect> boundsCache;
					std::unordered_set<std::string> visiting;
					const auto bounds = getWorldBounds(targetName, it->second, getWorldBounds, boundsCache, visiting);
					return { localPos.x - bounds.x, localPos.y - bounds.y };
				};

			auto nudgeSelection = [&](float deltaX, float deltaY)
				{
					if (m_SelectedUIObjectNames.empty() || it == uiObjectsByScene.end())
					{
						return;
					}

					std::unordered_map<std::string, nlohmann::json> beforeSnapshots;
					std::unordered_map<std::string, nlohmann::json> afterSnapshots;
					captureUISnapshots(m_SelectedUIObjectNames, beforeSnapshots);

					std::unordered_map<std::string, UIRect> boundsCache;
					std::unordered_set<std::string> visiting;

					for (const auto& name : m_SelectedUIObjectNames)
					{
						auto itObj = it->second.find(name);
						if (itObj == it->second.end() || !itObj->second)
						{
							continue;
						}

						UIRect worldBounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
						worldBounds.x += deltaX;
						worldBounds.y += deltaY;

						UIRect parentBounds{};
						const std::string parentName = itObj->second->GetParentName();
						if (!parentName.empty() && it->second.find(parentName) != it->second.end())
						{
							parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
						}
						setLocalFromWorld(*itObj->second, worldBounds, parentBounds);
					}

					captureUISnapshots(m_SelectedUIObjectNames, afterSnapshots);
					if (beforeSnapshots != afterSnapshots)
					{
						m_UndoManager.Push(UndoManager::Command{
							"Nudge UI Objects",
							[applyUISnapshots, beforeSnapshots]()
							{
								applyUISnapshots(beforeSnapshots);
							},
							[applyUISnapshots, afterSnapshots]()
							{
								applyUISnapshots(afterSnapshots);
							}
							});
					}
				};

			const ImVec2 designCanvasSize{ 1920.0f, 1080.0f };
			const float scaleX = (designCanvasSize.x > 0.0f) ? (canvasAvail.x / designCanvasSize.x) : 1.0f;
			const float scaleY = (designCanvasSize.y > 0.0f) ? (canvasAvail.y / designCanvasSize.y) : 1.0f;

			const float baseScale = std::min(scaleX, scaleY);
			canvasZoom = std::clamp(canvasZoom, 0.1f, 5.0f);
			const float scale = baseScale * canvasZoom;
			const ImVec2 viewOrigin = { canvasPos.x + canvasPan.x, canvasPos.y + canvasPan.y };

			const ImU32 background = IM_COL32(30, 30, 36, 255);
			drawList->AddRectFilled(canvasPos, { canvasPos.x + canvasAvail.x, canvasPos.y + canvasAvail.y }, background, 6.0f);
			drawList->AddRect(canvasPos, { canvasPos.x + canvasAvail.x, canvasPos.y + canvasAvail.y }, IM_COL32(0, 0, 0, 180), 6.0f);

			if (snapEnabled && snapSize > 1.0f)
			{
				const float gridStep = snapSize * scale;
				const float offsetX = std::fmod(canvasPan.x, gridStep);
				const float offsetY = std::fmod(canvasPan.y, gridStep);
				for (float x = canvasPos.x + offsetX; x < canvasPos.x + canvasAvail.x; x += gridStep)
				{
					drawList->AddLine({ x, canvasPos.y }, { x, canvasPos.y + canvasAvail.y }, IM_COL32(45, 45, 50, 90));
				}
				for (float y = canvasPos.y + offsetY; y < canvasPos.y + canvasAvail.y; y += gridStep)
				{
					drawList->AddLine({ canvasPos.x, y }, { canvasPos.x + canvasAvail.x, y }, IM_COL32(45, 45, 50, 90));
				}
			}

			UIObject* selectedObject = nullptr;
			if (it != uiObjectsByScene.end())
			{
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;

				for (const auto& [name, uiObject] : it->second)
				{
					if (!uiObject || !uiObject->HasBounds())
					{
						continue;
					}

					const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
					const ImVec2 min = { viewOrigin.x + bounds.x * scale, viewOrigin.y + bounds.y * scale };
					const ImVec2 max = { min.x + bounds.width * scale, min.y + bounds.height * scale };
					const bool selected = (m_SelectedUIObjectNames.find(uiObject->GetName()) != m_SelectedUIObjectNames.end());
					if (selected)
					{
						selectedObject = uiObject.get();
					}

					bool isVisible = true;
					if (auto* baseComponent = uiObject->GetComponent<UIComponent>())
					{
						isVisible = baseComponent->GetVisible();
					}
					if (!isVisible)
					{
						continue;
					}

					const ImU32 fillColor = selected ? IM_COL32(80, 130, 220, 120) : IM_COL32(90, 90, 110, 120);
					const ImU32 outlineColor = selected ? IM_COL32(120, 160, 255, 255) : IM_COL32(0, 0, 0, 160);

					const float rotationDegrees = uiObject->GetRotationDegrees();
					const float rotationRadians = XMConvertToRadians(rotationDegrees);
					const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
					const ImVec2 half = { (max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f };
					auto rotatePoint = [&](const ImVec2& point)
						{
							const float cosA = std::cos(rotationRadians);
							const float sinA = std::sin(rotationRadians);
							const ImVec2 local = { point.x - center.x, point.y - center.y };
							return ImVec2{
								center.x + local.x * cosA - local.y * sinA,
								center.y + local.x * sinA + local.y * cosA
							};
						};

					ImVec2 corners[4] = {
						rotatePoint({ center.x - half.x, center.y - half.y }),
						rotatePoint({ center.x + half.x, center.y - half.y }),
						rotatePoint({ center.x + half.x, center.y + half.y }),
						rotatePoint({ center.x - half.x, center.y + half.y })
					};

					drawList->AddConvexPolyFilled(corners, 4, fillColor);
					drawList->AddPolyline(corners, 4, outlineColor, true, 2.0f);

					const ImVec2 textPos = { min.x + 6.0f, min.y + 6.0f };
					drawList->AddText(textPos, IM_COL32(230, 230, 230, 255), uiObject->GetName().c_str());

					if (auto* textComponent = uiObject->GetComponent<UITextComponent>())
					{
						const auto& text = textComponent->GetText();
						if (!text.empty())
						{
							drawList->AddText({ min.x + 6.0f, min.y + 24.0f }, IM_COL32(240, 240, 240, 255), text.c_str());
						}
					}

					if (auto* buttonComponent = uiObject->GetComponent<UIButtonComponent>())
					{
						ImU32 buttonOutline = IM_COL32(100, 100, 100, 200);
						if (buttonComponent->GetIsHovered())
						{
							buttonOutline = IM_COL32(160, 200, 255, 255);
						}
						if (buttonComponent->GetIsPressed())
						{
							buttonOutline = IM_COL32(120, 160, 220, 255);
						}
						drawList->AddPolyline(corners, 4, buttonOutline, true, 2.0f);
					}

					if (auto* sliderComponent = uiObject->GetComponent<UISliderComponent>())
					{
						const float normalized = sliderComponent->GetNormalizedValue();
						const ImVec2 barMin = { min.x + 6.0f, max.y - 16.0f };
						const ImVec2 barMax = { max.x - 6.0f, max.y - 8.0f };
						const float barWidth = max(0.0f, barMax.x - barMin.x);
						const ImVec2 fillMax = { barMin.x + barWidth * normalized, barMax.y };
						drawList->AddRectFilled(barMin, barMax, IM_COL32(60, 60, 60, 180), 2.0f);
						drawList->AddRectFilled(barMin, fillMax, IM_COL32(120, 180, 255, 220), 2.0f);
					}

					if (auto* progressComponent = uiObject->GetComponent<UIProgressBarComponent>())
					{
						const float percent = std::clamp(progressComponent->GetPercent(), 0.0f, 1.0f);
						const ImVec2 barMin = { min.x + 6.0f, max.y - 12.0f };
						const ImVec2 barMax = { max.x - 6.0f, max.y - 6.0f };
						const float barWidth = max(0.0f, barMax.x - barMin.x);
						const ImVec2 fillMax = { barMin.x + barWidth * percent, barMax.y };
						drawList->AddRectFilled(barMin, barMax, IM_COL32(50, 50, 50, 160), 2.0f);
						drawList->AddRectFilled(barMin, fillMax, IM_COL32(140, 220, 140, 220), 2.0f);
					}
				}
			}

			if (m_SelectedUIObjectNames.size() > 1 && it != uiObjectsByScene.end())
			{
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;
				bool first = true;
				UIRect combined{};

				for (const auto& name : m_SelectedUIObjectNames)
				{
					auto itObj = it->second.find(name);
					if (itObj == it->second.end() || !itObj->second)
					{
						continue;
					}
					const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
					if (first)
					{
						combined = bounds;
						first = false;
					}
					else
					{
						const float minX = min(combined.x, bounds.x);
						const float minY = min(combined.y, bounds.y);
						const float maxX = max(combined.x + combined.width, bounds.x + bounds.width);
						const float maxY = max(combined.y + combined.height, bounds.y + bounds.height);
						combined.x = minX;
						combined.y = minY;
						combined.width = maxX - minX;
						combined.height = maxY - minY;
					}
				}

				if (!first)
				{
					const ImVec2 min = { viewOrigin.x + combined.x * scale, viewOrigin.y + combined.y * scale };
					const ImVec2 max = { min.x + combined.width * scale, min.y + combined.height * scale };
					drawList->AddRect(min, max, IM_COL32(160, 200, 255, 180), 0.0f, 0, 2.0f);
					const float handleSize = 6.0f;
					const ImU32 handleColor = IM_COL32(200, 200, 200, 255);
					std::array<ImVec2, 4> corners = { min, {max.x, min.y}, {max.x, max.y}, {min.x, max.y} };
					for (const auto& corner : corners)
					{
						drawList->AddRectFilled({ corner.x - handleSize, corner.y - handleSize }, { corner.x + handleSize, corner.y + handleSize }, handleColor);
					}
					const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
					const ImVec2 topCenter = { (min.x + max.x) * 0.5f, min.y };
					ImVec2 direction = { topCenter.x - center.x, topCenter.y - center.y };
					const float dirLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
					if (dirLength > 0.001f)
					{
						direction.x /= dirLength;
						direction.y /= dirLength;
					}
					else
					{
						direction = { 0.0f, -1.0f };
					}
					const ImVec2 rotationHandle = { topCenter.x + direction.x * 20.0f, topCenter.y + direction.y * 20.0f };
					drawList->AddLine(topCenter, rotationHandle, IM_COL32(160, 200, 255, 200), 2.0f);
					drawList->AddCircleFilled(rotationHandle, 5.0f, IM_COL32(200, 200, 200, 255));
				}
			}

			ImGui::InvisibleButton("CanvasHitBox", canvasAvail);
			const bool canvasHovered = ImGui::IsItemHovered();
			ImGuiIO& io = ImGui::GetIO();

			if (canvasHovered && io.KeyCtrl && io.MouseWheel != 0.0f)
			{
				const float prevZoom = canvasZoom;
				canvasZoom = std::clamp(canvasZoom + io.MouseWheel * 0.1f, 0.1f, 5.0f);
				if (canvasZoom != prevZoom)
				{
					const ImVec2 mousePos = io.MousePos;
					const ImVec2 localBefore = {
						(mousePos.x - viewOrigin.x) / (baseScale * prevZoom),
						(mousePos.y - viewOrigin.y) / (baseScale * prevZoom)
					};
					canvasPan.x = mousePos.x - canvasPos.x - localBefore.x * (baseScale * canvasZoom);
					canvasPan.y = mousePos.y - canvasPos.y - localBefore.y * (baseScale * canvasZoom);
				}
			}

			if (canvasHovered && ImGui::IsMouseDown(ImGuiMouseButton_Middle))
			{
				canvasPan.x += io.MouseDelta.x;
				canvasPan.y += io.MouseDelta.y;
			}

			if (canvasHovered && !io.WantTextInput && !isDragging && !m_SelectedUIObjectNames.empty())
			{
				const float step = io.KeyShift ? (nudgeStep * 10.0f) : nudgeStep;
				if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow))
				{
					nudgeSelection(-step, 0.0f);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_RightArrow))
				{
					nudgeSelection(step, 0.0f);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
				{
					nudgeSelection(0.0f, -step);
				}
				if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
				{
					nudgeSelection(0.0f, step);
				}
			}

			auto getHandleHit = [&](const ImVec2& mouse,
				const std::array<ImVec2, 4>& corners,
				const ImVec2& rotationHandle) -> HandleDragMode
				{
					const float handleSize = 8.0f;
					const float rotationRadius = 8.0f;

					auto hit = [&](const ImVec2& center)
						{
							return mouse.x >= center.x - handleSize && mouse.x <= center.x + handleSize
								&& mouse.y >= center.y - handleSize && mouse.y <= center.y + handleSize;
						};

					auto hitCircle = [&](const ImVec2& center)
						{
							const float dx = mouse.x - center.x;
							const float dy = mouse.y - center.y;
							return (dx * dx + dy * dy) <= rotationRadius * rotationRadius;
						};

					if (hit(corners[0])) return HandleDragMode::ResizeTL;
					if (hit(corners[1])) return HandleDragMode::ResizeTR;
					if (hit(corners[3])) return HandleDragMode::ResizeBL;
					if (hit(corners[2])) return HandleDragMode::ResizeBR;
					if (hitCircle(rotationHandle)) return HandleDragMode::Rotate;
					return HandleDragMode::None;
				};


			auto getBorderHit = [&](const UIRect& bounds, const ImVec2& localPoint, float threshold) -> HandleDragMode
				{
					const float centerX = bounds.x + bounds.width * 0.5f;
					const float centerY = bounds.y + bounds.height * 0.5f;
					const bool nearLeft = std::abs(localPoint.x - bounds.x) <= threshold;
					const bool nearRight = std::abs(localPoint.x - (bounds.x + bounds.width)) <= threshold;
					const bool nearTop = std::abs(localPoint.y - bounds.y) <= threshold;
					const bool nearBottom = std::abs(localPoint.y - (bounds.y + bounds.height)) <= threshold;

					if (nearLeft && nearTop) return HandleDragMode::ResizeTL;
					if (nearRight && nearTop) return HandleDragMode::ResizeTR;
					if (nearLeft && nearBottom) return HandleDragMode::ResizeBL;
					if (nearRight && nearBottom) return HandleDragMode::ResizeBR;
					if (nearLeft) return (localPoint.y < centerY) ? HandleDragMode::ResizeTL : HandleDragMode::ResizeBL;
					if (nearRight) return (localPoint.y < centerY) ? HandleDragMode::ResizeTR : HandleDragMode::ResizeBR;
					if (nearTop) return (localPoint.x < centerX) ? HandleDragMode::ResizeTL : HandleDragMode::ResizeTR;
					if (nearBottom) return (localPoint.x < centerX) ? HandleDragMode::ResizeBL : HandleDragMode::ResizeBR;

					return HandleDragMode::None;
				};

			bool clickedOnUI = false;

			if (selectedObject && selectedObject->HasBounds() && m_SelectedUIObjectNames.size() == 1)
			{
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;
				const auto bounds = getWorldBounds(selectedObject->GetName(), it->second, getWorldBounds, boundsCache, visiting);
				const ImVec2 min = { viewOrigin.x + bounds.x * scale, viewOrigin.y + bounds.y * scale };
				const ImVec2 max = { min.x + bounds.width * scale, min.y + bounds.height * scale };
				const float handleSize = 6.0f;
				const ImU32 handleColor = IM_COL32(200, 200, 200, 255);

				const float rotationDegrees = selectedObject->GetRotationDegrees();
				const float rotationRadians = XMConvertToRadians(rotationDegrees);
				const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
				const ImVec2 half = { (max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f };
				auto rotatePoint = [&](const ImVec2& point)
					{
						const float cosA = std::cos(rotationRadians);
						const float sinA = std::sin(rotationRadians);
						const ImVec2 local = { point.x - center.x, point.y - center.y };
						return ImVec2{
							center.x + local.x * cosA - local.y * sinA,
							center.y + local.x * sinA + local.y * cosA
						};
					};

				ImVec2 corners[4] = {
					rotatePoint({ center.x - half.x, center.y - half.y }),
					rotatePoint({ center.x + half.x, center.y - half.y }),
					rotatePoint({ center.x + half.x, center.y + half.y }),
					rotatePoint({ center.x - half.x, center.y + half.y })
				};

				drawList->AddPolyline(corners, 4, IM_COL32(120, 160, 255, 255), true, 2.0f);
				drawList->AddRectFilled({ corners[0].x - handleSize, corners[0].y - handleSize }, { corners[0].x + handleSize, corners[0].y + handleSize }, handleColor);
				drawList->AddRectFilled({ corners[1].x - handleSize, corners[1].y - handleSize }, { corners[1].x + handleSize, corners[1].y + handleSize }, handleColor);
				drawList->AddRectFilled({ corners[2].x - handleSize, corners[2].y - handleSize }, { corners[2].x + handleSize, corners[2].y + handleSize }, handleColor);
				drawList->AddRectFilled({ corners[3].x - handleSize, corners[3].y - handleSize }, { corners[3].x + handleSize, corners[3].y + handleSize }, handleColor);
				const ImVec2 topCenter = { (corners[0].x + corners[1].x) * 0.5f, (corners[0].y + corners[1].y) * 0.5f };
				ImVec2 direction = { topCenter.x - center.x, topCenter.y - center.y };
				const float dirLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
				if (dirLength > 0.001f)
				{
					direction.x /= dirLength;
					direction.y /= dirLength;
				}
				else
				{
					direction = { 0.0f, -1.0f };
				}
				const ImVec2 rotationHandle = { topCenter.x + direction.x * 20.0f, topCenter.y + direction.y * 20.0f };
				drawList->AddLine(topCenter, rotationHandle, IM_COL32(160, 200, 255, 200), 2.0f);
				drawList->AddCircleFilled(rotationHandle, 5.0f, IM_COL32(200, 200, 200, 255));
			}

			if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && it != uiObjectsByScene.end())
			{
				const ImVec2 mousePos = io.MousePos;
				const ImVec2 localPosHit = { (mousePos.x - viewOrigin.x) / scale, (mousePos.y - viewOrigin.y) / scale };
				const float interactionScale = (baseScale > 0.0f) ? baseScale : 1.0f;
				const ImVec2 localPosDrag = { (mousePos.x - viewOrigin.x) / interactionScale, (mousePos.y - viewOrigin.y) / interactionScale };
				UIObject* hitObject = nullptr;
				int hitZ = std::numeric_limits<int>::min();
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;

				for (const auto& [name, uiObject] : it->second)
				{
					if (!uiObject || !uiObject->HasBounds())
					{
						continue;
					}

					const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
					const float rotationRadians = XMConvertToRadians(uiObject->GetRotationDegrees());
					const ImVec2 center = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
					const float cosA = std::cos(-rotationRadians);
					const float sinA = std::sin(-rotationRadians);
					const ImVec2 localPoint = {
						center.x + (localPosHit.x - center.x) * cosA - (localPosHit.y - center.y) * sinA,
						center.y + (localPosHit.x - center.x) * sinA + (localPosHit.y - center.y) * cosA
					};
					const bool inside = localPoint.x >= bounds.x && localPoint.x <= bounds.x + bounds.width
						&& localPoint.y >= bounds.y && localPoint.y <= bounds.y + bounds.height;

					if (inside && uiObject->GetZOrder() >= hitZ)
					{
						hitObject = uiObject.get();
						hitZ = uiObject->GetZOrder();
					}
				}

				if (selectedObject && selectedObject->HasBounds())
				{
					if (m_SelectedUIObjectNames.size() == 1)
					{
						const auto bounds = getWorldBounds(selectedObject->GetName(), it->second, getWorldBounds, boundsCache, visiting);
						const ImVec2 min = { viewOrigin.x + bounds.x * scale, viewOrigin.y + bounds.y * scale };
						const ImVec2 max = { min.x + bounds.width * scale, min.y + bounds.height * scale };
						const float rotationRadians = XMConvertToRadians(selectedObject->GetRotationDegrees());
						const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
						const ImVec2 half = { (max.x - min.x) * 0.5f, (max.y - min.y) * 0.5f };
						const ImVec2 localPoint = {
							center.x + (localPosHit.x - center.x) * std::cos(-rotationRadians)
								- (localPosHit.y - center.y) * std::sin(-rotationRadians),
							center.y + (localPosHit.x - center.x) * std::sin(-rotationRadians)
								+ (localPosHit.y - center.y) * std::cos(-rotationRadians)
						};
						auto rotatePoint = [&](const ImVec2& point)
							{
								const float cosA = std::cos(rotationRadians);
								const float sinA = std::sin(rotationRadians);
								const ImVec2 local = { point.x - center.x, point.y - center.y };
								return ImVec2{
									center.x + local.x * cosA - local.y * sinA,
									center.y + local.x * sinA + local.y * cosA
								};
							};
						std::array<ImVec2, 4> corners = {
							rotatePoint({ center.x - half.x, center.y - half.y }),
							rotatePoint({ center.x + half.x, center.y - half.y }),
							rotatePoint({ center.x + half.x, center.y + half.y }),
							rotatePoint({ center.x - half.x, center.y + half.y })
						};
						const ImVec2 topCenter = { (corners[0].x + corners[1].x) * 0.5f, (corners[0].y + corners[1].y) * 0.5f };
						ImVec2 direction = { topCenter.x - center.x, topCenter.y - center.y };
						const float dirLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
						if (dirLength > 0.001f)
						{
							direction.x /= dirLength;
							direction.y /= dirLength;
						}
						else
						{
							direction = { 0.0f, -1.0f };
						}
						const ImVec2 rotationHandle = { topCenter.x + direction.x * 20.0f, topCenter.y + direction.y * 20.0f };

						HandleDragMode handleHit = getHandleHit(mousePos, corners, rotationHandle);
						if (handleHit == HandleDragMode::None)
						{
							const float maxThreshold = std::min(bounds.width, bounds.height) * 0.25f;
							const float borderThreshold = std::min(6.0f / scale, maxThreshold);
							handleHit = getBorderHit(bounds, localPoint, borderThreshold);
						}

						if (handleHit != HandleDragMode::None)
						{
							clickedOnUI = true;
							m_SelectedUIObjectName = selectedObject->GetName();
							m_SelectedUIObjectNames.clear();
							m_SelectedUIObjectNames.insert(selectedObject->GetName());

							if (selectionHasHorizontalSlotChild(m_SelectedUIObjectNames))
							{
								handleHit = HandleDragMode::Move;
							}

							dragTargetNames = resolveDragTargets(m_SelectedUIObjectNames);
							draggingName = selectedObject->GetName();
							isDragging = true;
							dragMode = handleHit;
							dragOffset = (dragMode == HandleDragMode::Move) ? computeDragOffset(localPosDrag, dragTargetNames) : localPosDrag;
							dragStartWorldBounds.clear();
							dragStartRotations.clear();
							for (const auto& name : dragTargetNames)
							{
								auto itObj = it->second.find(name);
								if (itObj != it->second.end() && itObj->second)
								{
									dragStartWorldBounds[name] = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
									dragStartRotations[name] = itObj->second->GetRotationDegrees();
								}
							}
							dragStartSelectionBounds = bounds;
							hasDragSelectionBounds = true;
							captureUISnapshots(dragTargetNames, dragStartSnapshots);
							if (dragMode == HandleDragMode::Rotate)
							{
								dragRotationCenter = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
								dragStartAngle = std::atan2(localPosDrag.y - dragRotationCenter.y, localPosDrag.x - dragRotationCenter.x);
							}

							hitObject = nullptr;
						}
					}
					else if (m_SelectedUIObjectNames.size() > 1)
					{
						UIRect combined{};
						bool first = true;
						for (const auto& name : m_SelectedUIObjectNames)
						{
							const auto bounds = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
							if (first)
							{
								combined = bounds;
								first = false;
							}
							else
							{
								const float minX = min(combined.x, bounds.x);
								const float minY = min(combined.y, bounds.y);
								const float maxX = max(combined.x + combined.width, bounds.x + bounds.width);
								const float maxY = max(combined.y + combined.height, bounds.y + bounds.height);
								combined.x = minX;
								combined.y = minY;
								combined.width = maxX - minX;
								combined.height = maxY - minY;
							}
						}
						if (!first)
						{
							const ImVec2 min = { viewOrigin.x + combined.x * scale, viewOrigin.y + combined.y * scale };
							const ImVec2 max = { min.x + combined.width * scale, min.y + combined.height * scale };
							std::array<ImVec2, 4> corners = { min, {max.x, min.y}, {max.x, max.y}, {min.x, max.y} };
							const ImVec2 center = { (min.x + max.x) * 0.5f, (min.y + max.y) * 0.5f };
							const ImVec2 topCenter = { (min.x + max.x) * 0.5f, min.y };
							ImVec2 direction = { topCenter.x - center.x, topCenter.y - center.y };
							const float dirLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
							if (dirLength > 0.001f)
							{
								direction.x /= dirLength;
								direction.y /= dirLength;
							}
							else
							{
								direction = { 0.0f, -1.0f };
							}
							const ImVec2 rotationHandle = { topCenter.x + direction.x * 20.0f, topCenter.y + direction.y * 20.0f };

							HandleDragMode handleHit = getHandleHit(mousePos, corners, rotationHandle);
							if (handleHit == HandleDragMode::None)
							{
								const float maxThreshold = std::min(combined.width, combined.height) * 0.25f;
								const float borderThreshold = std::min(6.0f / scale, maxThreshold);
								handleHit = getBorderHit(combined, localPosHit, borderThreshold);
							}

							if (handleHit != HandleDragMode::None)
							{
								clickedOnUI = true;
								if (selectionHasHorizontalSlotChild(m_SelectedUIObjectNames))
								{
									handleHit = HandleDragMode::Move;
								}

								dragTargetNames = resolveDragTargets(m_SelectedUIObjectNames);
								isDragging = true;
								dragMode = handleHit;
								dragOffset = (dragMode == HandleDragMode::Move) ? computeDragOffset(localPosDrag, dragTargetNames) : localPosDrag;
								dragStartWorldBounds.clear();
								dragStartRotations.clear();
								for (const auto& name : dragTargetNames)
								{
									auto itObj = it->second.find(name);
									if (itObj != it->second.end() && itObj->second)
									{
										dragStartWorldBounds[name] = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
										dragStartRotations[name] = itObj->second->GetRotationDegrees();
									}
								}
								dragStartSelectionBounds = combined;
								hasDragSelectionBounds = true;
								captureUISnapshots(dragTargetNames, dragStartSnapshots);
								if (dragMode == HandleDragMode::Rotate)
								{
									dragRotationCenter = { combined.x + combined.width * 0.5f, combined.y + combined.height * 0.5f };
									dragStartAngle = std::atan2(localPosDrag.y - dragRotationCenter.y, localPosDrag.x - dragRotationCenter.x);
								}

								hitObject = nullptr;
							}
						}
					}
				}

				if (hitObject)
				{
					clickedOnUI = true;
					HandleDragMode borderMode = HandleDragMode::None;
					{
						std::unordered_map<std::string, UIRect> hitBoundsCache;
						std::unordered_set<std::string> hitVisiting;
						const auto hitBounds = getWorldBounds(hitObject->GetName(), it->second, getWorldBounds, hitBoundsCache, hitVisiting);
						const float rotationRadians = XMConvertToRadians(hitObject->GetRotationDegrees());
						const ImVec2 center = { hitBounds.x + hitBounds.width * 0.5f, hitBounds.y + hitBounds.height * 0.5f };
						const float cosA = std::cos(-rotationRadians);
						const float sinA = std::sin(-rotationRadians);
						const ImVec2 localPoint = {
							center.x + (localPosHit.x - center.x) * cosA - (localPosHit.y - center.y) * sinA,
							center.y + (localPosHit.x - center.x) * sinA + (localPosHit.y - center.y) * cosA
						};
						const float maxThreshold = std::min(hitBounds.width, hitBounds.height) * 0.25f;
						const float borderThreshold = std::min(6.0f / scale, maxThreshold);
						borderMode = getBorderHit(hitBounds, localPoint, borderThreshold);
					}

					const bool append = io.KeyShift;
					if (!append)
					{
						m_SelectedUIObjectNames.clear();
					}
					if (!m_SelectedUIObjectNames.insert(hitObject->GetName()).second && append)
					{
						m_SelectedUIObjectNames.erase(hitObject->GetName());
					}
					m_SelectedUIObjectName = hitObject->GetName();
					if (selectionHasHorizontalSlotChild(m_SelectedUIObjectNames))
					{
						borderMode = HandleDragMode::Move;
					}

					dragTargetNames = resolveDragTargets(m_SelectedUIObjectNames);
					draggingName = hitObject->GetName();
					isDragging = true;
					dragMode = (borderMode != HandleDragMode::None) ? borderMode : HandleDragMode::Move;
					const auto bounds = getWorldBounds(hitObject->GetName(), it->second, getWorldBounds, boundsCache, visiting);
					dragOffset = (dragMode == HandleDragMode::Move) ? computeDragOffset(localPosDrag, dragTargetNames) : localPosDrag;
					dragStartWorldBounds.clear();
					dragStartRotations.clear();
					dragStartSelectionBounds = UIRect{};
					hasDragSelectionBounds = false;
					for (const auto& name : dragTargetNames)
					{
						auto itObj = it->second.find(name);
						if (itObj != it->second.end() && itObj->second)
						{
							dragStartWorldBounds[name] = getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
							dragStartRotations[name] = itObj->second->GetRotationDegrees();
						}
					}
					captureUISnapshots(dragTargetNames, dragStartSnapshots);
				}
			}

			if (canvasHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !clickedOnUI)
			{
				m_SelectedUIObjectNames.clear();
				m_SelectedUIObjectName.clear();
			}

			if (isDragging && io.MouseDown[ImGuiMouseButton_Left] && it != uiObjectsByScene.end())
			{
				const ImVec2 mousePos = io.MousePos;
				const ImVec2 localPos = { (mousePos.x - viewOrigin.x) / scale, (mousePos.y - viewOrigin.y) / scale };
				std::unordered_map<std::string, UIRect> boundsCache;
				std::unordered_set<std::string> visiting;
				const float minSize = 10.0f;
				const bool keepAspect = io.KeyShift;
				const auto& dragSelection = dragTargetNames.empty() ? m_SelectedUIObjectNames : dragTargetNames;

				if (dragMode == HandleDragMode::Move)
				{
					for (const auto& name : dragSelection)
					{
						auto itObj = it->second.find(name);
						if (itObj == it->second.end() || !itObj->second)
						{
							continue;
						}

						UIRect bounds = dragStartWorldBounds.count(name)
							? dragStartWorldBounds.at(name)
							: getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
						float deltaX = localPos.x - (dragOffset.x + bounds.x);
						float deltaY = localPos.y - (dragOffset.y + bounds.y);
						if (snapEnabled && snapSize > 1.0f)
						{
							deltaX = std::round(deltaX / snapSize) * snapSize;
							deltaY = std::round(deltaY / snapSize) * snapSize;
						}
						bounds.x += deltaX;
						bounds.y += deltaY;

						UIRect parentBounds{};
						const std::string parentName = itObj->second->GetParentName();
						if (!parentName.empty() && it->second.find(parentName) != it->second.end())
						{
							parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
						}
						setLocalFromWorld(*itObj->second, bounds, parentBounds);
					}
				}
				else if (dragMode == HandleDragMode::Rotate)
				{
					const float currentAngle = std::atan2(localPos.y - dragRotationCenter.y, localPos.x - dragRotationCenter.x);
					const float deltaRadians = currentAngle - dragStartAngle;
					const float deltaDegrees = XMConvertToDegrees(deltaRadians);

					for (const auto& name : dragSelection)
					{
						auto itObj = it->second.find(name);
						if (itObj == it->second.end() || !itObj->second)
						{
							continue;
						}

						const float startRotation = dragStartRotations.count(name) ? dragStartRotations.at(name) : itObj->second->GetRotationDegrees();
						itObj->second->SetRotationDegrees(startRotation + deltaDegrees);

						if (dragSelection.size() > 1)
						{
							UIRect startBounds = dragStartWorldBounds.count(name)
								? dragStartWorldBounds.at(name)
								: getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);
							const ImVec2 startCenter = { startBounds.x + startBounds.width * 0.5f, startBounds.y + startBounds.height * 0.5f };
							const float cosA = std::cos(deltaRadians);
							const float sinA = std::sin(deltaRadians);
							const ImVec2 offset = { startCenter.x - dragRotationCenter.x, startCenter.y - dragRotationCenter.y };
							const ImVec2 rotated = {
								dragRotationCenter.x + offset.x * cosA - offset.y * sinA,
								dragRotationCenter.y + offset.x * sinA + offset.y * cosA
							};
							UIRect bounds = startBounds;
							bounds.x = rotated.x - bounds.width * 0.5f;
							bounds.y = rotated.y - bounds.height * 0.5f;

							UIRect parentBounds{};
							const std::string parentName = itObj->second->GetParentName();
							if (!parentName.empty() && it->second.find(parentName) != it->second.end())
							{
								parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
							}
							setLocalFromWorld(*itObj->second, bounds, parentBounds);
						}
					}
				}
				else
				{
					if (dragSelection.size() > 1 && hasDragSelectionBounds)
					{
						UIRect selection = dragStartSelectionBounds;
						UIRect newSelection = selection;
						switch (dragMode)
						{
						case HandleDragMode::ResizeTL:
							newSelection.x = min(localPos.x, selection.x + selection.width - minSize);
							newSelection.y = min(localPos.y, selection.y + selection.height - minSize);
							newSelection.width = selection.x + selection.width - newSelection.x;
							newSelection.height = selection.y + selection.height - newSelection.y;
							break;
						case HandleDragMode::ResizeTR:
							newSelection.width = max(minSize, localPos.x - selection.x);
							newSelection.y = min(localPos.y, selection.y + selection.height - minSize);
							newSelection.height = selection.y + selection.height - newSelection.y;
							break;
						case HandleDragMode::ResizeBL:
							newSelection.x = min(localPos.x, selection.x + selection.width - minSize);
							newSelection.width = selection.x + selection.width - newSelection.x;
							newSelection.height = max(minSize, localPos.y - selection.y);
							break;
						case HandleDragMode::ResizeBR:
							newSelection.width = max(minSize, localPos.x - selection.x);
							newSelection.height = max(minSize, localPos.y - selection.y);
							break;
						default:
							break;
						}

						if (keepAspect && selection.height > 0.0f)
						{
							const float aspect = selection.width / selection.height;
							const float adjustedWidth = max(minSize, newSelection.width);
							const float adjustedHeight = max(minSize, adjustedWidth / aspect);
							newSelection.width = adjustedWidth;
							newSelection.height = adjustedHeight;
							switch (dragMode)
							{
							case HandleDragMode::ResizeTL:
								newSelection.x = selection.x + selection.width - newSelection.width;
								newSelection.y = selection.y + selection.height - newSelection.height;
								break;
							case HandleDragMode::ResizeTR:
								newSelection.x = selection.x;
								newSelection.y = selection.y + selection.height - newSelection.height;
								break;
							case HandleDragMode::ResizeBL:
								newSelection.x = selection.x + selection.width - newSelection.width;
								newSelection.y = selection.y;
								break;
							case HandleDragMode::ResizeBR:
								newSelection.x = selection.x;
								newSelection.y = selection.y;
								break;
							default:
								break;
							}
						}

						if (snapEnabled && snapSize > 1.0f)
						{
							newSelection.x = std::round(newSelection.x / snapSize) * snapSize;
							newSelection.y = std::round(newSelection.y / snapSize) * snapSize;
							newSelection.width = std::round(newSelection.width / snapSize) * snapSize;
							newSelection.height = std::round(newSelection.height / snapSize) * snapSize;
						}

						const float scaleX = selection.width > 0.0f ? (newSelection.width / selection.width) : 1.0f;
						const float scaleY = selection.height > 0.0f ? (newSelection.height / selection.height) : 1.0f;

						for (const auto& name : dragSelection)
						{
							auto itObj = it->second.find(name);
							if (itObj == it->second.end() || !itObj->second)
							{
								continue;
							}

							UIRect startBounds = dragStartWorldBounds.count(name)
								? dragStartWorldBounds.at(name)
								: getWorldBounds(name, it->second, getWorldBounds, boundsCache, visiting);

							UIRect bounds = startBounds;
							bounds.x = newSelection.x + (startBounds.x - selection.x) * scaleX;
							bounds.y = newSelection.y + (startBounds.y - selection.y) * scaleY;
							bounds.width = max(minSize, startBounds.width * scaleX);
							bounds.height = max(minSize, startBounds.height * scaleY);

							UIRect parentBounds{};
							const std::string parentName = itObj->second->GetParentName();
							if (!parentName.empty() && it->second.find(parentName) != it->second.end())
							{
								parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
							}
							setLocalFromWorld(*itObj->second, bounds, parentBounds);
						}
					}
					else
					{
						auto found = it->second.find(draggingName);
						if (found != it->second.end() && found->second)
						{
							UIRect bounds = dragStartWorldBounds.count(found->second->GetName())
								? dragStartWorldBounds.at(found->second->GetName())
								: getWorldBounds(found->second->GetName(), it->second, getWorldBounds, boundsCache, visiting);
							const UIRect startBounds = bounds;
							const float rotationRadians = XMConvertToRadians(found->second->GetRotationDegrees());
							const ImVec2 center = { bounds.x + bounds.width * 0.5f, bounds.y + bounds.height * 0.5f };
							const float cosA = std::cos(-rotationRadians);
							const float sinA = std::sin(-rotationRadians);
							const ImVec2 localPoint = {
								center.x + (localPos.x - center.x) * cosA - (localPos.y - center.y) * sinA,
								center.y + (localPos.x - center.x) * sinA + (localPos.y - center.y) * cosA
							};

							const float startAspect = (bounds.height != 0.0f) ? (bounds.width / bounds.height) : 1.0f;
							switch (dragMode)
							{
							case HandleDragMode::ResizeTL:
								bounds.width = max(minSize, bounds.width + (bounds.x - localPoint.x));
								bounds.height = max(minSize, bounds.height + (bounds.y - localPoint.y));
								if (keepAspect)
								{
									bounds.height = max(minSize, bounds.width / startAspect);
									bounds.x = startBounds.x + (startBounds.width - bounds.width);
									bounds.y = startBounds.y + (startBounds.height - bounds.height);
								}
								else
								{
									bounds.x = localPoint.x;
									bounds.y = localPoint.y;
								}
								break;
							case HandleDragMode::ResizeTR:
								bounds.width = max(minSize, localPoint.x - bounds.x);
								bounds.height = max(minSize, bounds.height + (bounds.y - localPoint.y));
								if (keepAspect)
								{
									bounds.height = max(minSize, bounds.width / startAspect);
									bounds.x = startBounds.x;
									bounds.y = startBounds.y + (startBounds.height - bounds.height);
								}
								else
								{
									bounds.y = localPoint.y;
								}
								break;
							case HandleDragMode::ResizeBL:
								bounds.width = max(minSize, bounds.width + (bounds.x - localPoint.x));
								bounds.height = max(minSize, localPoint.y - bounds.y);
								if (keepAspect)
								{
									bounds.height = max(minSize, bounds.width / startAspect);
									bounds.x = startBounds.x + (startBounds.width - bounds.width);
									bounds.y = startBounds.y;
								}
								else
								{
									bounds.x = localPoint.x;
								}
								break;
							case HandleDragMode::ResizeBR:
								bounds.width = max(minSize, localPoint.x - bounds.x);
								bounds.height = max(minSize, localPoint.y - bounds.y);
								if (keepAspect)
								{
									bounds.height = max(minSize, bounds.width / startAspect);
									bounds.x = startBounds.x;
									bounds.y = startBounds.y;
								}
								break;
							default:
								break;
							}

							if (snapEnabled && snapSize > 1.0f)
							{
								bounds.x = std::round(bounds.x / snapSize) * snapSize;
								bounds.y = std::round(bounds.y / snapSize) * snapSize;
								bounds.width = std::round(bounds.width / snapSize) * snapSize;
								bounds.height = std::round(bounds.height / snapSize) * snapSize;
							}

							UIRect parentBounds{};
							const std::string parentName = found->second->GetParentName();
							if (!parentName.empty() && it->second.find(parentName) != it->second.end())
							{
								parentBounds = getWorldBounds(parentName, it->second, getWorldBounds, boundsCache, visiting);
							}
							setLocalFromWorld(*found->second, bounds, parentBounds);
						}
					}
				}
			}

			if (isDragging && !io.MouseDown[ImGuiMouseButton_Left])
			{
				isDragging = false;
				draggingName.clear();
				dragMode = HandleDragMode::None;
				dragEndSnapshots.clear();

				if (it != uiObjectsByScene.end())
				{
					captureUISnapshots(dragTargetNames.empty() ? m_SelectedUIObjectNames : dragTargetNames, dragEndSnapshots);
				}

				if (!dragStartSnapshots.empty() && !dragEndSnapshots.empty())
				{
					auto before = dragStartSnapshots;
					auto after = dragEndSnapshots;
					bool changed = false;
					for (const auto& [name, snapshot] : after)
					{
						auto itBefore = before.find(name);
						if (itBefore == before.end() || itBefore->second != snapshot)
						{
							changed = true;
							break;
						}
					}

					if (changed)
					{
						m_UndoManager.Push(UndoManager::Command{
							"Transform UI Objects",
							[this, uiManager, sceneName, before]()
							{
								if (!uiManager)
								{
									return;
								}
								auto& map = uiManager->GetUIObjects();
								auto itScene = map.find(sceneName);
								if (itScene == map.end())
								{
									return;
								}
								for (const auto& [name, snapshot] : before)
								{
									auto itObj = itScene->second.find(name);
									if (itObj != itScene->second.end() && itObj->second)
									{
										itObj->second->Deserialize(snapshot);
										itObj->second->UpdateInteractableFlags();
									}
								}
							},
							[this, uiManager, sceneName, after]()
							{
								if (!uiManager)
								{
									return;
								}
								auto& map = uiManager->GetUIObjects();
								auto itScene = map.find(sceneName);
								if (itScene == map.end())
								{
									return;
								}
								for (const auto& [name, snapshot] : after)
								{
									auto itObj = itScene->second.find(name);
									if (itObj != itScene->second.end() && itObj->second)
									{
										itObj->second->Deserialize(snapshot);
										itObj->second->UpdateInteractableFlags();
									}
								}
							}
							});
					}
				}
				dragStartSnapshots.clear();
				dragEndSnapshots.clear();
				dragStartWorldBounds.clear();
				dragStartRotations.clear();
				dragTargetNames.clear();
				hasDragSelectionBounds = false;
			}
		}
		else
		{
			ImGui::InvisibleButton("CanvasHitBox", canvasAvail);
		}
		ImGui::EndChild();

		ImGui::Columns(1);
		ImGui::End();
	}
}

// 카메라 절두체 그림


void EditorApplication::FocusEditorCameraOnObject(const std::shared_ptr<GameObject>& object)
{
	if (!object)
	{
		return;
	}

	auto scene = m_SceneManager.GetCurrentScene();
	if (!scene)
	{
		return;
	}

	auto editorCamera = scene->GetEditorCamera();
	if (!editorCamera)
	{
		return;
	}

	auto* cameraComponent = editorCamera->GetComponent<CameraComponent>();
	if (!cameraComponent)
	{
		return;
	}

	auto* transform = object->GetComponent<TransformComponent>();
	if (!transform)
	{
		return;
	}

	const XMFLOAT4X4 world = transform->GetWorldMatrix();
	const XMFLOAT3 target{ world._41, world._42, world._43 };
	const XMFLOAT3 up = XMFLOAT3(0.0f, 0.1f, 0.0f); // editor에서는 up 고정
	const XMFLOAT3 eye = cameraComponent->GetEye();

	const XMVECTOR eyeVec = XMLoadFloat3(&eye);
	const XMVECTOR targetVec = XMLoadFloat3(&target);
	XMVECTOR toTarget = XMVectorSubtract(targetVec, eyeVec);
	float distance = XMVectorGetX(XMVector3Length(toTarget));
	XMVECTOR forwardVec = XMVectorZero();

	if (distance > 0.00001f)
	{
		forwardVec = XMVector3Normalize(toTarget);
	}
	else
	{
		const XMFLOAT3 look = cameraComponent->GetLook();
		const XMVECTOR lookVec = XMLoadFloat3(&look);
		const XMVECTOR fallback = XMVectorSubtract(lookVec, eyeVec);
		const float fallbackDistance = XMVectorGetX(XMVector3Length(fallback));

		if (fallbackDistance > 0.001f)
		{
			forwardVec = XMVector3Normalize(fallback);
			distance = fallbackDistance;
		}
		else
		{
			forwardVec = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
			distance = 5.0f;
		}
	}

	XMFLOAT3 newEye = { target.x,target.y + 5,target.z - 5 };
	cameraComponent->SetEyeLookUp(newEye, target, up);
}


void EditorApplication::CreateDockSpace()
{
	ImGui::DockSpaceOverViewport(
		ImGui::GetID("EditorDockSpace"),
		ImGui::GetMainViewport(),
		ImGuiDockNodeFlags_PassthruCentralNode
	);
}

// 초기 창 셋팅
void EditorApplication::SetupEditorDockLayout()
{
	ImGuiID dockspaceID = ImGui::GetID("EditorDockSpace");

	ImGui::DockBuilderRemoveNode(dockspaceID);
	ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode);
	ImGui::DockBuilderSetNodeSize(dockspaceID, ImGui::GetMainViewport()->WorkSize);

	ImGuiID dockMain = dockspaceID; // dockMain을 쪼개서 쓰는 것.
	ImGuiID dockLeft,
		dockRight,
		dockRightA,
		dockRightB,
		dockBottom;



	// 분할   ( 어떤 영역을, 어느방향에서, 비율만큼, 뗀 영역의 이름, Dockspacemain)
	// 나눠지는 순서도 중요
	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Left, 0.2f, &dockLeft, &dockMain);

	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.30f, &dockRight, &dockMain); //Right 20%
	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Down, 0.2f, &dockBottom, &dockMain);  //Down 20%

	ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Left, 0.40f, &dockRightA, &dockMain); // Right 10%
	ImGui::DockBuilderSplitNode(dockRight, ImGuiDir_Right, 0.60f, &dockRightB, &dockMain);// Right 10%

	ImGui::DockBuilderSplitNode(dockMain, ImGuiDir_Right, 0.10f, &dockRight, &dockMain);

	//Down 30%

// 창 할당
	ImGui::DockBuilderDockWindow("Hierarchy", dockRightA);
	ImGui::DockBuilderDockWindow("Inspector", dockRightB);
	ImGui::DockBuilderDockWindow("Folder", dockBottom);
	ImGui::DockBuilderDockWindow("Game", dockMain);
	ImGui::DockBuilderDockWindow("Editor", dockMain);

	ImGui::DockBuilderFinish(dockspaceID);
}

SceneStateSnapshot EditorApplication::CaptureSceneState(const std::shared_ptr<Scene>& scene) const
{
	SceneStateSnapshot snapshot;
	snapshot.currentPath = m_CurrentScenePath;
	snapshot.selectedPath = m_SelectedResourcePath;
	snapshot.selectedObjectName = m_SelectedObjectName;
	snapshot.lastSelectedObjectName = m_LastSelectedObjectName;
	snapshot.objectNameBuffer = m_ObjectNameBuffer;
	snapshot.lastSceneName = m_LastSceneName;
	snapshot.sceneNameBuffer = m_SceneNameBuffer;

	if (scene)
	{
		snapshot.hasScene = true;
		scene->Serialize(snapshot.data);
	}
	return snapshot;
}

void EditorApplication::RestoreSceneState(const SceneStateSnapshot& snapshot)
{
	ClearPendingPropertySnapshots();

	if (snapshot.hasScene)
	{
		m_SceneManager.LoadSceneFromJsonData(snapshot.data, snapshot.currentPath);
	}
	m_CurrentScenePath = snapshot.currentPath;
	m_SelectedResourcePath = snapshot.selectedPath;
	m_SelectedObjectName = snapshot.selectedObjectName;
	m_LastSelectedObjectName = snapshot.lastSelectedObjectName;
	m_SelectedObjectNames.clear();
	if (!m_SelectedObjectName.empty())
	{
		m_SelectedObjectNames.insert(m_SelectedObjectName);
	}
	m_ObjectNameBuffer = snapshot.objectNameBuffer;
	m_LastSceneName = snapshot.lastSceneName;
	m_SceneNameBuffer = snapshot.sceneNameBuffer;
}

void EditorApplication::ClearPendingPropertySnapshots()
{
	m_PendingPropertySnapshots.clear();
	m_LastPendingSnapshotScenePath.clear();
}

void EditorApplication::OnResize(int width, int height)
{
	__super::OnResize(width, height);

	if (auto* uiManager = m_SceneManager.GetUIManager())
	{
		uiManager->SetViewportSize(UISize{ static_cast<float>(width), static_cast<float>(height) });
		uiManager->SetReferenceResolution(UISize{ 2560.0f, 1600.0f });
		uiManager->SetUseAnchorLayout(false);
		uiManager->SetUseResolutionScale(true);
	}

	if (m_InputManager)
	{
		m_InputManager->SetUIReferenceSize(DirectX::XMFLOAT2{ 2560.0f, 1600.0f });
	}
}

void EditorApplication::OnClose()
{
	m_SceneManager.RequestQuit();
}
