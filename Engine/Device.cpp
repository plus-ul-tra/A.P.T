#include "pch.h"
#include "Device.h"

void Device::Create(HWND hwnd)
{
	HRESULT hr = S_OK;

	UINT creationFlags = 0;
#ifdef _DEBUG
	creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif // _DEBUG

	hr = D3D11CreateDevice(NULL,
		D3D_DRIVER_TYPE_HARDWARE,
		NULL,
		creationFlags,
		&m_FeatureLevels,
		1,
		D3D11_SDK_VERSION,
		m_Device.GetAddressOf(),
		NULL,
		m_DXDC.GetAddressOf()
	);

	if (FAILED(hr))
	{
		std::cout << "Device 생성 실패" << std::endl;
	}
}
	