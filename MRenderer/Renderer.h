#pragma once

#include "DX11.h"
#include <unordered_map>
#include "Buffers.h"
#include "RenderPipeline.h"
#include "RenderTargetContext.h"

// 렌더링 진입점 클래스
// RenderPipeline을 관리한다.
// 
class Renderer
{
public:
	Renderer(AssetLoader& assetloader) : m_AssetLoader(assetloader) {}
	RenderPipeline& GetPipeline() { return m_Pipeline; }
	const RenderPipeline& GetPipeline() const { return m_Pipeline; }

	void Initialize(HWND hWnd, int width, int height, ID3D11Device* device, ID3D11DeviceContext* dxdc);
	void InitializeTest(HWND hWnd, int width, int height, ID3D11Device* device, ID3D11DeviceContext* dxdc);		//Editor의 Renderer 초기화
	void RenderFrame(const RenderData::FrameData& frame);
	void RenderFrame(const RenderData::FrameData& frame, RenderTargetContext& rendertargetcontext, RenderTargetContext& rendertargetcontext2);
	void RenderToBackBuffer();
	void InitVB(const RenderData::FrameData& frame);
	void InitIB(const RenderData::FrameData& frame);
	void EnsureMeshBuffers(const RenderData::FrameData& frame);
	void InitTexture();
	void InitShaders();

	ComPtr<ID3D11RenderTargetView>	GetRTView() { return m_pRTView; }
	ComPtr<IDXGISwapChain>			GetSwapChain() { return m_pSwapChain; }

//DX Set
public :
	HRESULT ResetRenderTarget(int width, int height);			//화면크기 바꿨을 때 렌더타겟도 그에 맞게 초기화, 테스트 아직 못함
private:
	void	DXSetup(HWND hWnd, int width, int height);
	HRESULT CreateDeviceSwapChain(HWND hWnd);
	HRESULT CreateRenderTarget();
	HRESULT CreateRenderTarget_Other();
	HRESULT ReCreateRenderTarget();							//스크린에 영향받는 것들
	void RecreateForAASampleChange(int width, int height, DWORD sampleCount);			//코덱스로 생성함 코드 검토 및 테스트 필요
	HRESULT CreateDepthStencil(int width, int height);
	HRESULT	CreateDepthStencilState();
	HRESULT	CreateRasterState();
	HRESULT	CreateSamplerState();
	HRESULT CreateBlendState();
	HRESULT ReleaseScreenSizeResource();						//화면 크기 영향 받는 리소스 해제

	DWORD m_dwAA = 4;				//안티에일리어싱, 1: 안함, 이후 수: 샘플 개수

	ComPtr<ID3D11Device>				m_pDevice;
	ComPtr<ID3D11DeviceContext>			m_pDXDC;
	ComPtr<IDXGISwapChain1>				m_pSwapChain;
	ComPtr<ID3D11RenderTargetView>		m_pRTView;
	ComPtr<ID3D11Texture2D>				m_pDS;
	ComPtr<ID3D11DepthStencilView>		m_pDSView;



	//imgui용 == Scene Draw용
	bool m_IsEditCam = false;
	ComPtr<ID3D11Texture2D>				m_pRTScene_Imgui;
	ComPtr<ID3D11Texture2D>				m_pRTScene_ImguiMSAA;
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_Imgui;
	ComPtr<ID3D11RenderTargetView>		m_pRTView_Imgui;

	ComPtr<ID3D11Texture2D>				m_pDSTex_Imgui;
	ComPtr<ID3D11DepthStencilView>		m_pDSViewScene_Imgui;

	ComPtr<ID3D11Texture2D>				m_pRTScene_Imgui_edit;
	ComPtr<ID3D11Texture2D>				m_pRTScene_Imgui_editMSAA;
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_Imgui_edit;
	ComPtr<ID3D11RenderTargetView>		m_pRTView_Imgui_edit;

	ComPtr<ID3D11Texture2D>				m_pDSTex_Imgui_edit;
	ComPtr<ID3D11DepthStencilView>		m_pDSViewScene_Imgui_edit;

