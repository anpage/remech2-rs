#ifndef FONT_H
#define FONT_H

#include "types.h"

class TextGlyph;
class VideoDriver;

class Font {
public:
	Font(void* p_data, VideoDriver* p_videoDriver);
	~Font();

	int32_t GetTextWidth(char* p_text);
	int32_t GetCharacterWidth(int32_t p_char);
	TextGlyph* AddText(int32_t p_left, int32_t p_top, char* p_text, uint8_t* p_colors);
	TextGlyph* AddOverlayText(int32_t p_left, int32_t p_top, char* p_text, uint8_t* p_colors);
	int32_t DrawString(int32_t p_left, int32_t p_top, char* p_text, uint8_t* p_colors);
	TextGlyph* AddTypedText(int32_t p_left, int32_t p_top, char* p_text, uint8_t* p_colors);
	int32_t DrawChar(int32_t p_left, int32_t p_top, int32_t p_char, uint8_t* p_colors);
	void TypeKey(int32_t p_left, int32_t p_top, int32_t p_key, uint8_t* p_colors);
	void ResetTyping();

private:
	void* m_data;
	int32_t m_typedRight[0x100];
	int32_t m_typedCount;

public:
	void* m_dataCopy; // m_data, for VideoDriver::DrawString
	int32_t m_height;
	VideoDriver* m_videoDriver;
};

#endif // FONT_H
