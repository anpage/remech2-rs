#ifndef MOUSESTATE_H
#define MOUSESTATE_H

#include "decomp.h"
#include "types.h"

class Font;
class VideoDriver;

#pragma pack(1)

// SIZE 0x43
class MouseState {
public:
	MouseState(VideoDriver* p_videoDriver, Font* p_font, void* p_cursorShape);
	~MouseState();

	void MoveCursorTo(MechS32 p_x, MechS32 p_y);
	undefined GetDoubleClicked();
	void FUN_1003a91c(undefined4);
	undefined4 GetLeftPressed();
	undefined4 GetRightPressed();
	undefined4 GetMiddlePressed();
	void DrawCursorPosition();
	void PressButton(MechS32 p_button);
	void ReadMouseState();

private:
	void* m_cursorShape;         // 0x00 — g_cursorShape, never read
	VideoDriver* m_videoDriver;  // 0x04
	Font* m_font;                // 0x08
	MechS32 m_positionTextWidth; // 0x0c — DrawCursorPosition's, restored before the next
	undefined4 m_leftPressed;    // 0x10
	undefined4 m_rightPressed;   // 0x14
	undefined4 m_middlePressed;  // 0x18
	undefined m_doubleClicked;   // 0x1c
	undefined m_unk0x1d;         // 0x1d — only cleared, by the constructor
	undefined m_unk0x1e;         // 0x1e — only cleared, by the constructor
	undefined4 m_lastClickTime;  // 0x1f
	undefined4 m_unk0x23;        // 0x23 — only cleared, by the constructor
	undefined4 m_unk0x27;        // 0x27 — only cleared, by the constructor

public:
	// The screens read the cursor position and the cockpit controls screen's scroll arrows
	// repeat while a button is held; they read these directly: an inline accessor would leave a
	// jmp at /Ob1.
	MechS32 m_x;            // 0x2b
	MechS32 m_y;            // 0x2f
	undefined4 m_leftDown;  // 0x33
	undefined4 m_rightDown; // 0x37

private:
	undefined4 m_middleDown; // 0x3b
	undefined4 m_enabled;    // 0x3f
};

#pragma pack()

// The globals of mousestate.cpp that other units use.
extern MechChar g_cursorPositionText[0x20];

#endif // MOUSESTATE_H
