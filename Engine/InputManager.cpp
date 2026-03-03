#include "pch.h"
#include "InputManager.h"
#include "EventDispatcher.h"
#include "Event.h"
#include "RayHelper.h"
#include <iostream>
#include <cstdlib>
#include "GameManager.h"

void InputManager::SetEventDispatcher(EventDispatcher* eventDispatcher)
{
	m_EventDispatcher = eventDispatcher;
}

void InputManager::SetGameManager(GameManager* gameManager)
{
	m_GameManager = gameManager;
}

void InputManager::SetEnabled(bool enabled)
{
	if (m_Enabled == enabled)
		return;

	m_Enabled = enabled;
	if (!m_Enabled)
	{
		ResetState();
	}
}

void InputManager::Update()
{
	if (!m_Enabled)
		return;

	if (m_EventDispatcher == nullptr)
		return;

	const bool allowGameplayInput = !m_GameManager
		|| m_GameManager->IsExplorationInputAllowed()
		|| m_GameManager->IsCombatInputAllowed();


	m_Mouse.handled = false;
	m_PendingLeftClickMouse.handled = false;

	for (const auto& key : m_KeysDown)
	{
		if (!m_KeysDownPrev.contains(key))
		{
			//if (allowGameplayInput)
			//{
			Events::KeyEvent e{ key };
			m_EventDispatcher->Dispatch(EventType::KeyDown, &e);
			//}
		}
	}

	for (const auto& key : m_KeysDownPrev)
	{
		if (!m_KeysDown.contains(key))
		{
			//if (allowGameplayInput)
			//{
			Events::KeyEvent e{ key };
			m_EventDispatcher->Dispatch(EventType::KeyUp, &e);
			//}
		}
	}

	m_KeysDownPrev = m_KeysDown;

	if (m_PendingWheelDelta != 0)
	{
		int wheelDelta = m_PendingWheelDelta;
		m_PendingWheelDelta = 0;
		while (wheelDelta >= WHEEL_DELTA)
		{
			m_EventDispatcher->Dispatch(EventType::MouseWheelUp, nullptr);
			wheelDelta -= WHEEL_DELTA;
		}
		while (wheelDelta <= -WHEEL_DELTA)
		{
			m_EventDispatcher->Dispatch(EventType::MouseWheelDown, nullptr);
			wheelDelta += WHEEL_DELTA;
		}
	}

	const ULONGLONG now = GetTickCount64();

	auto makeUIMouseState = [&]() {
		Events::MouseState uiMouse = m_Mouse;
		if (m_HasViewportRect)
		{
			uiMouse.pos.x -= m_ViewportRect.left;
			uiMouse.pos.y -= m_ViewportRect.top;
		}
		if (m_HasViewportRect && m_UIReferenceSize.x > 0.0f && m_UIReferenceSize.y > 0.0f)
		{
			const float width = static_cast<float>(m_ViewportRect.right - m_ViewportRect.left);
			const float height = static_cast<float>(m_ViewportRect.bottom - m_ViewportRect.top);
			if (width > 0.0f && height > 0.0f)
			{
				const float scaleX = m_UIReferenceSize.x / width;
				const float scaleY = m_UIReferenceSize.y / height;
				uiMouse.pos.x = static_cast<LONG>(uiMouse.pos.x * scaleX);
				uiMouse.pos.y = static_cast<LONG>(uiMouse.pos.y * scaleY);
			}
		}
		return uiMouse;
		};

	// pending 싱글클릭 타임아웃 체크(프레임마다)
	if (m_PendingLeftClick)
	{
		const ULONGLONG elapsed = now - m_PendingLeftClickTime;
		if (elapsed > static_cast<ULONGLONG>(m_DoubleClickThereshlodsMs))
		{
			// 더블클릭 윈도우 지나면 싱글 확정 발사
			m_PendingLeftClickMouse.handled = false;
			if (!m_PendingLeftClickMouse.handled && allowGameplayInput)
				m_EventDispatcher->Dispatch(EventType::MouseLeftClick, &m_PendingLeftClickMouse);
			m_PendingLeftClick = false;
		}
	}

	// 마우스 좌클릭
	if (!m_MousePrev.leftPressed && m_Mouse.leftPressed)
	{
		m_Mouse.handled = false;

		Events::MouseState uiMouse = makeUIMouseState();
		m_EventDispatcher->Dispatch(EventType::Pressed, &uiMouse);
		if (uiMouse.handled)
		{
			m_Mouse.handled = true;
		}

		// 더블 클릭 판정 : pending 존재 + 시간 / 거리 조건 만족
		bool isDoubleClick = false;

		if (m_PendingLeftClick)
		{
			const ULONGLONG elapsed = now - m_PendingLeftClickTime;
			const int dx = std::abs(m_Mouse.pos.x - m_PendingLeftClickPos.x);
			const int dy = std::abs(m_Mouse.pos.y - m_PendingLeftClickPos.y);

			isDoubleClick = (elapsed <= static_cast<ULONGLONG>(m_DoubleClickThereshlodsMs))
				&& (dx <= m_DoubleClickMaxDistance && dy <= m_DoubleClickMaxDistance);
		}

		if (isDoubleClick)
		{
			// 더블클릭이면 pending 싱글 취소 + 더블만 발사
			m_PendingLeftClick = false;
			m_SuppressDragAfterDoubleClick = true;
			uiMouse.handled = false;
			m_EventDispatcher->Dispatch(EventType::UIDoubleClicked, &uiMouse);
			if (uiMouse.handled)
			{
				m_Mouse.handled = true;
			}
			if (!m_Mouse.handled && allowGameplayInput)
				m_EventDispatcher->Dispatch(EventType::MouseLeftDoubleClick, &m_Mouse);
		}

		else
		{
			// 첫 클릭은 pending으로 저장만(싱글클릭 이벤트 실행 X)
			m_PendingLeftClick = true;
			m_PendingLeftClickTime = now;
			m_PendingLeftClickPos = m_Mouse.pos;

			// 클릭 당시 마우스 상태를 저장해둠.
			m_PendingLeftClickMouse = m_Mouse;
		}


	}

	// 좌클릭 홀드
	else if (m_MousePrev.leftPressed && m_Mouse.leftPressed)
	{
		if (!m_SuppressDragAfterDoubleClick)
		{
			m_Mouse.handled = false;

			Events::MouseState uiMouse = makeUIMouseState();
			m_EventDispatcher->Dispatch(EventType::UIDragged, &uiMouse);
			if (uiMouse.handled)
			{
				m_Mouse.handled = true;
			}

			if (!m_Mouse.handled && allowGameplayInput)
			{
				m_Mouse.handled = false;
				m_EventDispatcher->Dispatch(EventType::Dragged, &m_Mouse);
			}
			if (!m_Mouse.handled && allowGameplayInput)
			{
				m_Mouse.handled = false;
				m_EventDispatcher->Dispatch(EventType::MouseLeftClickHold, &m_Mouse);
			}
		}
	}
	// 좌클릭 업
	else if (m_MousePrev.leftPressed && !m_Mouse.leftPressed)
	{
		m_SuppressDragAfterDoubleClick = false;
		m_Mouse.handled = false;
		
		Events::MouseState uiMouse = makeUIMouseState();
		m_EventDispatcher->Dispatch(EventType::Released, &uiMouse);
		if (uiMouse.handled)
		{
			m_Mouse.handled = true;
		}

		if (!m_Mouse.handled && allowGameplayInput)
		{
			m_Mouse.handled = false;
			m_EventDispatcher->Dispatch(EventType::MouseLeftClickUp, &m_Mouse);
		}
	}
	

	// 우클릭
	if (m_MousePrev.rightPressed == false && m_Mouse.rightPressed && allowGameplayInput)
	{
		m_Mouse.handled = false;
		m_EventDispatcher->Dispatch(EventType::MouseRightClick, &m_Mouse);
	}
	else if (m_MousePrev.rightPressed == true && m_Mouse.rightPressed && allowGameplayInput)
	{
		m_Mouse.handled = false;
		m_EventDispatcher->Dispatch(EventType::MouseRightClickHold, &m_Mouse);
	}
	else if(m_MousePrev.rightPressed == true && !m_Mouse.rightPressed && allowGameplayInput)
	{
		m_Mouse.handled = false;
		m_EventDispatcher->Dispatch(EventType::MouseRightClickUp, &m_Mouse);
	}
	

	// Hovered : 매 프레임	
	{
		Events::MouseState uiMouse = makeUIMouseState();
		m_EventDispatcher->Dispatch(EventType::UIHovered, &uiMouse);
		if (uiMouse.handled)
		{
			m_Mouse.handled = true;
		}
	}


	if (allowGameplayInput) 
	{
		// Hover는 UI 처리 여부와 무관하게 매 프레임 게임플레이 레이어에도 전달한다.
		// (월드 오브젝트 hover 판정/툴팁 갱신 용도)
		m_EventDispatcher->Dispatch(EventType::Hovered, &m_Mouse);
	}

	m_MousePrev = m_Mouse;
}

