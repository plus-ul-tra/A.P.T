#include "pch.h"

LRESULT CALLBACK NzWndProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
	NzWndBase* pNzWnd = reinterpret_cast<NzWndBase*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));

	if (pNzWnd != nullptr)
	{
		if (pNzWnd->OnWndProc(hwnd, msg, wparam, lparam))
			return 0;
	}
	switch (msg)
	{
	case WM_SIZE:
		if (pNzWnd && wparam != SIZE_MINIMIZED)
			pNzWnd->OnResize(LOWORD(lparam), HIWORD(lparam));
		return 0;
	case WM_CLOSE:
		if (pNzWnd) pNzWnd->OnClose();

		pNzWnd->Destroy();
		return 0;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;

	default:
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
}

bool NzWndBase::Create(const wchar_t* className, const wchar_t* windowName, int width, int height)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.lpszClassName = className;
	wc.lpfnWndProc = NzWndProc;

	ATOM classId = 0;
	if (!GetClassInfoEx(HINSTANCE(), className, &wc))
	{
		classId = RegisterClassEx(&wc);

		if (0 == classId) return false;
	}

	m_width = width;
	m_height = height;

	RECT rc = { 0, 0, width, height };
	constexpr DWORD kWindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU;
	AdjustWindowRect(&rc, kWindowStyle, FALSE);

	m_hwnd = CreateWindowEx(NULL, MAKEINTATOM(classId), L"", kWindowStyle, CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top, HWND(), HMENU(), HINSTANCE(), NULL);

	if (NULL == m_hwnd) return false;

	::SetWindowText((HWND)m_hwnd, windowName);

	SetWindowLongPtr((HWND)m_hwnd, GWLP_USERDATA, (LONG_PTR)this);

	ShowWindow((HWND)m_hwnd, SW_SHOW);
	//UpdateWindow((HWND)m_hwnd);

	return true;
}

void NzWndBase::Destroy()
{
	if (NULL != m_hwnd)
	{
		DestroyWindow((HWND)m_hwnd);
		m_hwnd = NULL;
	}
}

void NzWndBase::OnResize(int width, int height)
{
	m_width = width;
	m_height = height;
}