#include "popuppicture.h"

#include "audiosample.h"
#include "vfxa.h"
#include "videodriver.h"
#include "windowstate.h"

PopupPicture::PopupPicture(
	undefined* p_data,
	int32_t p_size,
	VideoDriver* p_videoDriver,
	int32_t p_left,
	int32_t p_top,
	AudioSample* p_sample
)
{
	int32_t size;

	m_data = p_data;
	m_size = p_size;
	m_videoDriver = p_videoDriver;
	m_sample = p_sample;
	m_videoDriver->ReadPictureSize(&m_width, &m_height, m_data, m_size, 2);

	size = VFX_shape_bounds(p_data, 0);
	m_width = (size >> 16) + 2;
	m_height = (size & 0xffff) + 2;
	m_left = p_left;
	m_top = p_top;
	m_left = (640 - m_width) / 2;
	m_top = (480 - m_height) / 2;

	m_saved.m_xMax = m_width - 1;
	m_saved.m_yMax = m_height - 1;
	m_saved.m_buffer = (undefined*) MechHeapAlloc(g_primaryHeap, m_width * m_height);

	m_savedView.m_x0 = 0;
	m_savedView.m_y0 = 0;
	m_savedView.m_x1 = m_saved.m_xMax;
	m_savedView.m_y1 = m_saved.m_yMax;
	m_savedView.m_window = &m_saved;

	m_screenView.m_x0 = m_left;
	m_screenView.m_y0 = m_top;
	m_screenView.m_x1 = m_width + m_left - 1;
	m_screenView.m_y1 = m_height + m_top - 1;
	m_screenView.m_window = &m_videoDriver->m_backBuffer;

	VFX_pane_copy(&m_screenView, 0, 0, &m_savedView, 0, 0, -1);
}

PopupPicture::~PopupPicture()
{
	Hide();
	MechHeapFree(g_primaryHeap, m_data);

	if (m_sample) {
		m_sample->Stop();
	}
}

void PopupPicture::Show()
{
	if (m_sample && !m_sample->IsPlaying()) {
		m_sample->Start();
	}

	VFX_shape_draw(&m_videoDriver->m_backView, m_data, 0, 319, 239);
	m_videoDriver->RestoreBackground(m_left, m_top, m_width, m_height);
	m_videoDriver->RedrawGlyphs(0);
	m_videoDriver->RedrawGlyphs(1);
}

void PopupPicture::Hide()
{
	VFX_pane_copy(&m_savedView, 0, 0, &m_screenView, 0, 0, -1);
	m_videoDriver->RestoreBackground(m_left, m_top, m_width, m_height);
	m_videoDriver->RedrawGlyphs(0);
	m_videoDriver->RedrawGlyphs(1);
}