	//그림자 매핑용
	ComPtr<ID3D11Texture2D>				m_pDSTex_Shadow;
	ComPtr<ID3D11DepthStencilView>		m_pDSViewScene_Shadow;
	ComPtr<ID3D11ShaderResourceView>	m_pShadowRV;
	TextureSize							m_ShadowTextureSize = { 0,0 };

	//DepthPass용
	ComPtr<ID3D11Texture2D>				m_pDSTex_Depth;
	ComPtr<ID3D11DepthStencilView>		m_pDSViewScene_Depth;
	ComPtr<ID3D11ShaderResourceView>	m_pDepthRV;
	ComPtr<ID3D11Texture2D>				m_pDSTex_DepthMSAA;
	ComPtr<ID3D11DepthStencilView>		m_pDSViewScene_DepthMSAA;
	ComPtr<ID3D11ShaderResourceView>	m_pDepthMSAARV;


	//PostPass용
	ComPtr<ID3D11Texture2D>				m_pRTScene_Post;
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_Post;
	ComPtr<ID3D11RenderTargetView>		m_pRTView_Post;

	//BlurPass용
	ComPtr<ID3D11Texture2D>				m_pRTScene_BlurOrigin;
	ComPtr<ID3D11Texture2D>				m_pRTScene_BlurOriginMSAA;
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_BlurOrigin;
	ComPtr<ID3D11RenderTargetView>		m_pRTView_BlurOrigin;

	ComPtr<ID3D11Texture2D>				m_pRTScene_Blur[static_cast<UINT>(BlurLevel::COUNT)];
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_Blur[static_cast<UINT>(BlurLevel::COUNT)];
	ComPtr<ID3D11RenderTargetView>		m_pRTView_Blur[static_cast<UINT>(BlurLevel::COUNT)];


	//Refraction용
	ComPtr<ID3D11Texture2D>				m_pRTScene_Refraction;
	ComPtr<ID3D11Texture2D>				m_pRTScene_RefractionMSAA;
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_Refraction;
	ComPtr<ID3D11RenderTargetView>		m_pRTView_Refraction;

	//Emissive용
	ComPtr<ID3D11Texture2D>				m_pRTScene_EmissiveOrigin;
	ComPtr<ID3D11Texture2D>				m_pRTScene_EmissiveOriginMSAA;
	ComPtr<ID3D11ShaderResourceView>	m_pTexRvScene_EmissiveOrigin;
	ComPtr<ID3D11RenderTargetView>		m_pRTView_EmissiveOrigin;

	ComPtr<ID3D11Texture2D>              m_pRTScene_Emissive[static_cast<UINT>(EmissiveLevel::COUNT)];
	ComPtr<ID3D11ShaderResourceView>     m_pTexRvScene_Emissive[static_cast<UINT>(EmissiveLevel::COUNT)];
	ComPtr<ID3D11RenderTargetView>       m_pRTView_Emissive[static_cast<UINT>(EmissiveLevel::COUNT)];

	//HDRIImage
	ComPtr<ID3D11ShaderResourceView>	m_pHDRI_1;
	ComPtr<ID3D11ShaderResourceView>	m_pHDRI_2;



	EnumArray<ComPtr<ID3D11DepthStencilState>, static_cast<size_t>(DS::MAX_)>	m_DSState;		//깊이 스텐실 상태
	EnumArray<ComPtr<ID3D11RasterizerState>, static_cast<size_t>(RS::MAX_)>		m_RState;		//래스터라이저 상태
	EnumArray<ComPtr<ID3D11SamplerState>, static_cast<size_t>(SS::MAX_)>		m_SState;		//샘플러 상태
	EnumArray<ComPtr<ID3D11BlendState>, static_cast<size_t>(BS::MAX_)>			m_BState;		//블렌딩 상태

private:
	void CreateContext();
	void ResolveImguiEditTargetIfNeeded();		//msaa로 그린 씬 결과를 외부에서 쓰도록 변환하는 함수

