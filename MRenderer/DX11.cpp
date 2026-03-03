#include <iostream>

#include "DX11.h"

BOOL g_bVSync = FALSE;
BOOL g_bAllowTearing = FALSE;







int ClearBackBuffer(COLOR col, ID3D11DeviceContext* dxdc, ID3D11RenderTargetView* rtview)
{
    dxdc->ClearRenderTargetView(rtview, (float*)&col);

    return S_OK;
}

int ClearBackBuffer(UINT flag, COLOR col, ID3D11DeviceContext* dxdc, ID3D11RenderTargetView* rtview, ID3D11DepthStencilView* dsview, float depth, UINT stencil)
{
    dxdc->ClearRenderTargetView(rtview, (float*)&col);
    dxdc->ClearDepthStencilView(dsview, flag, depth, stencil);	//깊이/스텐실 지우기.

    return 0;
}

int Flip(IDXGISwapChain* swapchain)
{

    const UINT syncInterval = g_bVSync ? 1u : 0u;
    UINT presentFlags = 0;
    if (!g_bVSync && g_bAllowTearing)
    {
        presentFlags |= DXGI_PRESENT_ALLOW_TEARING;
    }
    swapchain->Present(syncInterval, presentFlags);

    return 0;
}


DWORD AlignCBSize(DWORD size)
{
    DWORD sizeAligned = 0;
    BOOL bAligned = (size % 16) ? FALSE : TRUE;		//정렬(필요) 확인.
    TCHAR dbgMsg[256] = _T("");						//디버깅 메세지.


    if (bAligned)
    {
        sizeAligned = size;

        //_stprintf(dbgMsg, _T("[알림] 상수버퍼 : 16바이트 정렬됨. \n> ConstBuffer = %d \n> 필요 정렬 크기 = %d"), size, sizeAligned);
    }
    else
    {
        sizeAligned = (size / 16) * 16 + 16;		//정렬(필요) 크기 재산출.

        //_stprintf(dbgMsg, _T("[경고] 상수버퍼 : 16바이트 미정렬. \n> ConstBuffer = %d \n> 필요 정렬 크기 = %d"), size, sizeAligned);
    }

    return sizeAligned;
}

int CreateInputLayout(ID3D11Device* pDev, D3D11_INPUT_ELEMENT_DESC* ed, DWORD num, ID3DBlob* pVSCode, ID3D11InputLayout** ppLayout)
{
    HRESULT hr = S_OK;

    // 정접 입력구조 객체 생성 Create the input layout
    // 함께 사용될 셰이더(컴파일된 바이너리 코드)가 필요합니다.
    ID3D11InputLayout* pLayout = nullptr;
    hr = pDev->CreateInputLayout(ed, num, pVSCode->GetBufferPointer(), pVSCode->GetBufferSize(), &pLayout);
    if (FAILED(hr))
    {
        ERROR_MSG_HR(hr);
        return hr;
    }

    //외부로 리턴.
    *ppLayout = pLayout;

    return S_OK;
}

//초기값 없이 버퍼만 만들 때(크기 고정)
HRESULT CreateDynamicConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB)
{
    HRESULT hr = S_OK;
    ID3D11Buffer* pCB = nullptr;

    //정렬된 버퍼 크기 계산 
    DWORD sizeAligned = AlignCBSize(size);

    //상수 버퍼 정보 설정.
    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;				//동적 정점버퍼 설정.
    bd.ByteWidth = sizeAligned;						//버퍼 크기 : 128비트 정렬 추가.
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;				//CPU 접근 설정. 

    /*//서브리소스 설정.
    D3D11_SUBRESOURCE_DATA sd;
    sd.pSysMem = pData;										//상수 데이터 설정.
    sd.SysMemPitch = 0;
    sd.SysMemSlicePitch = 0;
    */

    //상수 버퍼 생성.
    hr = pDev->CreateBuffer(&bd, nullptr, &pCB);
    if (FAILED(hr))
    {
        ERROR_MSG_HR(hr);
        return hr;
    }

    //외부로 전달.
    *ppCB = pCB;

    return hr;
}

//초기 값도 같이 넣을 때
HRESULT CreateDynamicConstantBuffer(ID3D11Device* pDev, UINT size, LPVOID pData, ID3D11Buffer** ppCB)
{
    HRESULT hr = S_OK;
    ID3D11Buffer* pCB = nullptr;

    DWORD sizeAligned = AlignCBSize(size);

    D3D11_BUFFER_DESC bd;
    ZeroMemory(&bd, sizeof(bd));
    bd.Usage = D3D11_USAGE_DYNAMIC;				
    bd.ByteWidth = sizeAligned;						
    bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;				

    D3D11_SUBRESOURCE_DATA sd;
    sd.pSysMem = pData;							
    sd.SysMemPitch = 0;
    sd.SysMemSlicePitch = 0;

    hr = pDev->CreateBuffer(&bd, &sd, &pCB);
    if (FAILED(hr))
    {
        ERROR_MSG_HR(hr);
        return hr;
    }

    *ppCB = pCB;

    return S_OK;
}

