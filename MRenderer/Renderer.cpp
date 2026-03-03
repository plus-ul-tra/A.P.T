#include "OpaquePass.h"
#include "ShadowPass.h"
#include "DepthPass.h"
#include "WallPass.h"
#include "TransparentPass.h"
#include "BlurPass.h"
#include "PostPass.h"
#include "FrustumPass.h"
#include "RefractionPass.h"
#include "RenderTargetContext.h"
#include "DebugLinePass.h"
#include "EmissivePass.h"
#include "UIPass.h"
#include "Renderer.h"

namespace
{
	constexpr DXGI_FORMAT kSceneColorFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;
}

#include <algorithm>
//맵 순회하면서 지우거나 찾는 헬퍼 함수
namespace
{
	template <typename MapType>
	void EraseById(MapType& map, UINT32 id)
	{
		for (auto it = map.begin(); it != map.end();)
		{
			if (it->first.id == id)
			{
				it = map.erase(it);
				continue;
			}

			++it;
		}
	}

	template <typename MapType>
	auto FindById(MapType& map, UINT32 id)
	{
		return std::find_if(map.begin(), map.end(), [id](const auto& pair)
			{
				return pair.first.id == id;
			});
	}
}

UINT32 GetMaxMeshHandleId(const RenderData::FrameData& frame);

void Renderer::Initialize(HWND hWnd, int width, int height, ID3D11Device* device, ID3D11DeviceContext* dxdc)
{
	if (m_bIsInitialized)
		return;

	m_WindowSize.width = width;		m_WindowSize.height = height;

	m_pDevice = device;
	m_pDXDC = dxdc;

	DXSetup(hWnd, width, height);
	//SetupText();

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deferred;
	HRESULT hr = m_pDevice->CreateDeferredContext(0, deferred.GetAddressOf());


	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_VS.cso"), m_pVS.GetAddressOf(), m_pVSCode.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_PS.cso"), m_pPS.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_POS_VS.cso"), m_pVS_P.GetAddressOf(), m_pVSCode_P.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_POS_PS.cso"), m_pPS_P.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Frustum_PS.cso"), m_pPS_Frustum.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Quad_VS.cso"), m_pVS_Quad.GetAddressOf(), m_pVSCode_Quad.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Quad_PS.cso"), m_pPS_Quad.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/UI_VS.cso"), m_pVS_UI.GetAddressOf(), m_pVSCode_UI.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/UI_PS.cso"), m_pPS_UI.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_PBR_VS.cso"), m_pVS_PBR.GetAddressOf(), m_pVSCode_PBR.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_PBR_PS.cso"), m_pPS_PBR.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Post_VS.cso"), m_pVS_Post.GetAddressOf(), m_pVSCode_Post.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Post_PS.cso"), m_pPS_Post.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/SkyBox_VS.cso"), m_pVS_SkyBox.GetAddressOf(), m_pVSCode_SkyBox.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/SkyBox_PS.cso"), m_pPS_SkyBox.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_Shadow_VS.cso"), m_pVS_Shadow.GetAddressOf(), m_pVSCode_Shadow.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_Shadow_PS.cso"), m_pPS_Shadow.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_MakeShadow_VS.cso"), m_pVS_MakeShadow.GetAddressOf(), m_pVSCode_MakeShadow.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_MakeShadow_PS.cso"), m_pPS_MakeShadow.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_MakeShadowTransparent_PS.cso"), m_pPS_MakeShadow_Transparent.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_Emissive_VS.cso"), m_pVS_Emissive.GetAddressOf(), m_pVSCode_Emissive.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_Emissive_PS.cso"), m_pPS_Emissive.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_Wall_VS.cso"), m_pVS_Wall.GetAddressOf(), m_pVSCode_Wall.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_Wall_PS.cso"), m_pPS_Wall.GetAddressOf());

	//LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_Wall_VS.cso"), m_pVS_Wall.GetAddressOf(), m_pVSCode_Wall.GetAddressOf());
	//LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_Wall_PS.cso"), m_pPS_Wall.GetAddressOf());


	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_FullScreen_Triangle_VS.cso"), m_pVS_FSTriangle.GetAddressOf(), m_pVSCode_FSTriangle.GetAddressOf());


	CreateInputLayout();

	m_Pipeline.AddPass(std::make_unique<ShadowPass>(m_RenderContext, m_AssetLoader));
	//m_Pipeline.AddPass(std::make_unique<DepthPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<OpaquePass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<WallPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<TransparentPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<EmissivePass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<RefractionPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<BlurPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<PostPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<UIPass>(m_RenderContext, m_AssetLoader));
	CreateConstBuffer();


	InitTexture();
	InitShaders();

	//블러 테스트
	const wchar_t* filename = L"../MRenderer/fx/Vignette.png";
	hr = S_OK;
	hr = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, WIC_LOADER_DEFAULT,
		nullptr, m_Vignetting.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);

	}

	filename = L"../MRenderer/fx/SunSet.dds";
	hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_DEFAULT, 
		nullptr, m_SkyBox.GetAddressOf());

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);

	}

	//HDRISET
	filename = L"../MRenderer/fx/BlueStudio.dds";
	hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_DEFAULT,
		nullptr, m_pHDRI_1.GetAddressOf());

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);

	}
	//HDRISET
	filename = L"../MRenderer/fx/BlueStudio.dds";
	hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_DEFAULT,
		nullptr, m_pHDRI_2.GetAddressOf());

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);

	}

	filename = L"../MRenderer/fx/WaterNoise.jpg";
	hr = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, WIC_LOADER_DEFAULT,
		nullptr, m_WaterNoise.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
	}



	//그리드
	CreateGridVB();
	const int gridSize = m_HalfCells * 2 + 1;
	m_GridFlags.assign(gridSize, std::vector<int>(gridSize, 0));

	//Quad
	CreateQuadVB();
	CreateQuadIB();
	CreateUIWhiteTexture();


	CreateContext();		//마지막에 실행

	m_bIsInitialized = true;
}

void Renderer::InitializeTest(HWND hWnd, int width, int height, ID3D11Device* device, ID3D11DeviceContext* dxdc)
{
	if (m_bIsInitialized)
		return;

	m_WindowSize.width = width;		m_WindowSize.height = height;

	m_pDevice = device;
	m_pDXDC = dxdc;

	DXSetup(hWnd, width, 1600);
	//SetupText();

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> deferred;
	HRESULT hr = m_pDevice->CreateDeferredContext(0, deferred.GetAddressOf());


	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_VS.cso"), m_pVS.GetAddressOf(), m_pVSCode.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_PS.cso"), m_pPS.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_POS_VS.cso"), m_pVS_P.GetAddressOf(), m_pVSCode_P.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_POS_PS.cso"), m_pPS_P.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Frustum_PS.cso"), m_pPS_Frustum.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Quad_VS.cso"), m_pVS_Quad.GetAddressOf(), m_pVSCode_Quad.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Quad_PS.cso"), m_pPS_Quad.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/UI_VS.cso"), m_pVS_UI.GetAddressOf(), m_pVSCode_UI.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/UI_PS.cso"), m_pPS_UI.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_PBR_VS.cso"), m_pVS_PBR.GetAddressOf(), m_pVSCode_PBR.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_PBR_PS.cso"), m_pPS_PBR.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Post_VS.cso"), m_pVS_Post.GetAddressOf(), m_pVSCode_Post.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Post_PS.cso"), m_pPS_Post.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/SkyBox_VS.cso"), m_pVS_SkyBox.GetAddressOf(), m_pVSCode_SkyBox.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/SkyBox_PS.cso"), m_pPS_SkyBox.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_Shadow_VS.cso"), m_pVS_Shadow.GetAddressOf(), m_pVSCode_Shadow.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_Shadow_PS.cso"), m_pPS_Shadow.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_MakeShadow_VS.cso"), m_pVS_MakeShadow.GetAddressOf(), m_pVSCode_MakeShadow.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_MakeShadow_PS.cso"), m_pPS_MakeShadow.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_MakeShadowTransparent_PS.cso"), m_pPS_MakeShadow_Transparent.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_Emissive_VS.cso"), m_pVS_Emissive.GetAddressOf(), m_pVSCode_Emissive.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_Emissive_PS.cso"), m_pPS_Emissive.GetAddressOf());

	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_Wall_VS.cso"), m_pVS_Wall.GetAddressOf(), m_pVSCode_Wall.GetAddressOf());
	LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_Wall_PS.cso"), m_pPS_Wall.GetAddressOf());

	//LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_Wall_VS.cso"), m_pVS_Wall.GetAddressOf(), m_pVSCode_Wall.GetAddressOf());
	//LoadPixelShaderCSO(_T("../MRenderer/fx/Demo_Wall_PS.cso"), m_pPS_Wall.GetAddressOf());


	LoadVertexShaderCSO(_T("../MRenderer/fx/Demo_FullScreen_Triangle_VS.cso"), m_pVS_FSTriangle.GetAddressOf(), m_pVSCode_FSTriangle.GetAddressOf());


	CreateInputLayout();

	m_Pipeline.AddPass(std::make_unique<ShadowPass>(m_RenderContext, m_AssetLoader));		
	//m_Pipeline.AddPass(std::make_unique<DepthPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<OpaquePass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<WallPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<TransparentPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<EmissivePass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<FrustumPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<DebugLinePass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<RefractionPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<BlurPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<PostPass>(m_RenderContext, m_AssetLoader));
	m_Pipeline.AddPass(std::make_unique<UIPass>(m_RenderContext, m_AssetLoader));
	CreateConstBuffer();


	InitTexture();
	InitShaders();
	
	//블러 테스트
	const wchar_t* filename = L"../MRenderer/fx/Vignette.png";
	hr = S_OK;
	hr = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, WIC_LOADER_DEFAULT,
		nullptr, m_Vignetting.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);

	}

	filename = L"../MRenderer/fx/SunSet.dds";			//★★★★★★★★★★★★★★★★★★★★★★
	hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_DEFAULT, 
		nullptr, m_SkyBox.GetAddressOf());

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);

	}

	filename = L"../MRenderer/fx/WaterNoise.jpg";
	hr = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, WIC_LOADER_DEFAULT,
		nullptr, m_WaterNoise.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
	}

	//HDRISET
	filename = L"../MRenderer/fx/BlueStudio.dds";
	hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_DEFAULT,
		nullptr, m_pHDRI_1.GetAddressOf());

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);

	}
	//HDRISET
	filename = L"../MRenderer/fx/BlueStudio.dds";
	hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
		D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
		0, D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_DEFAULT,
		nullptr, m_pHDRI_2.GetAddressOf());

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);

	}

	//그리드
	CreateGridVB();
	const int gridSize = m_HalfCells * 2 + 1;
	m_GridFlags.assign(gridSize, std::vector<int>(gridSize, 0));

	//Quad
	CreateQuadVB();
	CreateQuadIB();
	CreateUIWhiteTexture();


	CreateContext();		//마지막에 실행

	m_bIsInitialized = true;
}

