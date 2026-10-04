#include "mousestate.h"

#include "font.h"
#include "inputdeviceinfo.h"
#include "inputdriver.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

DECOMP_SIZE_ASSERT(MouseState, 0x43)

// GLOBAL: MW2SHELL 0x100904a8
MechChar g_cursorPositionText[0x20];

// ReadMouseState's bounds test keeps a jmp per return: an /Ob1-expanded inline function.
inline MechS32 IsInsideWindow(POINT& p_point)
{
	if (p_point.x < 0 || p_point.x >= g_windowWidth) {
		return FALSE;
	}
	if (p_point.y < 0 || p_point.y >= g_windowHeight) {
		return FALSE;
	}

	return TRUE;
}

// FUNCTION: MW2SHELL 0x1003a790
MouseState::MouseState(VideoDriver* p_videoDriver, Font* p_font, void* p_unk0x00)
{
	m_videoDriver = p_videoDriver;
	m_font = p_font;
	m_unk0x00 = p_unk0x00;

	m_leftDown = m_leftPressed = 0;
	m_rightDown = m_rightPressed = 0;
	m_middleDown = m_middlePressed = 0;
	m_doubleClicked = m_unk0x1d = m_unk0x1e = 0;
	m_lastClickTime = m_unk0x23 = m_unk0x27 = 0;
	m_positionTextWidth = 0;
	m_x = 0;
	m_y = 0;
	m_enabled = 0;
	m_enabled = 1;

	FUN_1003a91c(0);
}

// FUNCTION: MW2SHELL 0x1003a884
void MouseState::MoveCursorTo(MechS32 p_x, MechS32 p_y)
{
	POINT point;

	if (!m_enabled) {
		return;
	}

	point.x = p_x;
	point.y = p_y;
	ClientToScreen(g_gameWindow, &point);
	SetCursorPos(point.x, point.y);
}

// FUNCTION: MW2SHELL 0x1003a8d8
MouseState::~MouseState()
{
	if (!m_enabled) {
		return;
	}
}

// FUNCTION: MW2SHELL 0x1003a900
undefined MouseState::GetDoubleClicked()
{
	return m_doubleClicked;
}

// Empty. The constructor calls it with 0, but neither the body nor the call says what it was
// for, so it keeps its placeholder.
// FUNCTION: MW2SHELL 0x1003a91c
void MouseState::FUN_1003a91c(undefined4)
{
}

// FUNCTION: MW2SHELL 0x1003a934
undefined4 MouseState::GetLeftPressed()
{
	return m_leftPressed;
}

// FUNCTION: MW2SHELL 0x1003a950
undefined4 MouseState::GetRightPressed()
{
	return m_rightPressed;
}

// FUNCTION: MW2SHELL 0x1003a96c
undefined4 MouseState::GetMiddlePressed()
{
	return m_middlePressed;
}

// FUNCTION: MW2SHELL 0x1003a988
void MouseState::DrawCursorPosition()
{
	if (m_rightDown == 1) {
		sprintf(g_cursorPositionText, "(%d,%d)", m_x, m_y);

		if (m_positionTextWidth) {
			m_videoDriver->RestoreBackground(0x230, 0x14, m_positionTextWidth, m_font->m_height);
		}

		m_positionTextWidth = m_videoDriver->DrawString(0x230, 0x14, m_font->m_dataCopy, g_cursorPositionText, NULL);
	}
}

// FUNCTION: MW2SHELL 0x1003aa2a
void MouseState::PressButton(MechS32 p_button)
{
	switch (p_button) {
	case 0:
		m_leftDown = 1;
		m_leftPressed = 1;
		break;
	case 1:
		m_middleDown = 1;
		m_middlePressed = 1;
		break;
	case 2:
		m_rightDown = 1;
		m_rightPressed = 1;
		break;
	default:
		break;
	}
}

// The only diff is a stack-slot permutation of point, leftDown, rightDown and middleDown.
// FUNCTION: MW2SHELL 0x1003aac5
void MouseState::ReadMouseState()
{
	POINT point;
	undefined4 middleDown;
	undefined4 leftDown;
	undefined4 rightDown;

	if (!g_windowActive) {
		return;
	}

	if (!g_runSim) {
		DrawCursorPosition();
	}

	leftDown = m_leftDown;
	rightDown = m_rightDown;
	middleDown = m_middleDown;

	if (m_enabled && GetCursorPos(&point)) {
		ScreenToClient(g_gameWindow, &point);

		if (IsInsideWindow(point)) {
			m_x = point.x;
			m_y = point.y;

			if (GetAsyncKeyState(VK_LBUTTON) & 0x8000) {
				m_leftDown = 1;
			}
			else {
				m_leftDown = 0;
			}

			if (GetAsyncKeyState(VK_RBUTTON) & 0x8000) {
				m_rightDown = 1;
			}
			else {
				m_rightDown = 0;
			}

			if (GetAsyncKeyState(VK_MBUTTON) & 0x8000) {
				m_middleDown = 1;
			}
			else {
				m_middleDown = 0;
			}
		}
	}

	if (m_leftDown == 1 && leftDown == 0) {
		m_leftPressed = 1;
	}
	else {
		m_leftPressed = 0;
	}

	if (m_rightDown == 1 && rightDown == 0) {
		m_rightPressed = 1;
	}
	else {
		m_rightPressed = 0;
	}

	if (m_middleDown == 1 && middleDown == 0) {
		m_middlePressed = 1;
	}
	else {
		m_middlePressed = 0;
	}

	m_doubleClicked = 0;
	if (m_leftDown == 1 && leftDown == 0) {
		if (timeGetTime() - m_lastClickTime <= 200) {
			m_doubleClicked = 1;
		}
		else {
			m_lastClickTime = timeGetTime();
		}
	}
}