HRESULT UpdateDynamicBuffer(ID3D11DeviceContext* pDXDC, ID3D11Resource* pBuff, LPVOID pData, UINT size)
{
    HRESULT hr = S_OK;

    //DWORD sizeAligned = AlignCBSize(size);


    D3D11_MAPPED_SUBRESOURCE mr = {};
    mr.pData = nullptr;							

    //버퍼 버퍼 접근
    hr = pDXDC->Map(pBuff, 0, D3D11_MAP_WRITE_DISCARD, 0, &mr);
    if (FAILED(hr))
    {
        ERROR_MSG_HR(hr);
        return hr;
    }

    memcpy(mr.pData, pData, size);				
    pDXDC->Unmap(pBuff, 0);			

    return S_OK;
}

float GetEngineTime()
{
    static ULONGLONG oldtime = GetTickCount64();
    ULONGLONG 		 nowtime = GetTickCount64();
    float dTime = (nowtime - oldtime) * 0.001f;
    oldtime = nowtime;

    return dTime;
}





void SetViewPort(int width, int height, ID3D11DeviceContext* dxdc)
{
    D3D11_VIEWPORT vp = {};
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    vp.Width = (float)width;
    vp.Height = (float)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    dxdc->RSSetViewports(1, &vp);
}






//렌더 타겟용 텍스쳐 만들기
HRESULT RTTexCreate(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex)
{
    //텍스처 정보 구성.
    D3D11_TEXTURE2D_DESC td = {};
    //ZeroMemory(&td, sizeof(td));
    td.Width = width;						//텍스처크기(1:1)
    td.Height = height;
    td.MipLevels = 0;
    td.ArraySize = 1;
    td.Format = fmt;							//텍스처 포멧 (DXGI_FORMAT_R8G8B8A8_UNORM 등..)
    td.SampleDesc.Count = 1;					// AA 없음.
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;		//용도 : RT + SRV
    td.CPUAccessFlags = 0;
    td.MiscFlags = D3D11_RESOURCE_MISC_GENERATE_MIPS;

    //텍스처 생성.
    ID3D11Texture2D* pTex = NULL;
    HRESULT hr = device->CreateTexture2D(&td, NULL, &pTex);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateTexture2D 실패"));
        return hr;
    }

    //성공후 외부로 리턴.
    if (ppTex) *ppTex = pTex;

    return hr;
}

//렌더 타겟용 뷰 만들기
//그림 그릴 추가 렌더 타겟
HRESULT RTViewCreate(ID3D11Device* device, DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView)
{
    //렌더타겟 정보 구성.
    D3D11_RENDER_TARGET_VIEW_DESC rd = {};
    //ZeroMemory(&rd, sizeof(rd));
    rd.Format = fmt;										//텍스처와 동일포멧유지.
    rd.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;		//2D RT.
    rd.Texture2D.MipSlice = 0;								//2D RT 용 추가 설정 : 밉멥 분할용 밉멥레벨 인덱스.
    //rd.Texture2DMS.UnusedField_NothingToDefine = 0;		//2D RT + AA 용 추가 설정

    //렌더타겟 생성.
    ID3D11RenderTargetView* pRTView = NULL;
    HRESULT hr = device->CreateRenderTargetView(pTex, &rd, &pRTView);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateRenderTargetView 실패"));
        return hr;
    }



    //성공후 외부로 리턴.
    if (ppRTView) *ppRTView = pRTView;

    return hr;
}


//렌더타겟용 쉐이더 리소스 뷰
//렌더 타겟에 그려진 그림을 리소스용으로 만들기
HRESULT RTSRViewCreate(ID3D11Device* device, DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV)
{
    //셰이더리소스뷰 정보 구성.
    D3D11_SHADER_RESOURCE_VIEW_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.Format = fmt;										//텍스처와 동일포멧유지.
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;		//2D SRV.
    sd.Texture2D.MipLevels = -1;								//2D SRV 추가 설정 : 밉멥 설정.
    sd.Texture2D.MostDetailedMip = 0;
    //sd.Texture2DMS.UnusedField_NothingToDefine = 0;		//2D SRV+AA 추가 설정

    //셰이더리소스뷰 생성.
    ID3D11ShaderResourceView* pTexRV = NULL;
    HRESULT hr = device->CreateShaderResourceView(pTex, &sd, &pTexRV);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] RT/ CreateShaderResourceView 실패"));
        return hr;
    }

    //성공후 외부로 리턴.
    if (ppTexRV) *ppTexRV = pTexRV;

    return hr;
}

HRESULT DSCreate(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView)
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
    hr = device->CreateTexture2D(&td, NULL, pDSTex);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] DS / CreateTexture 실패"));
        return hr;
    }


    //---------------------------------- 
    // 깊이/스텐실 뷰 생성.
    //---------------------------------- 
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = {};
    //ZeroMemory(&dd, sizeof(dd));
    dd.Format = td.Format;
    dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;			//2D (AA 없음)
    //dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2DMS;			//2D (AA 적용)
    dd.Texture2D.MipSlice = 0;
    //깊이/스텐실 뷰 생성.
    hr = device->CreateDepthStencilView(*pDSTex, &dd, pDSView);
    if (FAILED(hr))
    {
        //ynError(hr, _T("[Error] DS / CreateDepthStencilView 실패"));
        return hr;
    }

    return hr;
}