void Renderer::RenderFrame(const RenderData::FrameData& frame)
{
	dTime += frame.context.deltaTime;
	EnsureMeshBuffers(frame);
	//메인 카메라로 draw
	m_IsEditCam = false;
	m_RenderContext.isEditCam = m_IsEditCam;

	ID3D11ShaderResourceView* nullSRV[40] = { nullptr, };
	m_pDXDC->PSSetShaderResources(0, 40, nullSRV);
	m_Pipeline.Execute(frame, m_pDXDC.Get());

}

void Renderer::RenderFrame(const RenderData::FrameData& frame, RenderTargetContext& rendertargetcontext, RenderTargetContext& rendertargetcontext2)
{
	dTime += frame.context.deltaTime;
	EnsureMeshBuffers(frame);
	//메인 카메라로 draw
	m_IsEditCam = false;
	m_RenderContext.isEditCam = m_IsEditCam;

	ID3D11ShaderResourceView* nullSRV[40] = { nullptr, };
	m_pDXDC->PSSetShaderResources(0, 40, nullSRV);
	m_Pipeline.Execute(frame, m_pDXDC.Get());

	ResolveImguiEditTargetIfNeeded();
	rendertargetcontext.SetShaderResourceView(m_pTexRvScene_Post.Get());


	//edit카메라로 draw
	m_IsEditCam = true;
	m_RenderContext.isEditCam = m_IsEditCam;

	m_pDXDC->PSSetShaderResources(0, 40, nullSRV);
	m_Pipeline.Execute(frame, m_pDXDC.Get());

	rendertargetcontext2.SetShaderResourceView(m_pTexRvScene_Imgui_edit.Get());
}
//Backbuffer Draw
void Renderer::RenderToBackBuffer()
{
	ID3D11DeviceContext* dxdc = m_pDXDC.Get();
	if (!dxdc || !m_pRTView || !m_pTexRvScene_Post)
	{
		return;
	}

	dxdc->OMSetRenderTargets(1, m_pRTView.GetAddressOf(), nullptr);
	SetViewPort(m_WindowSize.width, m_WindowSize.height, dxdc);

	dxdc->OMSetBlendState(m_BState[BS::DEFAULT].Get(), nullptr, 0xFFFFFFFF);
	dxdc->RSSetState(m_RState[RS::SOLID].Get());
	dxdc->OMSetDepthStencilState(m_DSState[DS::DEPTH_OFF].Get(), 0);

	dxdc->PSSetSamplers(0, 1, m_SState[SS::WRAP].GetAddressOf());
	dxdc->PSSetSamplers(1, 1, m_SState[SS::MIRROR].GetAddressOf());
	dxdc->PSSetSamplers(2, 1, m_SState[SS::CLAMP].GetAddressOf());
	dxdc->PSSetSamplers(3, 1, m_SState[SS::BORDER].GetAddressOf());
	dxdc->PSSetSamplers(4, 1, m_SState[SS::BORDER_SHADOW].GetAddressOf());

	dxdc->VSSetShader(m_pVS_FSTriangle.Get(), nullptr, 0);
	dxdc->PSSetShader(m_pPS_Quad.Get(), nullptr, 0);
	dxdc->PSSetShaderResources(0, 1, m_pTexRvScene_Post.GetAddressOf());

	if (m_RenderContext.DrawFSTriangle)
	{
		m_RenderContext.DrawFSTriangle();
	}

	ID3D11ShaderResourceView* nullSrv = nullptr;
	dxdc->PSSetShaderResources(0, 1, &nullSrv);
}


void Renderer::InitVB(const RenderData::FrameData& frame)
{
	for (const auto& [layer, items] : frame.renderItems)
	{
		for (const auto& item : items)
		{
			const MeshHandle handle = item.mesh;
			const auto existingIt = FindById(m_VertexBuffers, handle.id);
			if (existingIt != m_VertexBuffers.end() && existingIt->first.generation == handle.generation)		//이미 있고 세대도 같다면 안만듦
				continue;

			RenderData::MeshData* mesh =
				m_AssetLoader.GetMeshes().Get(item.mesh);

			ComPtr<ID3D11Buffer> vb;
			CreateVertexBuffer(m_pDevice.Get(), mesh->vertices.data(), static_cast<UINT>(mesh->vertices.size() * sizeof(RenderData::Vertex)), sizeof(RenderData::Vertex), vb.GetAddressOf());


			EraseById(m_VertexBuffers, handle.id);
			m_VertexBuffers.emplace(handle, vb);
		}
	}
}

//보니까 데이터 형식 상 vb와 함수 하나로 합치는게 좋을듯
void Renderer::InitIB(const RenderData::FrameData& frame)
{
	for (const auto& [layer, items] : frame.renderItems)
	{
		for (const auto& item : items)
		{
			const MeshHandle handle = item.mesh;
			const auto existingIt = FindById(m_IndexBuffers, handle.id);
			if (existingIt != m_IndexBuffers.end() && existingIt->first.generation == handle.generation)
				continue;

			RenderData::MeshData* mesh =
				m_AssetLoader.GetMeshes().Get(item.mesh);


			ComPtr<ID3D11Buffer> ib;
			CreateIndexBuffer(m_pDevice.Get(), mesh->indices.data(), static_cast<UINT>(mesh->indices.size() * sizeof(uint32_t)), ib.GetAddressOf());
			EraseById(m_IndexBuffers, handle.id);
			EraseById(m_IndexCounts, handle.id);
			m_IndexBuffers.emplace(handle, ib);

			UINT32 cnt = static_cast<UINT32>(mesh->indices.size());
			m_IndexCounts.emplace(handle, cnt);
		}
	}
}

void Renderer::EnsureMeshBuffers(const RenderData::FrameData& frame)
{
	for (const auto& [layer, items] : frame.renderItems)
	{
		for (const auto& item : items)
		{
			const MeshHandle handle = item.mesh;

			if (!handle.IsValid())
			{
				continue;
			}

			RenderData::MeshData* mesh = m_AssetLoader.GetMeshes().Get(handle);
			if (!mesh)
			{
				continue;
			}

			//VB 존재여부 및 세대 체크
			const auto vbIt = FindById(m_VertexBuffers, handle.id);
			const bool vbAlive = (vbIt != m_VertexBuffers.end());
			const bool vbSameGen = (vbAlive && vbIt->first.generation == handle.generation);

			//IB 존재여부 및 세대 체크
			const auto ibIt = FindById(m_IndexBuffers, handle.id);
			const bool ibAlive = (ibIt != m_IndexBuffers.end());
			const bool ibSameGen = (ibAlive && ibIt->first.generation == handle.generation);

			//IndexCount 존재여부 및 세대 체크
			const auto cntIt = FindById(m_IndexCounts, handle.id);
			const bool cntAlive = (cntIt != m_IndexCounts.end());
			const bool cntSameGen = (cntAlive && cntIt->first.generation == handle.generation);

			// 하나라도 세대가 다르면 업데이트 필요
			const bool needsUpdate = !(vbSameGen && ibSameGen && cntSameGen);

			// VB 갱신 
			if (!vbSameGen)
			{
				ComPtr<ID3D11Buffer> vb;
				CreateVertexBuffer(
					m_pDevice.Get(),
					mesh->vertices.data(),
					static_cast<UINT>(mesh->vertices.size() * sizeof(RenderData::Vertex)),
					sizeof(RenderData::Vertex),
					vb.GetAddressOf()
				);

				EraseById(m_VertexBuffers, handle.id);
				m_VertexBuffers.emplace(handle, vb);
			}

			//  IB + Count 갱신 
			if (!ibSameGen || !cntSameGen)
			{
				ComPtr<ID3D11Buffer> ib;
				CreateIndexBuffer(
					m_pDevice.Get(),
					mesh->indices.data(),
					static_cast<UINT>(mesh->indices.size() * sizeof(uint32_t)),
					ib.GetAddressOf()
				);

				EraseById(m_IndexBuffers, handle.id);
				EraseById(m_IndexCounts, handle.id);

				m_IndexBuffers.emplace(handle, ib);
				m_IndexCounts.emplace(handle, static_cast<UINT32>(mesh->indices.size()));
			}

		}
	}
}

void Renderer::InitTexture()
{
	const auto& textures = m_AssetLoader.GetTextures();

	for (const auto& [key, handle] : textures.GetKeyToHandle())
	{
		if (!handle.IsValid())
			continue;

		const auto* texData = textures.Get(handle);
		if (!texData)
			continue;

		// id로 캐시 조회
		const auto existingIt = FindById(m_Textures, handle.id);
		const bool alive = (existingIt != m_Textures.end());
		const bool sameGen = (alive && existingIt->first.generation == handle.generation);

		// 이미 있고 세대도 같으면 스킵
		if (sameGen)
			continue;

		// 세대가 다르면 기존 id 항목 삭제
		if (alive && !sameGen)
		{
			EraseById(m_Textures, handle.id);
		}

		ComPtr<ID3D11ShaderResourceView> srv;

		std::wstring wpath(texData->path.begin(), texData->path.end());

		HRESULT hr = TexturesLoad(texData, wpath.c_str(), srv.GetAddressOf());
		if (FAILED(hr))
		{
			continue;
		}

		// 새 핸들(새 generation)로 저장
		m_Textures.emplace(handle, srv);
	}
}

void Renderer::InitShaders()
{
	const auto& vertexShaders = m_AssetLoader.GetVertexShaders();
	for (const auto& [key, handle] : vertexShaders.GetKeyToHandle())
	{
		if (!handle.IsValid())
		{
			continue;
		}

		const auto* shaderData = vertexShaders.Get(handle);
		if (!shaderData)
		{
			continue;
		}

		const auto existingIt = FindById(m_VertexShaders, handle.id);
		const bool alive = (existingIt != m_VertexShaders.end());
		const bool sameGen = (alive && existingIt->first.generation == handle.generation);

		if (sameGen)
		{
			continue;
		}

		if (alive && !sameGen)
		{
			EraseById(m_VertexShaders, handle.id);
		}

		VertexShaderResources resources{};
		if (!shaderData->path.empty())
		{
			std::wstring vsPath(shaderData->path.begin(), shaderData->path.end());
			std::filesystem::path sourcePath(shaderData->path);
			std::string extension = sourcePath.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
				{
					return static_cast<char>(std::tolower(c));
				});

			if (extension == ".cso")
			{
				LoadVertexShaderCSO(vsPath.c_str(), resources.vertexShader.GetAddressOf(), resources.vertexShaderCode.GetAddressOf());
			}
			else
			{
				LoadVertexShader(vsPath.c_str(), resources.vertexShader.GetAddressOf(), resources.vertexShaderCode.GetAddressOf());
			}
		}

		if (!resources.vertexShader)
		{
			continue;
		}

		m_VertexShaders.emplace(handle, std::move(resources));
	}

	const auto& pixelShaders = m_AssetLoader.GetPixelShaders();
	for (const auto& [key, handle] : pixelShaders.GetKeyToHandle())
	{
		if (!handle.IsValid())
		{
			continue;
		}

		const auto* shaderData = pixelShaders.Get(handle);
		if (!shaderData)
		{
			continue;
		}

		const auto existingIt = FindById(m_PixelShaders, handle.id);
		const bool alive = (existingIt != m_PixelShaders.end());
		const bool sameGen = (alive && existingIt->first.generation == handle.generation);

		if (sameGen)
		{
			continue;
		}

		if (alive && !sameGen)
		{
			EraseById(m_PixelShaders, handle.id);
		}

		PixelShaderResources resources{};
		if (!shaderData->path.empty())
		{
			std::wstring psPath(shaderData->path.begin(), shaderData->path.end());
			std::filesystem::path sourcePath(shaderData->path);
			std::string extension = sourcePath.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c)
				{
					return static_cast<char>(std::tolower(c));
				});

			if (extension == ".cso")
			{
				LoadPixelShaderCSO(psPath.c_str(), resources.pixelShader.GetAddressOf());
			}
			else
			{
				LoadPixelShader(psPath.c_str(), resources.pixelShader.GetAddressOf());
			}
		}

		if (!resources.pixelShader)
		{
			continue;
		}

		m_PixelShaders.emplace(handle, std::move(resources));
	}
}