	//DXHelper
	HRESULT Compile(const WCHAR* FileName, const char* EntryPoint, const char* ShaderModel, ID3DBlob** ppCode);
	HRESULT LoadVertexShader(const TCHAR* filename, ID3D11VertexShader** ppVS, ID3DBlob** ppVSCode);
	HRESULT LoadPixelShader(const TCHAR* filename, ID3D11PixelShader** ppPS);
	HRESULT LoadVertexShaderCSO(const TCHAR* filename, ID3D11VertexShader** ppVS, ID3DBlob** ppVSCode);
	HRESULT LoadPixelShaderCSO(const TCHAR* filename, ID3D11PixelShader** ppPS);
	HRESULT TexturesLoad(const RenderData::TextureData* texData, const wchar_t* filename, ID3D11ShaderResourceView** textureRV);
	HRESULT CreateInputLayout();			//일단 하나만, 나중에 레이아웃 추가되면 함수를 추가하든 여기서 추가하든 하면될듯
	HRESULT CreateConstBuffer();
	HRESULT CreateVertexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, UINT stride, ID3D11Buffer** ppVB);
	HRESULT CreateIndexBuffer(ID3D11Device* pDev, LPVOID pData, UINT size, ID3D11Buffer** ppIB);
	HRESULT CreateConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB);
	HRESULT RTTexCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex);
	HRESULT RTTexCreateMSAA(UINT width, UINT height, DXGI_FORMAT fmt, UINT sampleCount, UINT sampleQuality, ID3D11Texture2D** ppTex);
	HRESULT RTTexCreateMipMap(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex);
	HRESULT RTViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView);
	HRESULT RTSRViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV);
	HRESULT DSCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView);				//일반 DS용
	HRESULT DSCreate(UINT width, UINT height, ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView, ID3D11ShaderResourceView** pTexRV);		//쉐이더리소스뷰 생성 DS
	HRESULT DSCreateMSAA(UINT width, UINT height, DXGI_FORMAT fmt, UINT sampleCount, UINT sampleQuality, ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView, ID3D11ShaderResourceView** pSRV);
	HRESULT RTCubeTexCreate(UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex);
	HRESULT CubeRTViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView, UINT faceIndex);
	HRESULT RTCubeSRViewCreate(DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV);
