#ifndef ARCHIVEREADER_H
#define ARCHIVEREADER_H

#include "mainmenubutton.h"
#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

class AudioSample;
class Font;
class TextGlyph;
class ButtonMenu;
class Page;
class TMPackDataBase;
struct Collection;
struct MainMenuButton;

#pragma pack(1)

// A clan hall archive (ARCHWO.MW2, ARCHJF.MW2): an entry of the archive database laid out as
// pages of text and pictures, with links to further entries. A followed link opens a child
// reader on top of this one.
class ArchiveReader {
public:
	enum {
		c_buttonExit = 0,
		c_buttonPrevPage = 1,
		c_buttonNextPage = 2,
		c_buttonBack = 3,
		c_buttonTopic = 4 // topic links take ids from here up
	};

	// A link to another entry.
	struct Topic {
		int32_t m_id;
		int16_t m_entry;
	};

	ArchiveReader(
		char* p_name,
		Font* p_font,
		int32_t p_entry,
		uint8_t p_ownsDatabase,
		TMPackDataBase* p_database,
		Collection* p_pages,
		MainMenuButton* p_buttons,
		int32_t p_count
	);
	~ArchiveReader();

	void AddTopic(int16_t p_entry, int32_t p_index);
	void ClearGlyphs();
	void FirstPage();
	void PrevPage();
	void NextPage();
	void OpenTopic(int32_t p_id);
	int32_t Run();
	void Load(int32_t p_entry);

private:
	ButtonMenu* m_menu;
	TMPackDataBase* m_database;
	Font* m_font;
	ArchiveReader* m_child;
	TextGlyph* m_titleGlyph;
	Collection* m_topics;
	uint8_t m_colors[0x100];
	uint8_t m_ownsDatabase;
	MainMenuButton* m_buttons;
	int32_t m_count;
	Collection* m_pages;
	int32_t m_page;
	Page* m_currentPage;
	char m_title[0x80];
};

#pragma pack()

// The functions and globals of archivereader.cpp that other units use.
extern ArchiveReader* g_archiveReader;
extern size_t g_archiveReturnMessage;
extern AudioSample* g_archiveSound;
extern char g_archiveText[0x10000];
extern char g_archiveTitle[0x200];
extern char g_archiveTopicName[0x200];
extern MainMenuButton g_prevPageLinkButton;
extern MainMenuButton g_nextPageLinkButton;
extern char g_archiveLine[0x400];
void DrawArchive(TMPackDataBase* p_database, int32_t p_campaign, size_t p_wParam);

#endif // ARCHIVEREADER_H
