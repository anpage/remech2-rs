#ifndef BUTTONMENU_H
#define BUTTONMENU_H

#include "decomp.h"
#include "mainmenubutton.h"
#include "types.h"

class Font;
class VideoDriver;
struct Collection;

#pragma pack(1)

// The buttons of a menu screen.
class ButtonMenu {
public:
	ButtonMenu(
		VideoDriver* p_videoDriver,
		Font* p_font,
		uint8_t p_drawRect,
		MainMenuButton* p_buttons,
		int32_t p_count
	);
	~ButtonMenu();
	void RemoveButtonsFrom(int32_t p_id);
	int32_t HitTest(int32_t p_x, int32_t p_y);
	void DrawRects();
	void RemoveButton(int32_t p_id);
	void AddButton(MainMenuButton p_button, int32_t p_id, uint8_t p_drawRect);
	void EnableButton(int32_t p_id);
	void DisableButton(int32_t p_id);

private:
	Collection* m_items;        // 0x00
	VideoDriver* m_videoDriver; // 0x04
	Font* m_font;               // 0x08
	undefined m_colors[0x100];  // 0x0c

public:
	// The archive reader passes it on to the buttons it adds.
	uint8_t m_drawRect; // 0x10c
};

#pragma pack()

#endif // BUTTONMENU_H
