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
	void* m_data;                // 0x00
	int32_t m_typedRight[0x100]; // 0x04
	int32_t m_typedCount;        // 0x404

public:
	void* m_dataCopy;           // 0x408 — m_data, for VideoDriver::DrawString
	int32_t m_height;           // 0x40c
	VideoDriver* m_videoDriver; // 0x410
};

#endif // FONT_H