private:
	bool m_bIsInitialized = false;

	TextureSize m_WindowSize = { 0,0 };

	RenderPipeline m_Pipeline;
	AssetLoader& m_AssetLoader;


	//렌더링에 필요한 요소들 공유를 위한 소통 창구 
	RenderContext m_RenderContext;

	//버텍스버퍼들
	// ResourceHandle -> id, generation
	//※ map으로 관리 or 다른 방식 사용. notion issue 참조※	해결
	std::unordered_map<MeshHandle, ComPtr<ID3D11Buffer>>				m_VertexBuffers;
	std::unordered_map<MeshHandle, ComPtr<ID3D11Buffer>>				m_IndexBuffers;
	std::unordered_map<MeshHandle, UINT32>								m_IndexCounts;
	std::unordered_map<TextureHandle, ComPtr<ID3D11ShaderResourceView>>	m_Textures;

	std::unordered_map<VertexShaderHandle, VertexShaderResources>		m_VertexShaders;
	std::unordered_map<PixelShaderHandle,  PixelShaderResources>		m_PixelShaders;
	//상수 버퍼
	ComPtr<ID3D11Buffer>		m_pBCB;
	BaseConstBuffer				m_BCBuffer;
	ComPtr<ID3D11Buffer>		m_pCameraCB;
	CameraConstBuffer			m_CameraCBuffer;
	ComPtr<ID3D11Buffer>		m_pSkinCB;
	SkinningConstBuffer			m_SkinCBuffer;
	ComPtr<ID3D11Buffer>		m_pLightCB;
	LightConstBuffer			m_LightCBuffer;
	ComPtr<ID3D11Buffer>		m_pUIB;
	UIBuffer					m_UIBuffer;
	ComPtr<ID3D11Buffer>		m_pMatB;
	MaterialBuffer				m_MatBuffer;
	ComPtr<ID3D11Buffer>		m_pMaskB;
	MaskingBuffer				m_MaskBuffer;


	//임시
	ComPtr<ID3D11InputLayout> m_pInputLayout;
	ComPtr<ID3D11InputLayout> m_pInputLayout_P;
	//임시 쉐이더코드
	ComPtr<ID3D11VertexShader> m_pVS;
	ComPtr<ID3D11PixelShader> m_pPS;
	ComPtr<ID3DBlob> m_pVSCode;


	//Shadow, Depth용, 지금은 그리드용
	ComPtr<ID3D11VertexShader>	m_pVS_P;
	ComPtr<ID3D11PixelShader>	m_pPS_P;
	ComPtr<ID3D11PixelShader>	m_pPS_Frustum;
	ComPtr<ID3DBlob> m_pVSCode_P;

	//PBR
	ComPtr<ID3D11VertexShader>	m_pVS_PBR;
	ComPtr<ID3D11PixelShader>	m_pPS_PBR;
	ComPtr<ID3DBlob> m_pVSCode_PBR;

	//Wall
	ComPtr<ID3D11VertexShader>	m_pVS_Wall;
	ComPtr<ID3D11PixelShader>	m_pPS_Wall;
	ComPtr<ID3DBlob> m_pVSCode_Wall;


	//Post
	ComPtr<ID3D11VertexShader> m_pVS_Post;
	ComPtr<ID3D11PixelShader> m_pPS_Post;
	ComPtr<ID3DBlob> m_pVSCode_Post;

	//UI
	ComPtr<ID3D11VertexShader> m_pVS_UI;
	ComPtr<ID3D11PixelShader> m_pPS_UI;
	ComPtr<ID3DBlob> m_pVSCode_UI;
	ComPtr<ID3D11ShaderResourceView> m_UIWhiteTexture;

	//그림자 만들기
	ComPtr<ID3D11VertexShader>	m_pVS_MakeShadow;
	ComPtr<ID3D11PixelShader>	m_pPS_MakeShadow;
	ComPtr<ID3D11PixelShader>	m_pPS_MakeShadow_Transparent;
	ComPtr<ID3DBlob>			m_pVSCode_MakeShadow;

	//Emissive용
	ComPtr<ID3D11VertexShader>	m_pVS_Emissive;
	ComPtr<ID3D11PixelShader>	m_pPS_Emissive;
	ComPtr<ID3DBlob>			m_pVSCode_Emissive;

//그리드
private:
	ComPtr<ID3D11Buffer> m_GridVB;
	//ComPtr<ID3D11InputLayout> m_pInputLayoutGrid;

	UINT m_GridVertexCount = 0;
	std::vector<std::vector<int>> m_GridFlags;  // 0=empty,1=blocked
	void CreateGridVB();
	void UpdateGrid(const RenderData::FrameData& frame);
	void DrawGrid();
	float m_CellSize = 1.0f;
	int   m_HalfCells = 100;

	//쿼드
protected:
	std::vector<RenderData::Vertex> quadVertices =
	{
		{ { -1.f,  1.f, 0.0f },  {0.0f, 0.0f, 0.0f}, { 0.0f, 0.0f }, {0.0f, 0.0f, 0.0f, 0.0f}, {0,0,0,0}, {0.0f,0.0f,0.0f,0.0f} }, // LT
		{ {  1.f,  1.f, 0.0f },  {0.0f, 0.0f, 0.0f}, { 1.0f, 0.0f }, {0.0f, 0.0f, 0.0f, 0.0f}, {0,0,0,0}, {0.0f,0.0f,0.0f,0.0f} }, // RT
		{ { -1.f, -1.f, 0.0f },  {0.0f, 0.0f, 0.0f}, { 0.0f, 1.0f }, {0.0f, 0.0f, 0.0f, 0.0f}, {0,0,0,0}, {0.0f,0.0f,0.0f,0.0f} }, // LB
		{ {  1.f, -1.f, 0.0f },  {0.0f, 0.0f, 0.0f}, { 1.0f, 1.0f }, {0.0f, 0.0f, 0.0f, 0.0f}, {0,0,0,0}, {0.0f,0.0f,0.0f,0.0f} }, // RB
	};

	std::vector<uint32_t> quadIndices =
	{
		0, 1, 2,
		2, 1, 3
	};

	ComPtr<ID3D11Buffer>		m_QuadVertexBuffers;
	ComPtr<ID3D11Buffer>		m_QuadIndexBuffers;
	UINT						m_QuadIndexCounts = 0;

	ComPtr<ID3D11VertexShader> m_pVS_Quad;
	ComPtr<ID3D11PixelShader> m_pPS_Quad;
	ComPtr<ID3DBlob> m_pVSCode_Quad;


	void CreateQuadVB();
	void CreateQuadIB();
	void CreateUIWhiteTexture();

	//블러 테스트
	ComPtr<ID3D11ShaderResourceView> m_Vignetting;

	//스카이박스 테스트
	ComPtr<ID3D11ShaderResourceView> m_SkyBox;
	ComPtr<ID3D11VertexShader> m_pVS_SkyBox;
	ComPtr<ID3D11PixelShader> m_pPS_SkyBox;
	ComPtr<ID3DBlob> m_pVSCode_SkyBox;

	//그림자매핑 테스트
	ComPtr<ID3D11VertexShader> m_pVS_Shadow;
	ComPtr<ID3D11PixelShader> m_pPS_Shadow;
	ComPtr<ID3DBlob> m_pVSCode_Shadow;

	//물 노이즈 
	ComPtr<ID3D11ShaderResourceView> m_WaterNoise;
	//임시
	float dTime = 0.0f;

	//FullScreenTriangle
