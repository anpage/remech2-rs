#ifndef BUTTONMENU_H
#define BUTTONMENU_H

#include "decomp.h"
#include "mainmenubutton.h"
#include "types.h"

class Font;
class VideoDriver;
struct Collection;

#pragma pack(1)

// SIZE 0x10d
// The buttons of a menu screen.
class ButtonMenu {
public:
	ButtonMenu(VideoDriver* p_videoDriver, Font* p_font, MechU8 p_drawRect, MainMenuButton* p_buttons, MechS32 p_count);
	~ButtonMenu();
	void RemoveButtonsFrom(MechS32 p_id);
	MechS32 HitTest(MechS32 p_x, MechS32 p_y);
	void DrawRects();
	void RemoveButton(MechS32 p_id);
	void AddButton(MainMenuButton p_button, MechS32 p_id, MechU8 p_drawRect);
	void EnableButton(MechS32 p_id);
	void DisableButton(MechS32 p_id);

private:
	Collection* m_items;        // 0x00
	VideoDriver* m_videoDriver; // 0x04
	Font* m_font;               // 0x08
	undefined m_colors[0x100];  // 0x0c

public:
	// The archive reader passes it on to the buttons it adds.
	MechU8 m_drawRect; // 0x10c
};

#pragma pack()

#endif // BUTTONMENU_H
