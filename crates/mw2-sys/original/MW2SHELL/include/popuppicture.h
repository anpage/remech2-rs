#ifndef POPUPPICTURE_H
#define POPUPPICTURE_H

#include "decomp.h"
#include "pane.h"
#include "types.h"
#include "window.h"

class AudioSample;
class VideoDriver;

// A picture shown centered over the back buffer. It saves the pixels it covers, so hiding it
// restores the screen.
// SIZE 0x35c
class PopupPicture {
public:
	PopupPicture(
		undefined* p_data,
		MechS32 p_size,
		VideoDriver* p_videoDriver,
		MechS32 p_left,
		MechS32 p_top,
		AudioSample* p_sample
	);
	~PopupPicture();

	void Show();
	void Hide();

private:
	undefined* m_data;                 // 0x00
	MechS32 m_size;                    // 0x04
	VideoDriver* m_videoDriver;        // 0x08
	MechS32 m_left;                    // 0x0c
	MechS32 m_top;                     // 0x10
	MechS32 m_width;                   // 0x14
	MechS32 m_height;                  // 0x18
	PANE m_savedView;                  // 0x1c
	PANE m_screenView;                 // 0x30
	WINDOW m_saved;                    // 0x44
	undefined m_unk0x58[0x358 - 0x58]; // 0x58 — never accessed; 0x300 bytes, a palette's size
	AudioSample* m_sample;             // 0x358
};

#endif // POPUPPICTURE_H