void InputManager::OnKeyDown(char key)
{
	m_KeysDown.insert(key);
}

void InputManager::OnKeyUp(char key)
{
	m_KeysDown.erase(key);
}

bool InputManager::IsKeyPressed(char key) const
{
	return m_KeysDown.contains(key);
}

bool InputManager::OnHandleMessage(const MSG& msg)
{
	if (!m_Enabled)
		return false;

	switch (msg.message)
	{

	case WM_KEYDOWN:
	{
		OnKeyDown(static_cast<char>(msg.wParam));
	}
	break;

	case WM_KEYUP:
	{
		OnKeyUp(static_cast<char>(msg.wParam));
	}
	break;

	case WM_MOUSEMOVE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	{
		HandleMsgMouse(msg);
	}
	break;
	case WM_MOUSEWHEEL:
	{
		m_PendingWheelDelta += GET_WHEEL_DELTA_WPARAM(msg.wParam);
	}
	break;
	case WM_KILLFOCUS:
	case WM_ACTIVATEAPP:
	{
		if (msg.message == WM_KILLFOCUS || msg.wParam == FALSE)
		{
			ResetState();
		}
	}
	break;
	default:
		return false; // Unhandled message
	}

	return true;
}

void InputManager::HandleMsgMouse(const MSG& msg)
{
	int x = GetXFromLParam(msg.lParam);
	int y = GetYFromLParam(msg.lParam); // [오류 수정]

	m_Mouse.pos = { x, y };

	if (msg.message == WM_LBUTTONDOWN)
	{
		m_Mouse.leftPressed = true;
		SetCapture(msg.hwnd);
	}
	else if (msg.message == WM_RBUTTONDOWN)
	{
		m_Mouse.rightPressed = true;
		SetCapture(msg.hwnd);
	}
	else if (msg.message == WM_LBUTTONUP)
	{
		m_Mouse.leftPressed = false;
		ReleaseCapture();
	}
	else if (msg.message == WM_RBUTTONUP)
	{
		m_Mouse.rightPressed = false;
		ReleaseCapture();
	} 

}