void Renderer::CreateContext()
{
	m_RenderContext.WindowSize = m_WindowSize;

	m_RenderContext.pDevice					= m_pDevice;
	m_RenderContext.pDXDC					= m_pDXDC;
	m_RenderContext.pDSView					= m_pDSView;
	m_RenderContext.pRTView					= m_pRTView;

	m_RenderContext.vertexBuffers			= &m_VertexBuffers;
	m_RenderContext.indexBuffers			= &m_IndexBuffers;
	m_RenderContext.indexCounts				= &m_IndexCounts;
	m_RenderContext.textures				= &m_Textures;
	m_RenderContext.vertexShaders			= &m_VertexShaders;
	m_RenderContext.pixelShaders			= &m_PixelShaders;

	m_RenderContext.pBCB					= m_pBCB;
	m_RenderContext.BCBuffer				= m_BCBuffer;
	m_RenderContext.pCameraCB				= m_pCameraCB;
	m_RenderContext.CameraCBuffer			= m_CameraCBuffer;
	m_RenderContext.pSkinCB					= m_pSkinCB;
	m_RenderContext.SkinCBuffer				= m_SkinCBuffer;
	m_RenderContext.pLightCB				= m_pLightCB;		
	m_RenderContext.LightCBuffer			= m_LightCBuffer;
	m_RenderContext.pUIB					= m_pUIB;
	m_RenderContext.UIBuffer				= m_UIBuffer;
	m_RenderContext.pMatB					= m_pMatB;
	m_RenderContext.MatBuffer				= m_MatBuffer;
	m_RenderContext.pMaskB					= m_pMaskB;
	m_RenderContext.MaskBuffer				= m_MaskBuffer;

	m_RenderContext.VS						= m_pVS;
	m_RenderContext.PS						= m_pPS;

	m_RenderContext.InputLayout				= m_pInputLayout;
	m_RenderContext.InputLayout_P			= m_pInputLayout_P;


	m_RenderContext.VS_P					= m_pVS_P;
	m_RenderContext.PS_P					= m_pPS_P;
	m_RenderContext.PS_Frustum				= m_pPS_Frustum;

	m_RenderContext.VS_PBR					= m_pVS_PBR;
	m_RenderContext.PS_PBR					= m_pPS_PBR;

	m_RenderContext.VS_Wall					= m_pVS_Wall;
	m_RenderContext.PS_Wall					= m_pPS_Wall;

	m_RenderContext.VS_Quad					= m_pVS_Quad;
	m_RenderContext.PS_Quad					= m_pPS_Quad;

	m_RenderContext.VS_Post					= m_pVS_Post;
	m_RenderContext.PS_Post					= m_pPS_Post;

	m_RenderContext.VS_UI = m_pVS_UI;
	m_RenderContext.PS_UI = m_pPS_UI;

	m_RenderContext.UIQuadVertexBuffer = m_QuadVertexBuffers;
	m_RenderContext.UIQuadIndexBuffer = m_QuadIndexBuffers;
	m_RenderContext.UIQuadIndexCount = m_QuadIndexCounts;
	m_RenderContext.UIWhiteTexture = m_UIWhiteTexture;

	m_RenderContext.VS_MakeShadow			= m_pVS_MakeShadow;
	m_RenderContext.PS_MakeShadow			= m_pPS_MakeShadow;
	m_RenderContext.PS_MakeShadow_Transparent = m_pPS_MakeShadow_Transparent;
	m_RenderContext.VSCode_MakeShadow		= m_pVSCode_MakeShadow;

	m_RenderContext.VS_Emissive				= m_pVS_Emissive;
	m_RenderContext.PS_Emissive				= m_pPS_Emissive;
	m_RenderContext.VSCode_Emissive			= m_pVSCode_Emissive;

	m_RenderContext.RState					= m_RState;
	m_RenderContext.DSState					= m_DSState;
	m_RenderContext.SState					= m_SState;
	m_RenderContext.BState					= m_BState;

	m_RenderContext.pRTScene_Imgui			= m_pRTScene_Imgui;
	m_RenderContext.pRTScene_ImguiMSAA		= m_pRTScene_ImguiMSAA;
	m_RenderContext.pTexRvScene_Imgui		= m_pTexRvScene_Imgui;
	m_RenderContext.pRTView_Imgui			= m_pRTView_Imgui;

	m_RenderContext.pDSTex_Imgui			= m_pDSTex_Imgui;
	m_RenderContext.pDSViewScene_Imgui		= m_pDSViewScene_Imgui;

	m_RenderContext.pRTScene_Imgui_edit		= m_pRTScene_Imgui_edit;
	m_RenderContext.pRTScene_Imgui_editMSAA = m_pRTScene_Imgui_editMSAA;
	m_RenderContext.pTexRvScene_Imgui_edit	= m_pTexRvScene_Imgui_edit;
	m_RenderContext.pRTView_Imgui_edit		= m_pRTView_Imgui_edit;

	m_RenderContext.pDSTex_Imgui_edit		= m_pDSTex_Imgui_edit;
	m_RenderContext.pDSViewScene_Imgui_edit = m_pDSViewScene_Imgui_edit;

	m_RenderContext.pDSTex_Shadow			= m_pDSTex_Shadow;
	m_RenderContext.pDSViewScene_Shadow		= m_pDSViewScene_Shadow;
	m_RenderContext.pShadowRV				= m_pShadowRV;
	m_RenderContext.ShadowTextureSize		= m_ShadowTextureSize;

	m_RenderContext.pDSTex_Depth			= m_pDSTex_Depth;
	m_RenderContext.pDSViewScene_Depth		= m_pDSViewScene_Depth;
	m_RenderContext.pDepthRV				= m_pDepthRV;
	m_RenderContext.pDSViewScene_DepthMSAA	= m_pDSViewScene_DepthMSAA;
	m_RenderContext.pDepthMSAARV			= m_pDepthMSAARV;

	m_RenderContext.pRTScene_Post			= m_pRTScene_Post;
	m_RenderContext.pTexRvScene_Post		= m_pTexRvScene_Post;
	m_RenderContext.pRTView_Post			= m_pRTView_Post;

	m_RenderContext.pRTScene_BlurOrigin			= m_pRTScene_BlurOrigin;
	m_RenderContext.pRTScene_BlurOriginMSAA		= m_pRTScene_BlurOriginMSAA;
	m_RenderContext.pTexRvScene_BlurOrigin		= m_pTexRvScene_BlurOrigin;
	m_RenderContext.pRTView_BlurOrigin			= m_pRTView_BlurOrigin;

	m_RenderContext.pRTScene_Blur			= m_pRTScene_Blur;
	m_RenderContext.pTexRvScene_Blur		= m_pTexRvScene_Blur;
	m_RenderContext.pRTView_Blur			= m_pRTView_Blur;

	m_RenderContext.pRTScene_Refraction			= m_pRTScene_Refraction;
	m_RenderContext.pRTScene_RefractionMSAA		= m_pRTScene_RefractionMSAA;
	m_RenderContext.pTexRvScene_Refraction		= m_pTexRvScene_Refraction;
	m_RenderContext.pRTView_Refraction			= m_pRTView_Refraction;

	m_RenderContext.pRTScene_EmissiveOrigin		= m_pRTScene_EmissiveOrigin;
	m_RenderContext.pRTScene_EmissiveOriginMSAA = m_pRTScene_EmissiveOriginMSAA;
	m_RenderContext.pTexRvScene_EmissiveOrigin	= m_pTexRvScene_EmissiveOrigin;
	m_RenderContext.pRTView_EmissiveOrigin		= m_pRTView_EmissiveOrigin;

	m_RenderContext.pRTScene_Emissive			= m_pRTScene_Emissive;
	m_RenderContext.pTexRvScene_Emissive		= m_pTexRvScene_Emissive;
	m_RenderContext.pRTView_Emissive			= m_pRTView_Emissive;

	m_RenderContext.pHDRI_1						= m_pHDRI_1;
	m_RenderContext.pHDRI_2						= m_pHDRI_2;


	m_RenderContext.Vignetting				= m_Vignetting;

	m_RenderContext.SkyBox					= m_SkyBox;
	m_RenderContext.VS_SkyBox				= m_pVS_SkyBox;
	m_RenderContext.PS_SkyBox				= m_pPS_SkyBox;

	m_RenderContext.VS_Shadow				= m_pVS_Shadow;
	m_RenderContext.PS_Shadow				= m_pPS_Shadow;

	m_RenderContext.WaterNoise				= m_WaterNoise;
	m_RenderContext.dTime = &dTime;

	//FullScreenTriangle
	m_RenderContext.VS_FSTriangle			= m_pVS_FSTriangle;
	m_RenderContext.DrawFSTriangle =
		[this]()
		{
			// 정점 버퍼를 바인딩 해제 (이전에 쓰던 버퍼가 영향을 주지 않도록)
			UINT stride = 0;
			UINT offset = 0;
			ID3D11Buffer* nullBuffer = nullptr;
			m_pDXDC->IASetVertexBuffers(0, 1, &nullBuffer, &stride, &offset);

			// 인덱스 버퍼도 필요 없음
			m_pDXDC->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);

			// 토폴로지는 삼각형 리스트
			m_pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

			// 딱 3개의 정점만 그리라고 명령 (셰이더에서 SV_VertexID로 처리)
			D3D11_VIEWPORT cur;
			UINT n = 1;
			m_pDXDC->RSGetViewports(&n, &cur);

			m_pDXDC->Draw(3, 0);
		};



	//DrawQuad함수
	m_RenderContext.DrawFullscreenQuad =
		[this]()
		{
			UINT stride = sizeof(RenderData::Vertex);
			UINT offset = 0;

			ID3D11Buffer* vb = m_QuadVertexBuffers.Get();
			ID3D11Buffer* ib = m_QuadIndexBuffers.Get();

			m_pDXDC->IASetInputLayout(m_pInputLayout.Get());
			m_pDXDC->IASetVertexBuffers(0, 1, &vb, &stride, &offset);
			m_pDXDC->IASetIndexBuffer(ib, DXGI_FORMAT_R32_UINT, 0);
			m_pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			//m_pDXDC->OMSetDepthStencilState(m_RenderContext.DSState[DS::DEPTH_OFF].Get(), 0);

			m_pDXDC->DrawIndexed(m_QuadIndexCounts, 0, 0);
		};

	m_RenderContext.DrawGrid =
		[this]()
		{
			DrawGrid();
		};

	m_RenderContext.UpdateGrid =
		[this](const RenderData::FrameData& frame)
		{
			UpdateGrid(frame);
		};

	m_RenderContext.MyDrawText =
		[this](float width, float height)
		{
			RenderTextCenter(width, height);
		};
}