protected:
	ComPtr<ID3D11VertexShader> m_pVS_FSTriangle;
	ComPtr<ID3DBlob> m_pVSCode_FSTriangle;

private:
	std::unique_ptr<DirectX::SpriteBatch> m_SpriteBatch;
	std::unique_ptr<DirectX::SpriteFont>  m_SpriteFont;

	void SetupText();
	//문자열 줄바꿈 처리
	std::wstring WrapText(const std::wstring& text, float maxWidth)
	{
		std::wstring result;
		std::wstring line;

		for (wchar_t c : text)
		{
			if (c == L'\n')
			{
				result += line + L'\n';
				line.clear();
				continue;
			}

			line += c;

			if (DirectX::XMVectorGetX(
				m_SpriteFont->MeasureString(line.c_str())
			) > maxWidth)
			{
				result += line.substr(0, line.size() - 1) + L'\n';
				line = c;
			}
		}

		result += line;
		return result;
	}
	void RenderTextCenter(int screenW, int screenH)
	{
		/*const wchar_t* msg = L"황재하\n진영상\n홍한울\n정성우\n권윤정\n박지훈\n이지원";*/
		const wchar_t* msg = L"나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나나";
		float maxWidth = 300.f;

		std::wstring wrapped = WrapText(msg, maxWidth);

		// 1. 측정 및 계산
		XMVECTOR size = m_SpriteFont->MeasureString(wrapped.c_str());
		float x = (screenW - DirectX::XMVectorGetX(size)) * 0.5f;
		float y = (screenH - DirectX::XMVectorGetY(size)) * 0.5f;

		// 2. 파이프라인 정리 (핵심)
		// 모든 셰이더 리소스를 해제하여 Hazard 방지
		ID3D11ShaderResourceView* nullSRVs[128] = { nullptr };
		m_pDXDC->PSSetShaderResources(0, 128, nullSRVs);

		// 깊이 버퍼 해제 (Hazard #9 방지)
		// 현재 사용중인 렌더 타겟만 설정하고 DepthStencil은 nullptr로 설정
		m_pDXDC->OMSetRenderTargets(1, m_pRTView_Post.GetAddressOf(), nullptr);

		// 3. 뷰포트 설정
		SetViewPort(m_WindowSize.width, m_WindowSize.height, m_pDXDC.Get());

		// 4. SpriteBatch 실행
		// SpriteBatch가 자체적으로 InputLayout, BlendState, DepthStencil 등을 설정하도록 둠
		m_SpriteBatch->Begin();



		m_SpriteFont->DrawString(
			m_SpriteBatch.get(),
			wrapped.c_str(),
			DirectX::XMFLOAT2(x, y),
			DirectX::Colors::Black
		);

		m_SpriteBatch->End();
	}

};