void InputManager::SetViewportRect(const RECT& rect)
{
	m_ViewportRect    = rect;
	m_HasViewportRect = true;
}

void InputManager::ClearViewportRect()
{
	m_ViewportRect    = { 0,0,0,0 };
	m_HasViewportRect = false;
}

void InputManager::SetUIReferenceSize(const DirectX::XMFLOAT2& size)
{
	m_UIReferenceSize = size;
}

bool InputManager::TryGetMouseNDC(DirectX::XMFLOAT2& outNdc) const
{
	if (!m_HasViewportRect)
		return false;

	const float width  = static_cast<float>(m_ViewportRect.right - m_ViewportRect.left);
	const float height = static_cast<float>(m_ViewportRect.bottom - m_ViewportRect.top);
	if (width <= 0.0f || height <= 0.0f)
		return false;

	const float x = static_cast<float>(m_Mouse.pos.x - m_ViewportRect.left);
	const float y = static_cast<float>(m_Mouse.pos.y - m_ViewportRect.top);

	const float nx = (x / width) * 2.0f - 1.0f;
	const float ny = 1.0f - (y / height) * 2.0f;

	outNdc = { nx, ny };
	return true;

}

bool InputManager::IsPointInViewport(const POINT& point) const
{
	if (!m_HasViewportRect)
		return false;

	return point.x >= m_ViewportRect.left
		&& point.x <= m_ViewportRect.right
		&& point.y >= m_ViewportRect.top
		&& point.y <= m_ViewportRect.bottom;
}

bool InputManager::BuildPickRay(const DirectX::XMFLOAT4X4& view, 
								const DirectX::XMFLOAT4X4& proj, 
								DirectX::XMFLOAT3& outOrigin, 
								DirectX::XMFLOAT3& outDirection) const
{
	return BuildPickRay(view, proj, m_Mouse, outOrigin, outDirection);
}

bool InputManager::BuildPickRay(const DirectX::XMFLOAT4X4& view, 
						        const DirectX::XMFLOAT4X4& proj,
						        const Events::MouseState& mouseState, 
						        DirectX::XMFLOAT3& outOrigin,
						        DirectX::XMFLOAT3& outDirection) const
{
	Ray ray{};
	if (!BuildPickRay(view, proj, mouseState, ray))
		return false;

	outOrigin = ray.m_Pos;
	outDirection = ray.m_Dir;
	return true;
}

bool InputManager::BuildPickRay(const DirectX::XMFLOAT4X4& view, 
								const DirectX::XMFLOAT4X4& 
								proj, Ray& outRay) const
{
	return BuildPickRay(view, proj, m_Mouse, outRay);
}

bool InputManager::BuildPickRay(const DirectX::XMFLOAT4X4& view, 
								const DirectX::XMFLOAT4X4& proj, 
								const Events::MouseState& mouseState, 
								Ray& outRay) const
{
	if (!m_HasViewportRect)
		return false;
	
	const float width  = static_cast<float>(m_ViewportRect.right - m_ViewportRect.left);
	const float height = static_cast<float>(m_ViewportRect.bottom - m_ViewportRect.top);
	if (width <= 0.0f || height <= 0.0f)
		return false;

	const float x = static_cast<float>(mouseState.pos.x);
	const float y = static_cast<float>(mouseState.pos.y);

	const float vpX = static_cast<float>(m_ViewportRect.left);
	const float vpY = static_cast<float>(m_ViewportRect.top);

	const auto viewMat = DirectX::XMLoadFloat4x4(&view);
	const auto projMat = DirectX::XMLoadFloat4x4(&proj);

	outRay = MakePickRayLH(x, y, vpX, vpY, width, height, viewMat, projMat);
	return true;
}

void InputManager::ResetState()
{
	m_KeysDown.clear();
	m_KeysDownPrev.clear();
	m_Mouse     = Events::MouseState{};
	m_MousePrev = Events::MouseState{};
	m_SuppressDragAfterDoubleClick = false;
	m_PendingWheelDelta = 0;
}