void Renderer::ResolveImguiEditTargetIfNeeded()
{
	if (m_dwAA <= 1 || !m_pRTScene_Imgui_editMSAA || !m_pRTScene_Imgui_edit || !m_pDXDC)
	{
		return;
	}

	m_pDXDC->ResolveSubresource(m_pRTScene_Imgui_edit.Get(), 0, m_pRTScene_Imgui_editMSAA.Get(), 0, kSceneColorFormat);
}

HRESULT Renderer::Compile(const WCHAR* FileName, const char* EntryPoint, const char* ShaderModel, ID3DBlob** ppCode)
{
	HRESULT hr = S_OK;
	ID3DBlob* pError = nullptr;

	//컴파일 옵션1.
	UINT Flags = D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;		//열우선 행렬 처리. 구형 DX9 이전까지의 전통적인 방식. 속도가 요구된다면, "행우선" 으로 처리할 것.
	//UINT Flags = D3DCOMPILE_PACK_MATRIX_COLUMN_MAJOR;	//행우선 행렬 처리. 열 우선 처리보다 속도의 향상이 있지만, 행렬을 전치한수 GPU 에 공급해야 한다.
	//UINT Flags = D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_PACK_MATRIX_ROW_MAJOR;
#ifdef _DEBUG
	Flags |= D3DCOMPILE_DEBUG;							//디버깅 모드시 옵션 추가.
#endif

	//셰이더 소스 컴파일.
	hr = D3DCompileFromFile(FileName,
		nullptr,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,          //#include 도 컴파일
		EntryPoint,
		ShaderModel,
		Flags,						//컴파일 옵션.1
		0,							//컴파일 옵션2,  Effect 파일 컴파일시 적용됨. 이외에는 무시됨.
		ppCode,						//[출력] 컴파일된 셰이더 코드.
		&pError						//[출력] 컴파일 에러 코드.
	);
	if (FAILED(hr))
	{
		if (pError)
		{
			MessageBoxA(
				nullptr,
				(char*)pError->GetBufferPointer(),
				"Shader Compile Error",
				MB_OK | MB_ICONERROR
			);
		}
	}
	SafeRelease(pError);
	return hr;
}

HRESULT Renderer::LoadVertexShader(const TCHAR* filename, ID3D11VertexShader** ppVS, ID3DBlob** ppVSCode)
{
	HRESULT hr = S_OK;

	ID3D11VertexShader* pVS = nullptr;
	ID3DBlob* pCode = nullptr;

	hr = Compile((const WCHAR*)filename, "VS_Main", "vs_5_0", &pCode);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("[실패] Vertex Shader 컴파일 실패"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	hr = m_pDevice->CreateVertexShader(
		pCode->GetBufferPointer(),
		pCode->GetBufferSize(),
		nullptr,
		&pVS);

	if (FAILED(hr))
	{
		SafeRelease(pCode);
		return hr;
	}

	*ppVS = pVS;
	*ppVSCode = pCode;

	return hr;
}
HRESULT Renderer::LoadPixelShader(const TCHAR* filename, ID3D11PixelShader** ppPS)
{
	HRESULT hr = S_OK;

	ID3D11PixelShader* pPS = nullptr;
	ID3DBlob* pCode = nullptr;

	hr = Compile((const WCHAR*)filename, "PS_Main", "ps_5_0", &pCode);
	if (FAILED(hr))
	{
		MessageBox(NULL, _T("[실패] Pixel Shader 컴파일 실패"), _T("Error"), MB_OK | MB_ICONERROR);
		return hr;
	}

	hr = m_pDevice->CreatePixelShader(
		pCode->GetBufferPointer(),
		pCode->GetBufferSize(),
		nullptr,
		&pPS);

	SafeRelease(pCode);

	if (FAILED(hr))
		return hr;

	*ppPS = pPS;
	return hr;
}

HRESULT Renderer::LoadVertexShaderCSO(const TCHAR* filename, ID3D11VertexShader** ppVS, ID3DBlob** ppVSCode)
{
	HRESULT hr = D3DReadFileToBlob(filename, ppVSCode);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;

	}

	hr = m_pDevice->CreateVertexShader(
		(*ppVSCode)->GetBufferPointer(),
		(*ppVSCode)->GetBufferSize(),
		nullptr,
		ppVS);

	return hr;
}

HRESULT Renderer::LoadPixelShaderCSO(const TCHAR* filename, ID3D11PixelShader** ppPS)
{
	ID3DBlob* pCode = nullptr;

	HRESULT hr = D3DReadFileToBlob(filename, &pCode);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;

	}

	hr = m_pDevice->CreatePixelShader(
		pCode->GetBufferPointer(),
		pCode->GetBufferSize(),
		nullptr,
		ppPS);

	SafeRelease(pCode);
	return hr;
}

HRESULT Renderer::TexturesLoad(const RenderData::TextureData* texData, const wchar_t* filename, ID3D11ShaderResourceView** textureRV)
{
	HRESULT hr = S_OK;

	// 텍스처 로드 : DXTK (WIC) 사용 ★
	if (texData->sRGB)
	{
		hr = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
			D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
			0, D3D11_RESOURCE_MISC_GENERATE_MIPS, WIC_LOADER_SRGB_DEFAULT,
			nullptr, textureRV);
		if (FAILED(hr))
		{
			hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
				D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
				0, D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_FORCE_SRGB,
				nullptr, textureRV);
			if (FAILED(hr))
			{
				ERROR_MSG_HR(hr);
				return hr;

			}
		}
	}
	else
	{
		hr = DirectX::CreateWICTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
			D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
			0, D3D11_RESOURCE_MISC_GENERATE_MIPS, WIC_LOADER_DEFAULT,
			nullptr, textureRV);
		if (FAILED(hr))
		{
			hr = DirectX::CreateDDSTextureFromFileEx(m_pDevice.Get(), m_pDXDC.Get(), filename, 0,
				D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE,
				0, D3D11_RESOURCE_MISC_GENERATE_MIPS, DDS_LOADER_DEFAULT,
				nullptr, textureRV);
			if (FAILED(hr))
			{
				ERROR_MSG_HR(hr);
				return hr;

			}
		}
	}
	

	return hr;
}


