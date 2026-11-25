#ifndef POPUPPICTURE_H
#define POPUPPICTURE_H

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
		void* p_data,
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
	void* m_data;
	int32_t m_size;
	VideoDriver* m_videoDriver;
	int32_t m_left;
	int32_t m_top;
	int32_t m_width;
	int32_t m_height;
	PANE m_savedView;
	PANE m_screenView;
	WINDOW m_saved;
	AudioSample* m_sample;
};

#endif // POPUPPICTURE_H
