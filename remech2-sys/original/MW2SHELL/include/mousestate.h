#ifndef MOUSESTATE_H
#define MOUSESTATE_H

#include "types.h"

class Font;
class VideoDriver;

#pragma pack(1)

class MouseState {
public:
	MouseState(VideoDriver* p_videoDriver, Font* p_font, void* p_cursorShape);
	~MouseState();

	void MoveCursorTo(int32_t p_x, int32_t p_y);
	uint8_t GetDoubleClicked();
	uint32_t GetLeftPressed();
	uint32_t GetRightPressed();
	uint32_t GetMiddlePressed();
	void DrawCursorPosition();
	void PressButton(int32_t p_button);
	void ReadMouseState();

private:
	void* m_cursorShape; // g_cursorShape, never read
	VideoDriver* m_videoDriver;
	Font* m_font;
	int32_t m_positionTextWidth; // DrawCursorPosition's, restored before the next
	uint32_t m_leftPressed;
	uint32_t m_rightPressed;
	uint32_t m_middlePressed;
	uint8_t m_doubleClicked;
	uint32_t m_lastClickTime;

public:
	// The screens read the cursor position, and the cockpit controls screen's scroll arrows
	// repeat while a button is held.
	int32_t m_x;
	int32_t m_y;
	uint32_t m_leftDown;
	uint32_t m_rightDown;

private:
	uint32_t m_middleDown;
	uint32_t m_enabled;
};

#pragma pack()

// The globals of mousestate.cpp that other units use.
extern char g_cursorPositionText[0x20];

#endif // MOUSESTATE_H