HRESULT Renderer::CreateInputLayout()
{
	HRESULT hr = S_OK;

	// 정점 입력구조 Input layout
	D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,       0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,       0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,          0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TANGENT",   0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },

		{ "BONEINDICES",  0, DXGI_FORMAT_R16G16B16A16_UINT,     0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "BONEWEIGHTS",  0, DXGI_FORMAT_R32G32B32A32_FLOAT,    0, 56, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	
	UINT numElements = ARRAYSIZE(layout);

	// 정접 입력구조 객체 생성 Create the input layout
	hr = m_pDevice->CreateInputLayout(layout,
		numElements,
		m_pVSCode->GetBufferPointer(),
		m_pVSCode->GetBufferSize(),
		m_pInputLayout.GetAddressOf()
	);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	// 정점 입력구조 Input layout
	D3D11_INPUT_ELEMENT_DESC layout2[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,       0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT numElements2 = ARRAYSIZE(layout2);

	// 정접 입력구조 객체 생성 Create the input layout
	hr = m_pDevice->CreateInputLayout(layout2,
		numElements2,
		m_pVSCode_P->GetBufferPointer(),
		m_pVSCode_P->GetBufferSize(),
		m_pInputLayout_P.GetAddressOf()
	);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	return hr;
}

HRESULT Renderer::CreateConstBuffer()
{
	HRESULT hr = S_OK;

	hr = CreateDynamicConstantBuffer(m_pDevice.Get(), sizeof(BaseConstBuffer), m_pBCB.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	hr = CreateDynamicConstantBuffer(m_pDevice.Get(), sizeof(CameraConstBuffer), m_pCameraCB.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	hr = CreateDynamicConstantBuffer(m_pDevice.Get(), sizeof(SkinningConstBuffer), m_pSkinCB.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	hr = CreateDynamicConstantBuffer(m_pDevice.Get(), sizeof(LightConstBuffer), m_pLightCB.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	hr = CreateDynamicConstantBuffer(m_pDevice.Get(), sizeof(UIBuffer), m_pUIB.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	hr = CreateDynamicConstantBuffer(m_pDevice.Get(), sizeof(MaterialBuffer), m_pMatB.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	hr = CreateDynamicConstantBuffer(m_pDevice.Get(), sizeof(MaskingBuffer), m_pMaskB.GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	return hr;
}

HRESULT Renderer::CreateVertexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, UINT stride, ID3D11Buffer** ppVB)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = size;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA rd;
	ZeroMemory(&rd, sizeof(rd));
	rd.pSysMem = pData;

	ID3D11Buffer* pVB = nullptr;
	hr = m_pDevice->CreateBuffer(&bd, &rd, &pVB);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	*ppVB = pVB;

	return hr;
}

HRESULT Renderer::CreateIndexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, ID3D11Buffer** ppIB)
{
	HRESULT hr = S_OK;

	D3D11_BUFFER_DESC bd = {};

	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = size;
	bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;

	D3D11_SUBRESOURCE_DATA rd;
	ZeroMemory(&rd, sizeof(rd));
	rd.pSysMem = pData;

	ID3D11Buffer* pIB = nullptr;
	hr = m_pDevice->CreateBuffer(&bd, &rd, &pIB);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	*ppIB = pIB;

	return hr;
}

HRESULT Renderer::CreateConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB)
{
	HRESULT hr = S_OK;

	DWORD sizeAligned = AlignCBSize(size);

	D3D11_BUFFER_DESC bd = {};
	bd.Usage = D3D11_USAGE_DEFAULT;
	bd.ByteWidth = sizeAligned;
	bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;

	ID3D11Buffer* pCB = nullptr;
	hr = pDev->CreateBuffer(&bd, nullptr, &pCB);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	*ppCB = pCB;

	return hr;
}

HRESULT Renderer::RTTexCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex)
{
	//텍스처 정보 구성.
	D3D11_TEXTURE2D_DESC td = {};
	//ZeroMemory(&td, sizeof(td));
	td.Width = width;						//텍스처크기(1:1)
	td.Height = height;
	td.MipLevels = 1;						//밉맵 생성 안함
	td.ArraySize = 1;
	td.Format = fmt;							//텍스처 포멧 (DXGI_FORMAT_R8G8B8A8_UNORM 등..)
	td.SampleDesc.Count = 1;					// AA 없음.
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;		//용도 : RT + SRV
	td.CPUAccessFlags = 0;
	td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	//텍스처 생성.
	ID3D11Texture2D* pTex = NULL;
	HRESULT hr = m_pDevice->CreateTexture2D(&td, NULL, &pTex);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	//성공후 외부로 리턴.
	if (ppTex) *ppTex = pTex;

	return hr;
}

HRESULT Renderer::RTTexCreateMSAA(UINT width, UINT height, DXGI_FORMAT fmt, UINT sampleCount, UINT sampleQuality, ID3D11Texture2D** ppTex)
{
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = fmt;
	td.SampleDesc.Count = sampleCount;
	td.SampleDesc.Quality = sampleQuality;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET;
	td.CPUAccessFlags = 0;
	td.MiscFlags = 0;

	ID3D11Texture2D* pTex = NULL;
	HRESULT hr = m_pDevice->CreateTexture2D(&td, NULL, &pTex);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	if (ppTex)
	{
		*ppTex = pTex;
	}

	return hr;
}

HRESULT Renderer::RTTexCreateMipMap(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex)
{
	//텍스처 정보 구성.
	D3D11_TEXTURE2D_DESC td = {};
	//ZeroMemory(&td, sizeof(td));
	td.Width = width;						//텍스처크기(1:1)
	td.Height = height;
	td.MipLevels = 0;						//밉맵 생성 안함
	td.ArraySize = 1;
	td.Format = fmt;							//텍스처 포멧 (DXGI_FORMAT_R8G8B8A8_UNORM 등..)
	td.SampleDesc.Count = 1;					// AA 없음.
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;		//용도 : RT + SRV
	td.CPUAccessFlags = 0;
	td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

	//텍스처 생성.
	ID3D11Texture2D* pTex = NULL;
	HRESULT hr = m_pDevice->CreateTexture2D(&td, NULL, &pTex);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	//성공후 외부로 리턴.
	if (ppTex) *ppTex = pTex;

	return hr;
}

HRESULT Renderer::RTViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView)
{
	if (!pTex)
	{
		return E_INVALIDARG;
	}

	D3D11_TEXTURE2D_DESC td = {};
	pTex->GetDesc(&td);

	//렌더타겟 정보 구성.
	D3D11_RENDER_TARGET_VIEW_DESC rd = {};
	//ZeroMemory(&rd, sizeof(rd));
	rd.Format = fmt;										//텍스처와 동일포멧유지.
	rd.ViewDimension = (td.SampleDesc.Count > 1) ? D3D11_RTV_DIMENSION_TEXTURE2DMS : D3D11_RTV_DIMENSION_TEXTURE2D;		//2D RT.
	if (rd.ViewDimension == D3D11_RTV_DIMENSION_TEXTURE2D)
	{
		rd.Texture2D.MipSlice = 0;								//2D RT 용 추가 설정 : 밉멥 분할용 밉멥레벨 인덱스.
	}
	//rd.Texture2DMS.UnusedField_NothingToDefine = 0;		//2D RT + AA 용 추가 설정

	//렌더타겟 생성.
	ID3D11RenderTargetView* pRTView = NULL;
	HRESULT hr = m_pDevice->CreateRenderTargetView(pTex, &rd, &pRTView);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}



	//성공후 외부로 리턴.
	if (ppRTView) *ppRTView = pRTView;

	return hr;
}

HRESULT Renderer::RTSRViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV)
{
	if (!pTex)
	{
		return E_INVALIDARG;
	}

	D3D11_TEXTURE2D_DESC td = {};
	pTex->GetDesc(&td);

	//셰이더리소스뷰 정보 구성.
	D3D11_SHADER_RESOURCE_VIEW_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Format = fmt;										//텍스처와 동일포멧유지.
	sd.ViewDimension = (td.SampleDesc.Count > 1) ? D3D11_SRV_DIMENSION_TEXTURE2DMS : D3D11_SRV_DIMENSION_TEXTURE2D;		//2D SRV.
	if (sd.ViewDimension == D3D11_SRV_DIMENSION_TEXTURE2D)
	{
		sd.Texture2D.MipLevels = -1;								//2D SRV 추가 설정 : 밉멥 설정.
		sd.Texture2D.MostDetailedMip = 0;
	}
	//sd.Texture2DMS.UnusedField_NothingToDefine = 0;		//2D SRV+AA 추가 설정

	//셰이더리소스뷰 생성.
	ID3D11ShaderResourceView* pTexRV = NULL;
	HRESULT hr = m_pDevice->CreateShaderResourceView(pTex, &sd, &pTexRV);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	//성공후 외부로 리턴.
	if (ppTexRV) *ppTexRV = pTexRV;

	return hr;
}

HRESULT Renderer::DSCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView)
{
	HRESULT hr = S_OK;


	//---------------------------------- 
	// 깊이/스텐실 버퍼용 빈 텍스처로 만들기.	
	//---------------------------------- 
	//깊이/스텐실 버퍼 정보 구성.
	D3D11_TEXTURE2D_DESC td = {};
	//ZeroMemory(&td, sizeof(td));
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = fmt;									//원본 RT 와 동일 포멧유지.
	//td.Format  = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;	//깊이 버퍼 (32bit) + 스텐실 (8bit) / 신형 하드웨어 (DX11)
	td.SampleDesc.Count = 1;							// AA 없음.
	//td.SampleDesc.Count = g_dwAA;						// AA 설정 - RT 과 동일 규격 준수.
	//td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;			//깊이-스텐실 버퍼용으로 설정.
	td.MiscFlags = 0;
	//깊이/스텐실 버퍼용 빈 텍스처로 만들기.	
	hr = m_pDevice->CreateTexture2D(&td, NULL, pDSTex);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	//---------------------------------- 
	// 깊이/스텐실 뷰 생성.
	//---------------------------------- 
	D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
	//ZeroMemory(&dd, sizeof(dd));
	dd.Format = td.Format;
	dd.ViewDimension = (td.SampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;			//2D (AA 적용)
	if (dd.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D)
	{
		dd.Texture2D.MipSlice = 0;
	}
	//깊이/스텐실 뷰 생성.
	hr = m_pDevice->CreateDepthStencilView(*pDSTex, &dd, pDSView);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	return hr;
}


HRESULT Renderer::DSCreate(UINT width, UINT height, ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView, ID3D11ShaderResourceView** pTexRV)
{
	HRESULT hr = S_OK;


	//---------------------------------- 
	// 깊이/스텐실 버퍼용 빈 텍스처로 만들기.	
	//---------------------------------- 
	//깊이/스텐실 버퍼 정보 구성.
	D3D11_TEXTURE2D_DESC td = {};
	//ZeroMemory(&td, sizeof(td));
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_R32_TYPELESS;									//원본 RT 와 동일 포멧유지.
	//td.Format  = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;	//깊이 버퍼 (32bit) + 스텐실 (8bit) / 신형 하드웨어 (DX11)
	td.SampleDesc.Count = 1;							// AA 없음.
	//td.SampleDesc.Count = g_dwAA;						// AA 설정 - RT 과 동일 규격 준수.
	//td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;			//깊이-스텐실 버퍼용으로 설정.
	td.MiscFlags = 0;
	//깊이/스텐실 버퍼용 빈 텍스처로 만들기.	
	hr = m_pDevice->CreateTexture2D(&td, NULL, pDSTex);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	//---------------------------------- 
	// 깊이/스텐실 뷰 생성.
	//---------------------------------- 
	D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
	//ZeroMemory(&dd, sizeof(dd));
	dd.Format = DXGI_FORMAT_D32_FLOAT;
	dd.ViewDimension = (td.SampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;			//2D (AA 적용)
	if (dd.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D)
	{
		dd.Texture2D.MipSlice = 0;
	}
	//깊이/스텐실 뷰 생성.
	hr = m_pDevice->CreateDepthStencilView(*pDSTex, &dd, pDSView);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	//쉐이더 리소스 뷰 생성
	D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
	td.Format = DXGI_FORMAT_R32_TYPELESS;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	dd.Format = DXGI_FORMAT_R32_FLOAT;

	sd.Format = DXGI_FORMAT_R32_FLOAT;
	sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	sd.Texture2D.MipLevels = 1;
	sd.Texture2D.MostDetailedMip = 0;

	hr = m_pDevice->CreateShaderResourceView(*pDSTex, &sd, pTexRV);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	return hr;
}

HRESULT Renderer::DSCreateMSAA(UINT width, UINT height, DXGI_FORMAT fmt, UINT sampleCount, UINT sampleQuality,
	ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView, ID3D11ShaderResourceView** pSRV)
{
	if (!pDSTex || !pDSView || !pSRV) return E_INVALIDARG;

	// 1) MSAA Depth Texture: Typeless + SRV bind
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;

	// ★ fmt 인자는 무시하고, SRV 가능한 typeless로 강제 (Stencil 없이 depth만)
	td.Format = DXGI_FORMAT_R32_TYPELESS;

	td.SampleDesc.Count = sampleCount;
	td.SampleDesc.Quality = sampleQuality;
	td.Usage = D3D11_USAGE_DEFAULT;

	// ★ SRV까지 필요
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = m_pDevice->CreateTexture2D(&td, nullptr, pDSTex);
	if (FAILED(hr)) { ERROR_MSG_HR(hr); return hr; }

	// 2) DSV: D32_FLOAT (MSAA)
	D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
	dd.Format = DXGI_FORMAT_D32_FLOAT;
	dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;

	hr = m_pDevice->CreateDepthStencilView(*pDSTex, &dd, pDSView);
	if (FAILED(hr)) { ERROR_MSG_HR(hr); return hr; }

	// 3) SRV: R32_FLOAT (MSAA)
	D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
	sd.Format = DXGI_FORMAT_R32_FLOAT;
	sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DMS;

	hr = m_pDevice->CreateShaderResourceView(*pDSTex, &sd, pSRV);
	if (FAILED(hr)) { ERROR_MSG_HR(hr); return hr; }

	return S_OK;
}


HRESULT Renderer::RTCubeTexCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex)
{
	//텍스처 정보 구성.
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = 0;
	td.ArraySize = 6;							//ArraySize추가로 해당 텍스쳐가 6면을 가진다는 것을 의미
	td.Format = fmt;							//텍스처 포멧 (DXGI_FORMAT_R8G8B8A8_UNORM 등..)
	td.SampleDesc.Count = 1;					// AA 없음.
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	td.CPUAccessFlags = 0;
	td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS | D3D11_RESOURCE_MISC_TEXTURECUBE;	//밉맵 플래그, 텍스쳐 큐브 추가

	//텍스처 생성.
	ID3D11Texture2D* pTex = NULL;
	HRESULT hr = m_pDevice->CreateTexture2D(&td, NULL, &pTex);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	//성공후 외부로 리턴.
	if (ppTex) *ppTex = pTex;

	return hr;
}

HRESULT Renderer::CubeRTViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView, UINT faceIndex)
{
	//렌더타겟 정보 구성.
	D3D11_RENDER_TARGET_VIEW_DESC rd = {};
	//ZeroMemory(&rd, sizeof(rd));
	rd.Format = fmt;
	rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2DARRAY;		//렌더 타겟 뷰가 Texture2D를 배열로 가지고 있음
	rd.Texture2D.MipSlice = 0;
	//rd.Texture2DMS.UnusedField_NothingToDefine = 0;		
	rd.Texture2DArray.FirstArraySlice = faceIndex;				//렌더 타겟 뷰에서 이미지 배열 몇번 째에 저장할 것인지(예상)
	rd.Texture2DArray.ArraySize = 1;							//렌더 타겟 뷰의 배열 크기를 1
	//6이 아닌 이유는 렌더타겟 6개를 만들고 렌더 타겟에 그려진 이미지를 하나의 이미지에
	//데이터를 저장하기 때문에(예상)

//렌더타겟 생성.
	ID3D11RenderTargetView* pRTView = NULL;
	HRESULT hr = m_pDevice->CreateRenderTargetView(pTex, &rd, &pRTView);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}



	//성공후 외부로 리턴.
	if (ppRTView) *ppRTView = pRTView;

	return hr;

}

