#ifndef ARCHIVEREADER_H
#define ARCHIVEREADER_H

#include "decomp.h"
#include "tmpackdatabase.h"
#include "types.h"

#include <windows.h>

class Font;
class TextGlyph;
class ButtonMenu;
class Page;
class TMPackDataBase;
struct Collection;
struct MainMenuButton;

#pragma pack(1)

// SIZE 0x1b9
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

	// SIZE 0x06
	// A link to another entry.
	struct Topic {
		MechS32 m_id;    // 0x00
		MechS16 m_entry; // 0x04
	};

	ArchiveReader(
		MechChar* p_name,
		Font* p_font,
		MechS32 p_entry,
		MechU8 p_ownsDatabase,
		TMPackDataBase* p_database,
		Collection* p_pages,
		MainMenuButton* p_buttons,
		MechS32 p_count
	);
	~ArchiveReader();

	void AddTopic(MechS16 p_entry, MechS32 p_index);
	void ClearGlyphs();
	void FirstPage();
	void PrevPage();
	void NextPage();
	void OpenTopic(MechS32 p_id);
	MechS32 Run();
	void Load(MechS32 p_entry);

private:
	ButtonMenu* m_menu;               // 0x00
	TMPackDataBase* m_database;       // 0x04
	undefined4 m_unk0x08;             // 0x08 — never accessed
	Font* m_font;                     // 0x0c
	ArchiveReader* m_child;           // 0x10
	undefined m_unk0x14[0x1c - 0x14]; // 0x14 — never accessed
	TextGlyph* m_titleGlyph;          // 0x1c
	Collection* m_topics;             // 0x20
	undefined m_colors[0x100];        // 0x24
	MechU8 m_ownsDatabase;            // 0x124
	MainMenuButton* m_buttons;        // 0x125
	MechS32 m_count;                  // 0x129
	Collection* m_pages;              // 0x12d
	MechS32 m_page;                   // 0x131
	Page* m_currentPage;              // 0x135
	MechChar m_title[0x80];           // 0x139
};

#pragma pack()

// The functions and globals of archivereader.cpp that other units use.
void DrawArchive(TMPackDataBase* p_database, MechS32 p_campaign, WPARAM p_wParam);

#endif // ARCHIVEREADER_H
