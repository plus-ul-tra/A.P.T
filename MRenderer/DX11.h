#pragma once
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "D3D11")		

#include <Windows.h>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi1_5.h>

#include <wrl/client.h>
#include <memory>
#include <tchar.h>

#include <DirectXMath.h>

#include <WICTextureLoader.h>
#include <DDSTextureLoader.h>

#include "SpriteBatch.h"
#include "SpriteFont.h"

using Microsoft::WRL::ComPtr;

using namespace DirectX;

typedef DirectX::XMFLOAT4 COLOR;
typedef DXGI_MODE_DESC DISPLAY;

//#ifndef KEY_UP
//#define KEY_UP(key)	    ((GetAsyncKeyState(key)&0x8001) == 0x8001)
//#define KEY_DOWN(key)	((GetAsyncKeyState(key)&0x8000) == 0x8000)
//#define IsKeyUp         KEY_UP
//#define IsKeyDown       KEY_DOWN
//enum VK_CHAR
//{
//	VK_0 = 0x30,
//	VK_1, VK_2, VK_3, VK_4, VK_5, VK_6, VK_7, VK_8, VK_9,
//
//	VK_A = 0x41,
//	VK_B, VK_C, VK_D, VK_E, VK_F, VK_G, VK_H, VK_I, VK_J, VK_K, VK_L, VK_M, VK_N,
//	VK_O, VK_P, VK_Q, VK_R, VK_S, VK_T, VK_U, VK_V, VK_W, VK_X, VK_Y, VK_Z,
//};
//#endif
// 이 부분도 클래스화
inline void ERROR_MSG(HRESULT hr, const wchar_t* file, int line)
{
#if defined(_DEBUG)
    __debugbreak();
#endif

    wchar_t buf[512];
    swprintf_s(buf, L"[실패] \n파일: %s\n줄: %d\nHRESULT: 0x%08X", file, line, hr);
    MessageBox(NULL, buf, L"Error", MB_OK | MB_ICONERROR);
}

#define ERROR_MSG_HR(hr) ERROR_MSG((hr), _CRT_WIDE(__FILE__), __LINE__)


#pragma region 상태
enum class DS 
{
    DEPTH_ON,
    DEPTH_ON_WRITE_OFF,
    DEPTH_OFF,


    MAX_,


    DEPTH_ON_STENCIL_OFF = DEPTH_ON,
    DEPTH_OFF_STENCIL_OFF = DEPTH_OFF,
};

enum class RS 
{
    SOLID,				//채우기, 컬링 없음
    WIREFRM,			//Wireframe, 컬링 없음
    CULLBACK,			//뒷면 컬링 
    WIRECULLBACK,		//Wireframe, 뒷면 컬링 
    EMISSIVE,

    MAX_
};

enum class SS
{
    WRAP,               //반복
    MIRROR,             //거울반복
    CLAMP,              //가장자리 픽셀로 고정
    BORDER,             //지정된 색으로 처리
    BORDER_SHADOW,      //지정된 색으로 처리

    MAX_
};

enum class BS
{
    DEFAULT,            //불투명
    ALPHABLEND,         //투명, 반투명
    ALPHABLEND_WALL,    //벽
    ADD,                // 빛, 불꽃, 이펙트 등, 색 밝아짐
    MULTIPLY,           //그림자 등, 어두워짐

    MAX_
};
//enum 래퍼 타입
template<typename T, size_t N>
struct EnumArray
{
    T data[N];

    T& operator[](DS e)
    {
        return data[static_cast<size_t>(e)];
    }

    const T& operator[](DS e) const
    {
        return data[static_cast<size_t>(e)];
    }

    T& operator[](RS e)
    {
        return data[static_cast<size_t>(e)];
    }

    const T& operator[](RS e) const
    {
        return data[static_cast<size_t>(e)];
    }

    T& operator[](SS e)
    {
        return data[static_cast<size_t>(e)];
    }

    const T& operator[](SS e) const
    {
        return data[static_cast<size_t>(e)];
    }

    T& operator[](BS e)
    {
        return data[static_cast<size_t>(e)];
    }

    const T& operator[](BS e) const
    {
        return data[static_cast<size_t>(e)];
    }

};

#pragma endregion


struct TextureSize { int width, height; };




extern	BOOL 		g_bVSync;
extern	BOOL 		g_bAllowTearing;
extern int g_MonitorWidth;
extern int g_MonitorHeight;





int		ClearBackBuffer(COLOR col, ID3D11DeviceContext* dxdc, ID3D11RenderTargetView* rtview);
int		ClearBackBuffer(UINT flag, COLOR col, ID3D11DeviceContext* dxdc, ID3D11RenderTargetView* rtview, ID3D11DepthStencilView* dsview, float depth = 1.0f, UINT stencil = 0);
int Flip(IDXGISwapChain* swapchain);


#pragma region 버퍼 운용함수
//ID3D11Buffer*	CreateConstantBuffer(UINT size);

HRESULT CreateDynamicConstantBuffer(ID3D11Device* pDev, UINT size, ID3D11Buffer** ppCB);
HRESULT CreateDynamicConstantBuffer(ID3D11Device* pDev, UINT size, LPVOID pData, ID3D11Buffer** ppCB);
HRESULT UpdateDynamicBuffer(ID3D11DeviceContext* pDXDC, ID3D11Resource* pBuff, LPVOID pData, UINT size);

#pragma endregion


DWORD	AlignCBSize(DWORD size);

int	CreateInputLayout(ID3D11Device* pDev, D3D11_INPUT_ELEMENT_DESC* ed, DWORD num, ID3DBlob* pVSCode, ID3D11InputLayout** ppLayout);


float GetEngineTime();

template <typename T>
void SafeRelease(T*& ptr)
{
	if (ptr)
	{
		ptr->Release();
		ptr = nullptr;
	}
}

void SetViewPort(int width, int height, ID3D11DeviceContext* dxdc);





HRESULT RTTexCreate(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** ppTex);
HRESULT RTViewCreate(ID3D11Device* device, DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11RenderTargetView** ppRTView);
HRESULT RTSRViewCreate(ID3D11Device* device, DXGI_FORMAT fmt, ID3D11Texture2D* pTex, ID3D11ShaderResourceView** ppTexRV);
HRESULT DSCreate(ID3D11Device* device, UINT width, UINT height, DXGI_FORMAT fmt, ID3D11Texture2D** pDSTex, ID3D11DepthStencilView** pDSView);


//임시 지울 것