HRESULT Renderer::RTCubeSRViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV)
{
	//셰이더리소스뷰 정보 구성.
	D3D11_SHADER_RESOURCE_VIEW_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.Format = fmt;
	sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURECUBE;		//이 리소스는 큐브 텍스쳐임을 나타냄
	sd.Texture2D.MipLevels = -1;
	sd.Texture2D.MostDetailedMip = 0;
	//sd.Texture2DMS.UnusedField_NothingToDefine = 0;		

	//셰이더리소스뷰 생성.
	ID3D11ShaderResourceView* pTexRV = NULL;
	HRESULT hr = m_pDevice->CreateShaderResourceView(pTex, &sd, &pTexRV);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	//성공후 외부로 리턴.
	if (ppTexRV) *ppTexRV = pTexRV;

	return hr;

}

HRESULT Renderer::ResetRenderTarget(int width, int height)
{
	HRESULT hr = S_OK;

	//Unbind → Release → ResizeBuffers → Recreate → Viewport → Camera
	m_WindowSize.width = width;
	m_WindowSize.height = height;

	// 1.GPU 파이프라인 타겟 해제
	m_pDXDC->OMSetRenderTargets(0, nullptr, nullptr);
	m_pDXDC->Flush(); // 안전

	// 2.기존 화면 크기 의존 리소스 전부 Release
	ReleaseScreenSizeResource();

	// 3.SwapChain ResizeBuffers 호출
	const UINT swapChainFlags = g_bAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;
	hr = m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, swapChainFlags);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	// 4. 새 BackBuffer 획득 & RTV 재생성
	hr = CreateRenderTarget();
	if (FAILED(hr))
	{
		return hr;
	}

	// 5. DepthStencil 재생성 (AA 반영)
	hr = CreateDepthStencil(width, height);
	if (FAILED(hr))
	{
		return hr;
	}

	// 6. 화면 크기 기반 렌더 타겟 전부 재생성
	ReCreateRenderTarget();

	// 7. Viewport 재설정
	SetViewPort(width, height, m_pDXDC.Get());
	m_pDXDC->OMSetRenderTargets(1, m_pRTView.GetAddressOf(), m_pDSView.Get());
	CreateContext();

	// 8. ResizeTarget, 전체화면 전환 시, 디스플레이 모드 변경 시

	return hr;
}

void Renderer::DXSetup(HWND hWnd, int width, int height)
{
	if (m_dwAA > 1)
	{
		UINT colorQuality = 0;
		UINT depthQuality = 0;
		m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, m_dwAA, &colorQuality);
		m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, m_dwAA, &depthQuality);
		if (colorQuality == 0 || depthQuality == 0)
		{
			m_dwAA = 1;
		}
	}

	CreateDeviceSwapChain(hWnd);
	CreateRenderTarget();
	CreateDepthStencil(width, height);
	//스텐실 넣으면 바뀌어야함
	//nullptr부분에 뎁스스텐실뷰 포인터 넣기
	m_pDXDC->OMSetRenderTargets(1, m_pRTView.GetAddressOf(), m_pDSView.Get());
	SetViewPort(width, height, m_pDXDC.Get());
	CreateDepthStencilState();
	CreateSamplerState();
	CreateRasterState();
	CreateBlendState();

	CreateRenderTarget_Other();
}

HRESULT Renderer::CreateDeviceSwapChain(HWND hWnd)
{
	//HRESULT hr = S_OK;
	//DXGI_SWAP_CHAIN_DESC sd = {};
	//sd.BufferCount = 1;
	//sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	//sd.OutputWindow = hWnd;
	//sd.SampleDesc.Count = 1;
	//sd.Windowed = TRUE;

	//UINT flags = D3D11_CREATE_DEVICE_DEBUG;
	//hr = D3D11CreateDeviceAndSwapChain(
	//	nullptr,
	//	D3D_DRIVER_TYPE_HARDWARE,
	//	nullptr,
	//	D3D11_CREATE_DEVICE_DEBUG,
	//	nullptr, 0,
	//	D3D11_SDK_VERSION,
	//	&sd,
	//	m_pSwapChain.GetAddressOf(),
	//	m_pDevice.GetAddressOf(),
	//	nullptr,
	//	m_pDXDC.GetAddressOf()
	//);

	//if (FAILED(hr))
	//{
	//	ERROR_MSG(hr);
	//	return hr;
	//}

	//위는 device와 swapchain 동시 생성
	ComPtr<IDXGIDevice> dxgiDevice;
	m_pDevice->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice);

	ComPtr<IDXGIAdapter> adapter;
	dxgiDevice->GetAdapter(&adapter);

	ComPtr<IDXGIFactory> factory;
	adapter->GetParent(__uuidof(IDXGIFactory), &factory);

	ComPtr<IDXGIFactory2> factory2;
	factory.As(&factory2);


	g_bAllowTearing = FALSE;
	if (factory)
	{
		ComPtr<IDXGIFactory5> factory5;
		if (SUCCEEDED(factory.As(&factory5)))
		{
			BOOL allowTearing = FALSE;
			if (SUCCEEDED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
			{
				g_bAllowTearing = allowTearing;
			}
		}
	}

	HRESULT hr = S_OK;
	DXGI_SWAP_CHAIN_DESC1 sd = {};
	sd.Width = 0;
	sd.Height = 0;
	sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferCount = 2;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.SampleDesc.Count = 1;
	sd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	sd.Scaling = DXGI_SCALING_STRETCH;
	sd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	sd.Flags = g_bAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	hr = factory2->CreateSwapChainForHwnd(
		m_pDevice.Get(),
		hWnd,
		&sd,
		nullptr,
		nullptr,
		m_pSwapChain.GetAddressOf()
	);

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	return hr;
}

HRESULT Renderer::CreateRenderTarget()
{
	HRESULT hr = S_OK;

	ComPtr<ID3D11Texture2D> backBuffer;
	hr = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(backBuffer.GetAddressOf()));

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	hr = m_pDevice->CreateRenderTargetView(backBuffer.Get(), nullptr, m_pRTView.GetAddressOf());

	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}



	return hr;

}

HRESULT Renderer::CreateRenderTarget_Other()
{
	HRESULT hr = S_OK;
	ReCreateRenderTarget();

#pragma region ShadowMap
	m_ShadowTextureSize = { 4096, 4096 };
	DSCreate(m_ShadowTextureSize.width, m_ShadowTextureSize.height, m_pDSTex_Shadow.GetAddressOf(), m_pDSViewScene_Shadow.GetAddressOf(), m_pShadowRV.GetAddressOf());

	//DSCreateShadowMSAA(m_ShadowTextureSize.width, m_ShadowTextureSize.height, m_pDSTex_ShadowMSAA.GetAddressOf(), m_pDSViewScene_ShadowMSAA.GetAddressOf());
#pragma endregion


	return hr;
}

