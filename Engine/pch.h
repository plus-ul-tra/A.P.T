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
#include <string>

#include <memory>
#include <chrono>

#include <wrl.h>
using namespace Microsoft::WRL;


// DirectX
#include <d3d11.h>
#pragma comment(lib, "d3d11")


// DirectX Math
#include <DirectXMath.h>
using namespace DirectX;


// DirectX Graphics Infrastructure
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#pragma comment(lib, "dxgi")

#include <unordered_map>


#include "fmod.hpp"

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



// 미리 컴파일된 헤더를 사용하는 경우 컴파일이 성공하려면 이 소스 파일이 필요합니다.
// Direct3D & DXGI
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")
//#pragma comment(lib, "d3dcompiler.lib")

// Direct2D & DirectWrite
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// WIC (Windows Imaging Component)
#pragma comment(lib, "windowscodecs.lib")
