#pragma once

#ifndef PCH_H
#define PCH_H

#include "framework.h"
#include <iostream>

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

// Windows
#include <windows.h>
#include <objbase.h> // CoInitializeEx, CoUninitialize

#include <stdio.h>

#include <cassert>
#include <exception>
#include <crtdbg.h>

#include <array>
#include <list>
#include <vector>
#include <algorithm>
#include <string>


// 미리 컴파일된 헤더를 사용하는 경우 컴파일이 성공하려면 이 소스 파일이 필요합니다.
// 

#pragma comment(lib, "ole32.lib")
// Direct3D & DXGI
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
//#pragma comment(lib, "d3dcompiler.lib")

// Direct2D & DirectWrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// WIC (Windows Imaging Component)
#pragma comment(lib, "windowscodecs.lib")


// COM 및 DirectX 인터페이스
#include <wrl/client.h>              // ComPtr
#include <d3d11.h>                   // Direct3D 11
#include <dxgi1_6.h>                 // DXGI 1.6 (Windows 10 이상 최신 스왑체인)
#include <d2d1_3.h>                  // Direct2D 1.3 (ID2D1Factory4)
#include <d2d1_3helper.h>            // D2D1::Helper 클래스들
#include <dwrite_3.h>                // DirectWrite (최신 텍스트 엔진)
#include <wincodec.h>                // WIC (이미지 로딩)
#include <fstream>

#define _USE_MATH_DEFINES
#include <cmath>
#include <stdexcept>

//https://github.com/Microsoft/DirectXTK/wiki/throwIfFailed
namespace DX
{
	// Helper class for COM exceptions
	class com_exception : public std::exception
	{
	public:
		com_exception(HRESULT hr) : result(hr) {}

		const char* what() const noexcept override
		{
			static char s_str[64] = {};
			sprintf_s(s_str, "Failure with HRESULT of %08X",
				static_cast<unsigned int>(result));
			return s_str;
		}

	private:
		HRESULT result;
	};

	// Helper utility converts D3D API failures into exceptions.
	inline void ThrowIfFailed(HRESULT hr)
	{
		if (FAILED(hr))
		{
			throw com_exception(hr);
		}
	}
}

#define _CRTDBG_MAP_ALLOC

#endif


////////////////////////////////////
// test용 editor에 넣고 실제론 지울 예정
////////////////////////////////////
#ifdef _DEBUG
#pragma comment(lib, "assimp-vc143-mtd")
#else
#pragma comment(lib, "assimp-vc143-mt")
#endif