HRESULT Renderer::ReCreateRenderTarget()
{
	HRESULT hr = S_OK;
#pragma region Imgui RenderTarget
	DXGI_FORMAT fmt = kSceneColorFormat;

	//1. 렌더 타겟용 빈 텍스처로 만들기.	
	if (m_dwAA > 1)
	{
		RTTexCreateMSAA(m_WindowSize.width, m_WindowSize.height, fmt, m_dwAA, 0, m_pRTScene_ImguiMSAA.GetAddressOf());
		RTTexCreateMSAA(m_WindowSize.width, m_WindowSize.height, fmt, m_dwAA, 0, m_pRTScene_Imgui_editMSAA.GetAddressOf());
	}
	RTTexCreate(m_WindowSize.width, m_WindowSize.height, fmt, m_pRTScene_Imgui.GetAddressOf());
	RTTexCreate(m_WindowSize.width, m_WindowSize.height, fmt, m_pRTScene_Imgui_edit.GetAddressOf());

	//2. 렌더타겟뷰 생성.
	if (m_dwAA > 1)
	{
		RTViewCreate(fmt, m_pRTScene_ImguiMSAA.Get(), m_pRTView_Imgui.GetAddressOf());
		RTViewCreate(fmt, m_pRTScene_Imgui_editMSAA.Get(), m_pRTView_Imgui_edit.GetAddressOf());
	}
	else
	{
		RTViewCreate(fmt, m_pRTScene_Imgui.Get(), m_pRTView_Imgui.GetAddressOf());
		RTViewCreate(fmt, m_pRTScene_Imgui_edit.Get(), m_pRTView_Imgui_edit.GetAddressOf());
	}

	//3. 렌더타겟 셰이더 리소스뷰 생성 (멥핑용)
	RTSRViewCreate(fmt, m_pRTScene_Imgui.Get(), m_pTexRvScene_Imgui.GetAddressOf());
	RTSRViewCreate(fmt, m_pRTScene_Imgui_edit.Get(), m_pTexRvScene_Imgui_edit.GetAddressOf());

	DXGI_FORMAT dsFmt = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;		//원본 DS 포멧 유지.
	DSCreate(m_WindowSize.width, m_WindowSize.height, dsFmt, m_pDSTex_Imgui.GetAddressOf(), m_pDSViewScene_Imgui.GetAddressOf());
	DSCreate(m_WindowSize.width, m_WindowSize.height, dsFmt, m_pDSTex_Imgui_edit.GetAddressOf(), m_pDSViewScene_Imgui_edit.GetAddressOf());
#pragma endregion


#pragma region Depth
	DSCreate(m_WindowSize.width, m_WindowSize.height, m_pDSTex_Depth.GetAddressOf(), m_pDSViewScene_Depth.GetAddressOf(), m_pDepthRV.GetAddressOf());

	if (m_dwAA > 1)
	{
		DSCreateMSAA(m_WindowSize.width, m_WindowSize.height,
			DXGI_FORMAT_D32_FLOAT,   // 의미 없어도 맞춰두기
			m_dwAA, 0,
			m_pDSTex_DepthMSAA.GetAddressOf(),
			m_pDSViewScene_DepthMSAA.GetAddressOf(),
			m_pDepthMSAARV.GetAddressOf());
	}
#pragma endregion


#pragma region Post
	fmt = kSceneColorFormat;
	RTTexCreate(m_WindowSize.width, m_WindowSize.height, fmt, m_pRTScene_Post.GetAddressOf());

	//2. 렌더타겟뷰 생성.
	RTViewCreate(fmt, m_pRTScene_Post.Get(), m_pRTView_Post.GetAddressOf());

	//3. 렌더타겟 셰이더 리소스뷰 생성 (멥핑용)
	RTSRViewCreate(fmt, m_pRTScene_Post.Get(), m_pTexRvScene_Post.GetAddressOf());

#pragma endregion

#pragma region Blur
	fmt = kSceneColorFormat;
	RTTexCreate(m_WindowSize.width, m_WindowSize.height, fmt, m_pRTScene_BlurOrigin.GetAddressOf());

	//2. 렌더타겟뷰 생성.
	if (m_dwAA > 1)
	{
		RTTexCreateMSAA(m_WindowSize.width, m_WindowSize.height, fmt, m_dwAA, 0, m_pRTScene_BlurOriginMSAA.GetAddressOf());
		RTViewCreate(fmt, m_pRTScene_BlurOriginMSAA.Get(), m_pRTView_BlurOrigin.GetAddressOf());

	}
	else
	{
		RTViewCreate(fmt, m_pRTScene_BlurOrigin.Get(), m_pRTView_BlurOrigin .GetAddressOf());

	}

	//3. 렌더타겟 셰이더 리소스뷰 생성 (멥핑용)
	RTSRViewCreate(fmt, m_pRTScene_BlurOrigin.Get(), m_pTexRvScene_BlurOrigin.GetAddressOf());

	UINT width = m_WindowSize.width / 2;
	UINT height = m_WindowSize.height / 2;

	for (int i = 0; i < static_cast<int>(BlurLevel::COUNT); i++)
	{
		RTTexCreate(width, height, fmt, m_pRTScene_Blur[i].GetAddressOf());


		RTViewCreate(fmt,
			m_pRTScene_Blur[i].Get(),
			m_pRTView_Blur[i].GetAddressOf());


		RTSRViewCreate(fmt,
			m_pRTScene_Blur[i].Get(),
			m_pTexRvScene_Blur[i].GetAddressOf());


		width /= 2;
		height /= 2;
	}

#pragma endregion

#pragma region Refraction
	fmt = kSceneColorFormat;

	RTTexCreate(m_WindowSize.width, m_WindowSize.height, fmt, m_pRTScene_Refraction.GetAddressOf());

	if (m_dwAA > 1)
	{
		RTTexCreateMSAA(m_WindowSize.width, m_WindowSize.height, fmt, m_dwAA, 0, m_pRTScene_RefractionMSAA.GetAddressOf());
		RTViewCreate(fmt, m_pRTScene_RefractionMSAA.Get(), m_pRTView_Refraction.GetAddressOf());

	}
	else
	{
		RTViewCreate(fmt, m_pRTScene_Refraction.Get(), m_pRTView_Refraction.GetAddressOf());

	}
	RTSRViewCreate(fmt, m_pRTScene_Refraction.Get(), m_pTexRvScene_Refraction.GetAddressOf());
#pragma endregion

#pragma region Emissive
	fmt = kSceneColorFormat;

	RTTexCreate(m_WindowSize.width, m_WindowSize.height, fmt, m_pRTScene_EmissiveOrigin.GetAddressOf());

	if (m_dwAA > 1)
	{
		RTTexCreateMSAA(m_WindowSize.width, m_WindowSize.height, fmt, m_dwAA, 0, m_pRTScene_EmissiveOriginMSAA.GetAddressOf());
		RTViewCreate(fmt, m_pRTScene_EmissiveOriginMSAA.Get(), m_pRTView_EmissiveOrigin.GetAddressOf());
	}
	else
	{
		RTViewCreate(fmt, m_pRTScene_EmissiveOrigin.Get(), m_pRTView_EmissiveOrigin.GetAddressOf());
	}

	RTSRViewCreate(fmt, m_pRTScene_EmissiveOrigin.Get(), m_pTexRvScene_EmissiveOrigin.GetAddressOf());


	width = m_WindowSize.width / 2;
	height = m_WindowSize.height / 2;
	for (int i = 0; i < static_cast<int>(EmissiveLevel::COUNT); i++)
	{
		RTTexCreate(width, height, fmt, m_pRTScene_Emissive[i].GetAddressOf());


		RTViewCreate(fmt,
			m_pRTScene_Emissive[i].Get(),
			m_pRTView_Emissive[i].GetAddressOf());


		RTSRViewCreate(fmt,
			m_pRTScene_Emissive[i].Get(),
			m_pTexRvScene_Emissive[i].GetAddressOf());


		width /= 2;
		height /= 2;
	}
#pragma endregion



	return hr;
}

void Renderer::RecreateForAASampleChange(int width, int height, DWORD sampleCount)
{
	if (sampleCount > 1)
	{
		UINT colorQuality = 0;
		UINT depthQuality = 0;
		m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, sampleCount, &colorQuality);
		m_pDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_D32_FLOAT_S8X24_UINT, sampleCount, &depthQuality);
		if (colorQuality == 0 || depthQuality == 0)
		{
			sampleCount = 1;
		}
	}

	m_dwAA = sampleCount;

	// 1. GPU 파이프라인 타겟 해제
	m_pDXDC->OMSetRenderTargets(0, nullptr, nullptr);
	m_pDXDC->Flush();

	// 2. 기존 화면 크기 의존 리소스 전부 Release
	ReleaseScreenSizeResource();

	// 3. SwapChain ResizeBuffers 호출 (샘플 수 변경 시 백버퍼 재생성)
	const UINT swapChainFlags = g_bAllowTearing ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0u;
	m_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, swapChainFlags);

	// 4. 새 BackBuffer 획득 & RTV 재생성
	CreateRenderTarget();

	// 5. DepthStencil 재생성 (m_dwAA 반영)
	CreateDepthStencil(width, height);

	// 6. 화면 크기 기반 렌더 타겟 전부 재생성
	ReCreateRenderTarget();

	// 7. Viewport 및 기본 렌더 타겟 재설정
	SetViewPort(width, height, m_pDXDC.Get());
	m_pDXDC->OMSetRenderTargets(1, m_pRTView.GetAddressOf(), m_pDSView.Get());

	// 8. RenderContext 포인터 갱신
	CreateContext();
}

HRESULT Renderer::CreateDepthStencil(int width, int height)
{
	HRESULT hr = S_OK;

	D3D11_TEXTURE2D_DESC   td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
	td.SampleDesc.Count = m_dwAA;
	td.SampleDesc.Quality = 0;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	td.CPUAccessFlags = 0;
	td.MiscFlags = 0;
	hr = m_pDevice->CreateTexture2D(&td, NULL, &m_pDS);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	D3D11_DEPTH_STENCIL_VIEW_DESC  dd = {};
	dd.Format = td.Format;
	dd.ViewDimension = (td.SampleDesc.Count > 1) ? D3D11_DSV_DIMENSION_TEXTURE2DMS : D3D11_DSV_DIMENSION_TEXTURE2D;		//AA 설정 "MSAA"
	if (dd.ViewDimension == D3D11_DSV_DIMENSION_TEXTURE2D)
	{
		dd.Texture2D.MipSlice = 0;
	}

	hr = m_pDevice->CreateDepthStencilView(m_pDS.Get(), &dd, &m_pDSView);
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	return hr;
}

