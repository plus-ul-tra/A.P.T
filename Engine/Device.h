#pragma once

#include <wrl/client.h>
#include <d3d11.h>

using Microsoft::WRL::ComPtr;

class Device
{ // Device만 생성
public:
	void Create(HWND hwnd);
	ComPtr<ID3D11Device>		 GetDevice	() { return m_Device; }
	ComPtr<ID3D11DeviceContext>  GetDXDC    () { return m_DXDC;   }
private:
	ComPtr<ID3D11Device>		 m_Device;
	ComPtr<ID3D11DeviceContext>  m_DXDC;
	D3D_FEATURE_LEVEL m_FeatureLevels = D3D_FEATURE_LEVEL_11_0;
};



