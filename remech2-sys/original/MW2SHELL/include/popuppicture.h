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
class PopupPicture {
public:
	PopupPicture(
		undefined* p_data,
		int32_t p_size,
		VideoDriver* p_videoDriver,
		int32_t p_left,
		int32_t p_top,
		AudioSample* p_sample
	);
	~PopupPicture();

	void Show();
	void Hide();

private:
	undefined* m_data;          // 0x00
	int32_t m_size;             // 0x04
	VideoDriver* m_videoDriver; // 0x08
	int32_t m_left;             // 0x0c
	int32_t m_top;              // 0x10
	int32_t m_width;            // 0x14
	int32_t m_height;           // 0x18
	PANE m_savedView;           // 0x1c
	PANE m_screenView;          // 0x30
	WINDOW m_saved;             // 0x44
	AudioSample* m_sample;      // 0x358
};

#endif // POPUPPICTURE_H