HRESULT Renderer::CreateDepthStencilState()
{
	HRESULT hr;

	D3D11_DEPTH_STENCIL_DESC  ds;
	ds.DepthEnable = TRUE;
	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	ds.DepthFunc = D3D11_COMPARISON_LESS;
	ds.StencilEnable = FALSE;
	ds.DepthEnable = TRUE;
	ds.StencilEnable = FALSE;
	hr = m_pDevice->CreateDepthStencilState(&ds, m_DSState[DS::DEPTH_ON].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	hr = m_pDevice->CreateDepthStencilState(&ds, m_DSState[DS::DEPTH_ON_WRITE_OFF].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	ds.DepthEnable = FALSE;
	hr = m_pDevice->CreateDepthStencilState(&ds, m_DSState[DS::DEPTH_OFF].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	return hr;
}

HRESULT Renderer::CreateRasterState()
{
	HRESULT hr;

	//[상태객체 1] 기본 렌더링 상태 개체.
	D3D11_RASTERIZER_DESC rd;
	rd.FillMode = D3D11_FILL_SOLID;		//삼각형 색상 채우기.(기본값)
	rd.CullMode = D3D11_CULL_NONE;		//컬링 없음. (기본값은 컬링 Back)		
	rd.FrontCounterClockwise = false;   //이하 기본값...
	rd.DepthBias = 0;
	rd.DepthBiasClamp = 0;
	rd.SlopeScaledDepthBias = 0;
	rd.DepthClipEnable = true;
	rd.ScissorEnable = false;
	rd.MultisampleEnable = true;
	rd.AntialiasedLineEnable = false;
	//레스터라이져 상태 객체 생성.
	hr = m_pDevice->CreateRasterizerState(&rd, m_RState[RS::SOLID].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	//[상태객체2] 와이어 프레임 그리기. 
	rd.FillMode = D3D11_FILL_WIREFRAME;
	rd.CullMode = D3D11_CULL_NONE;
	//레스터라이져 객체 생성.
	hr = m_pDevice->CreateRasterizerState(&rd, m_RState[RS::WIREFRM].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	//[상태객체3] 컬링 On! "CCW"
	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_BACK;
	hr = m_pDevice->CreateRasterizerState(&rd, m_RState[RS::CULLBACK].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	//[상태객체4] 와이어 프레임 + 컬링 On! "CCW"
	rd.FillMode = D3D11_FILL_WIREFRAME;
	rd.CullMode = D3D11_CULL_BACK;
	hr = m_pDevice->CreateRasterizerState(&rd, m_RState[RS::WIRECULLBACK].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	rd.FillMode = D3D11_FILL_SOLID;
	rd.CullMode = D3D11_CULL_BACK;
	rd.DepthBias = -1;                 // 앞쪽으로
	rd.SlopeScaledDepthBias = -1.0f;   // 기울기 보정
	rd.DepthBiasClamp = 0.0f;

	hr = m_pDevice->CreateRasterizerState(&rd, m_RState[RS::EMISSIVE].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}
	
	return hr;
}

HRESULT Renderer::CreateSamplerState()
{
	HRESULT hr;
	//셈플러 정보 설정.(이하 기본값)
	D3D11_SAMPLER_DESC sd = {};
	//ZeroMemory(&sd, sizeof(D3D11_SAMPLER_DESC));
	sd.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.Filter = D3D11_FILTER_ANISOTROPIC;
	sd.MaxAnisotropy = 8;
	sd.MinLOD = 0;
	sd.MaxLOD = D3D11_FLOAT32_MAX;
	sd.MipLODBias = 0;
	sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sd.BorderColor[0] = 0;
	sd.BorderColor[1] = 0;
	sd.BorderColor[2] = 0;
	sd.BorderColor[3] = 1;

	hr = m_pDevice->CreateSamplerState(&sd, m_SState[SS::WRAP].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	sd.AddressU = D3D11_TEXTURE_ADDRESS_MIRROR;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_MIRROR;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_MIRROR;
	hr = m_pDevice->CreateSamplerState(&sd, m_SState[SS::MIRROR].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	sd.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	hr = m_pDevice->CreateSamplerState(&sd, m_SState[SS::CLAMP].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	hr = m_pDevice->CreateSamplerState(&sd, m_SState[SS::BORDER].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	sd.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	sd.BorderColor[0] = 1;
	sd.BorderColor[1] = 1;
	sd.BorderColor[2] = 1;
	sd.BorderColor[3] = 1;

	hr = m_pDevice->CreateSamplerState(&sd, m_SState[SS::BORDER_SHADOW].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	return hr;
}

HRESULT Renderer::CreateBlendState()
{
	HRESULT hr;

	D3D11_BLEND_DESC bd = {};
	D3D11_RENDER_TARGET_BLEND_DESC rtb = {};

	rtb.BlendEnable = FALSE;
	rtb.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	bd.RenderTarget[0] = rtb;
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;
	hr = m_pDevice->CreateBlendState(&bd, m_BState[BS::DEFAULT].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	rtb = {};
	rtb.BlendEnable = TRUE;
	rtb.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtb.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtb.BlendOp = D3D11_BLEND_OP_ADD;
	rtb.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtb.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtb.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtb.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	bd = {};
	bd.RenderTarget[0] = rtb;
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;

	hr = m_pDevice->CreateBlendState(&bd, m_BState[BS::ALPHABLEND].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	rtb = {};
	rtb.BlendEnable = TRUE;
	rtb.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtb.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	rtb.BlendOp = D3D11_BLEND_OP_ADD;
	rtb.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtb.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtb.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtb.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	bd = {};
	bd.RenderTarget[0] = rtb;
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;

	hr = m_pDevice->CreateBlendState(&bd, m_BState[BS::ALPHABLEND_WALL].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	rtb = {};
	rtb.BlendEnable = TRUE;
	rtb.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	rtb.DestBlend = D3D11_BLEND_ONE;
	rtb.BlendOp = D3D11_BLEND_OP_ADD;
	rtb.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtb.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtb.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtb.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	bd = {};
	bd.RenderTarget[0] = rtb;
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;

	hr = m_pDevice->CreateBlendState(&bd, m_BState[BS::ADD].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}


	rtb = {};
	rtb.BlendEnable = TRUE;
	rtb.SrcBlend = D3D11_BLEND_DEST_COLOR;
	rtb.DestBlend = D3D11_BLEND_ZERO;
	rtb.BlendOp = D3D11_BLEND_OP_ADD;
	rtb.SrcBlendAlpha = D3D11_BLEND_ONE;
	rtb.DestBlendAlpha = D3D11_BLEND_ZERO;
	rtb.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	rtb.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	bd = {};
	bd.RenderTarget[0] = rtb;
	bd.AlphaToCoverageEnable = FALSE;
	bd.IndependentBlendEnable = FALSE;

	hr = m_pDevice->CreateBlendState(&bd, m_BState[BS::MULTIPLY].GetAddressOf());
	if (FAILED(hr))
	{
		ERROR_MSG_HR(hr);
		return hr;
	}

	return hr;
}

HRESULT Renderer::ReleaseScreenSizeResource()
{
	HRESULT hr = S_OK;

	m_pRTView.Reset();
	m_pDS.Reset();
	m_pDSView.Reset();
	m_pRTScene_Imgui.Reset();
	m_pRTScene_ImguiMSAA.Reset();
	m_pTexRvScene_Imgui.Reset();
	m_pRTView_Imgui.Reset();
	m_pDSTex_Imgui.Reset();
	m_pDSViewScene_Imgui.Reset();
	m_pRTScene_Imgui_edit.Reset();
	m_pRTScene_Imgui_editMSAA.Reset();
	m_pTexRvScene_Imgui_edit.Reset();
	m_pRTView_Imgui_edit.Reset();
	m_pDSTex_Imgui_edit.Reset();
	m_pDSViewScene_Imgui_edit.Reset();
	m_pDSTex_Depth.Reset();
	m_pDSViewScene_Depth.Reset();
	m_pDSTex_DepthMSAA.Reset();
	m_pDSViewScene_DepthMSAA.Reset();
	m_pRTScene_Post.Reset();
	m_pTexRvScene_Post.Reset();
	m_pRTView_Post.Reset();
	m_pRTScene_EmissiveOrigin.Reset();
	m_pRTScene_EmissiveOriginMSAA.Reset();
	m_pTexRvScene_EmissiveOrigin.Reset();
	m_pRTView_EmissiveOrigin.Reset();

	return hr;
}

void Renderer::CreateGridVB()
{
	const int   N = m_HalfCells;
	const float s = m_CellSize;
	const float half = N * s;

	std::vector<RenderData::Vertex> v;
	v.reserve((N * 2 + 1) * 4);

	XMFLOAT3 cMajor(1, 1, 1), cMinor(0.7f, 0.7f, 0.7f);
	XMFLOAT3 cAxisX(0.8f, 0.2f, 0.2f), cAxisZ(0.2f, 0.4f, 0.8f);

	for (int i = -N; i <= N; ++i) 
	{ 
		float x = i * s;
		//XMFLOAT3 col = (i == 0) ? cAxisZ : ((i % 5 == 0) ? cMajor : cMinor);
		v.push_back({ XMFLOAT3(-half, 0.01f, x)});
		v.push_back({ XMFLOAT3(+half, 0.01f, x)});

		float z = i * s;
		//col = (i == 0) ? cAxisX : ((i % 5 == 0) ? cMajor : cMinor);
		v.push_back({ XMFLOAT3(z, 0.01f, -half) });
		v.push_back({ XMFLOAT3(z, 0.01f, +half)});
	}

	m_GridVertexCount = (UINT)v.size();

	D3D11_BUFFER_DESC bd{};
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bd.ByteWidth = UINT(v.size() * sizeof(RenderData::Vertex));
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	D3D11_SUBRESOURCE_DATA sd{ v.data(), 0, 0 };
	m_pDevice->CreateBuffer(&bd, &sd, m_GridVB.GetAddressOf());
}

void Renderer::UpdateGrid(const RenderData::FrameData& frame)
{
	const auto& context = frame.context;

	XMFLOAT3   eye = context.gameCamera.cameraPos;
	XMFLOAT4X4 view = context.gameCamera.view;
	XMFLOAT4X4 proj = context.gameCamera.proj;
	XMFLOAT4X4 vp = context.gameCamera.viewProj;


	XMFLOAT4X4 tm;

	XMMATRIX mView = XMLoadFloat4x4(&view);

	if (m_IsEditCam)
	{
		mView = XMLoadFloat4x4(&context.editorCamera.view);
		eye = context.editorCamera.cameraPos;
	}

	//XMMATRIX mProj = XMMatrixPerspectiveFovLH(Fov, 960.f / 800.f, 1, 300);
	XMMATRIX mProj = XMLoadFloat4x4(&proj);
	if (m_IsEditCam)
		mProj = XMLoadFloat4x4(&context.editorCamera.proj);

	XMStoreFloat4x4(&tm, mView);
	m_RenderContext.CameraCBuffer.mView = tm;


	XMStoreFloat4x4(&tm, mProj);
	m_RenderContext.CameraCBuffer.mProj = tm;

	XMStoreFloat4x4(&tm, mView * mProj);
	m_RenderContext.CameraCBuffer.mVP = tm;

	m_RenderContext.CameraCBuffer.camPos = eye;

	XMFLOAT4X4 wTM;
	const float cellSize = m_CellSize;
	const float snapX = (cellSize > 0.0f) ? std::floor(eye.x / cellSize) * cellSize : 0.0f;
	const float snapZ = (cellSize > 0.0f) ? std::floor(eye.z / cellSize) * cellSize : 0.0f;
	XMStoreFloat4x4(&wTM, XMMatrixTranslation(snapX, 0.0f, snapZ));
	m_RenderContext.BCBuffer.mWorld = wTM;

	UpdateDynamicBuffer(m_pDXDC.Get(), m_RenderContext.pBCB.Get(), &m_RenderContext.BCBuffer, sizeof(BaseConstBuffer));
	UpdateDynamicBuffer(m_pDXDC.Get(), m_RenderContext.pCameraCB.Get(), &m_RenderContext.CameraCBuffer, sizeof(CameraConstBuffer));

}

void Renderer::DrawGrid()
{
	m_pDXDC->IASetInputLayout(m_pInputLayout.Get());
	m_pDXDC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
	UINT strideC = sizeof(RenderData::Vertex), offsetC = 0;
	m_pDXDC->IASetVertexBuffers(0, 1, m_GridVB.GetAddressOf(), &strideC, &offsetC);
	m_pDXDC->VSSetShader(m_pVS_P.Get(), nullptr, 0);
	m_pDXDC->PSSetShader(m_pPS_P.Get(), nullptr, 0);
	m_pDXDC->VSSetConstantBuffers(0, 1, m_RenderContext.pBCB.GetAddressOf());
	m_pDXDC->VSSetConstantBuffers(1, 1, m_RenderContext.pCameraCB.GetAddressOf());

	m_pDXDC->Draw(m_GridVertexCount, 0);

}

void Renderer::CreateQuadVB()
{
	CreateVertexBuffer(m_pDevice.Get(), quadVertices.data(), static_cast<UINT>(quadVertices.size() * sizeof(RenderData::Vertex)), sizeof(RenderData::Vertex), m_QuadVertexBuffers.GetAddressOf());
}

void Renderer::CreateQuadIB()
{
	m_QuadIndexCounts = static_cast<UINT>(quadIndices.size());
	CreateIndexBuffer(m_pDevice.Get(), quadIndices.data(), static_cast<UINT>(quadIndices.size() * sizeof(UINT)), m_QuadIndexBuffers.GetAddressOf());
}

void Renderer::CreateUIWhiteTexture()
{
	if (!m_pDevice)
	{
		return;
	}

	const UINT32 whitePixel = 0xFFFFFFFF;
	D3D11_TEXTURE2D_DESC desc{};
	desc.Width = 1;
	desc.Height = 1;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.Usage = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	D3D11_SUBRESOURCE_DATA data{};
	data.pSysMem = &whitePixel;
	data.SysMemPitch = sizeof(UINT32);

	ComPtr<ID3D11Texture2D> texture;
	if (FAILED(m_pDevice->CreateTexture2D(&desc, &data, texture.GetAddressOf())))
	{
		return;
	}

	m_pDevice->CreateShaderResourceView(texture.Get(), nullptr, m_UIWhiteTexture.GetAddressOf());
}

void Renderer::SetupText()
{
	ComPtr<ID3D11DeviceContext1> dc1;
	m_pDXDC.As(&dc1);

	m_SpriteBatch = std::make_unique<DirectX::SpriteBatch>(dc1.Get());
	m_SpriteFont = std::make_unique<DirectX::SpriteFont>(m_pDevice.Get(), L"../Font/hangulfont.spritefont");
}

//핸들 개수 최대값 가져오는 함수
UINT32 GetMaxMeshHandleId(const RenderData::FrameData& frame)
{
	UINT32 maxId = 0;
	for (const auto& [layer, items] : frame.renderItems)
	{
		for (const auto& item : items)
		{
			if (item.mesh.IsValid() && item.mesh.id > maxId)
			{
				maxId = item.mesh.id;
			}
		}
	}

	return maxId;
}