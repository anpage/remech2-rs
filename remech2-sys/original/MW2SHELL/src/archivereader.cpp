#include "archivereader.h"

#include "audiosample.h"
#include "buttonmenu.h"
#include "collection.h"
#include "font.h"
#include "keyboardinput.h"
#include "mainmenubutton.h"
#include "menudata.h"
#include "menuscreen.h"
#include "messages.h"
#include "mousestate.h"
#include "page.h"
#include "refreshmode.h"
#include "shellglobals.h"
#include "shellmain.h"
#include "stringutil.h"
#include "textglyph.h"
#include "tmpackdatabase.h"
#include "types.h"
#include "videodriver.h"
#include "windowstate.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ArchiveReader* g_archiveReader = NULL;

// The message to post when the archive is left.
size_t g_archiveReturnMessage = 0;

// Database item 103: started when the archive opens, and again once a long entry's text has
// been read.
AudioSample* g_archiveSound = NULL;

// The text of the entry being loaded.
char g_archiveText[0x10000];

// The entry's title, the topic link names and the text line being read.
char g_archiveTitle[0x200];

char g_archiveTopicName[0x200];

// The link buttons of PrevPage and NextPage.
MainMenuButton g_prevPageLinkButton;

MainMenuButton g_nextPageLinkButton;

char g_archiveLine[0x400];

void ArchiveCallback(TMPackDataBase*, int32_t*, uint8_t*, char**, int32_t p_msg);

// Opens the clan hall archive of a campaign.
void DrawArchive(TMPackDataBase* p_database, int32_t p_campaign, size_t p_wParam)
{
	void* audioData;
	int32_t audioSize;

	g_videoDriver->LoadBackground(p_database, g_archiveScreens[p_campaign].m_picture);
	p_database->GetDBItem(103, &audioData, &audioSize);
	g_archiveSound = new AudioSample(g_audioSubsystem, audioData, audioSize);
	g_archiveSound->SetVolume(0x32);
	g_archiveSound->Start();

	g_archiveReader = new ArchiveReader(
		g_archiveNames[p_campaign],
		g_archiveFont,
		1,
		true,
		NULL,
		NULL,
		g_archiveScreens[p_campaign].m_buttons,
		1
	);
	g_archiveReturnMessage = p_wParam;
	RegisterScreenFunction(ArchiveCallback);
}

void ArchiveCallback(TMPackDataBase*, int32_t*, uint8_t*, char**, int32_t p_msg)
{
	if (p_msg == c_msgScreenFrame) {
		p_msg = g_archiveReader->Run();
	}

	if (p_msg != c_msgArchive) {
		delete g_archiveReader;
		g_archiveReader = NULL;
		delete g_archiveSound;
		g_archiveSound = NULL;

		if (p_msg == c_msgQuit) {
			MechPostMessage(c_msgQuit, c_msgArchive, 0);
		}
		else if (p_msg == c_msgReaderBack || p_msg == c_msgReaderExit) {
			MechPostMessage(g_archiveReturnMessage, c_msgArchive, 0);
		}
		else {
			MechPostMessage(p_msg, c_msgArchive, 0);
		}
		UnregisterScreenFunction(ArchiveCallback);
	}
}

void ArchiveReader::AddTopic(int16_t p_entry, int32_t p_index)
{
	Topic* topic = NULL;

	topic = (Topic*) MechHeapAlloc(g_primaryHeap, sizeof(Topic));
	topic->m_id = p_index + c_buttonTopic;
	topic->m_entry = p_entry;
	ExpandCollection(m_topics, topic);
}

// Clears the screen's glyphs, without deleting them.
void ArchiveReader::ClearGlyphs()
{
	g_videoDriver->ClearGlyphs(false);
}

void ArchiveReader::FirstPage()
{
	Page* page = NULL;

	if (m_pages->m_count == 1) {
		m_menu->RemoveButton(c_buttonNextPage);
	}

	if (m_page > 0 && m_page < m_pages->m_count) {
		page = (Page*) CollectionGet(m_pages, m_page);
		page->Hide();
		if (m_database) {
			m_menu->RemoveButtonsFrom(c_buttonTopic);
		}
	}

	m_page = 0;
	PrevPage();
}

void ArchiveReader::PrevPage()
{
	Page* page = NULL;
	Page::Link* link = NULL;
	MainMenuButton* button = &g_prevPageLinkButton;
	int32_t i;

	button->m_textPos.x = 0;
	button->m_textPos.y = 0;
	button->m_text = AllocateString("");

	if (m_pages->m_count == 1) {
		m_menu->RemoveButton(c_buttonNextPage);
	}

	if (m_page > 0 && m_page < m_pages->m_count) {
		page = (Page*) CollectionGet(m_pages, m_page);
		page->Hide();
		if (m_database) {
			m_menu->RemoveButtonsFrom(c_buttonTopic);
		}
	}

	m_page--;
	if (m_page < 0) {
		m_page = 0;
	}

	if (m_page == 0) {
		m_menu->RemoveButton(c_buttonPrevPage);
		if (m_titleGlyph) {
			delete m_titleGlyph;
		}
		m_titleGlyph = g_titleFont->AddText(0x140, 0x32, m_title, NULL);
	}

	if (m_page < m_pages->m_count && m_pages->m_count > 1) {
		m_menu->RemoveButton(c_buttonNextPage);
		m_menu->AddButton(m_buttons[c_buttonNextPage], c_buttonNextPage, m_menu->m_drawRect);
	}

	if (m_pages->m_count > 0) {
		m_currentPage = (Page*) CollectionGet(m_pages, m_page);
		for (i = 0; i < m_currentPage->m_links->m_count; i++) {
			link = (Page::Link*) CollectionGet(m_currentPage->m_links, i);
			button->m_left = link->m_left;
			button->m_top = link->m_top;
			button->m_right = link->m_right;
			button->m_bottom = link->m_bottom;
			m_menu->AddButton(g_prevPageLinkButton, link->m_id + c_buttonTopic, m_menu->m_drawRect);
		}
		m_currentPage->Restart();
	}
}

void ArchiveReader::NextPage()
{
	Page* page = NULL;
	MainMenuButton* button = &g_nextPageLinkButton;
	int32_t i;
	Page::Link* link;

	button->m_textPos.x = 0;
	button->m_textPos.y = 0;
	button->m_text = AllocateString("");

	if (m_page >= 0 && m_page < m_pages->m_count - 1) {
		page = (Page*) CollectionGet(m_pages, m_page);
		page->Hide();
		if (m_database) {
			m_menu->RemoveButtonsFrom(c_buttonTopic);
		}
	}

	m_page++;
	if (m_page >= m_pages->m_count) {
		m_page = m_pages->m_count - 1;
	}

	if (m_page == m_pages->m_count - 1) {
		m_menu->RemoveButton(c_buttonNextPage);
	}

	if (m_page > 0) {
		m_menu->RemoveButton(c_buttonPrevPage);
		m_menu->AddButton(m_buttons[c_buttonPrevPage], c_buttonPrevPage, m_menu->m_drawRect);
	}

	m_currentPage = (Page*) CollectionGet(m_pages, m_page);
	for (i = 0; i < m_currentPage->m_links->m_count; i++) {
		link = (Page::Link*) CollectionGet(m_currentPage->m_links, i);
		button->m_left = link->m_left;
		button->m_top = link->m_top;
		button->m_right = link->m_right;
		button->m_bottom = link->m_bottom;
		m_menu->AddButton(g_nextPageLinkButton, link->m_id + c_buttonTopic, m_menu->m_drawRect);
	}
	m_currentPage->Restart();
}

// Follows a topic link: opens its entry in a child reader.
void ArchiveReader::OpenTopic(int32_t p_id)
{
	Page* page = NULL;
	Topic* topic = NULL;
	int32_t i;

	page = (Page*) CollectionGet(m_pages, m_page);
	page->Hide();

	for (i = 0; i < m_topics->m_count; i++) {
		topic = (Topic*) CollectionGet(m_topics, i);
		if (topic->m_id == p_id) {
			if (m_titleGlyph) {
				delete m_titleGlyph;
				m_titleGlyph = NULL;
			}

			m_child =
				new ArchiveReader(m_title, g_archiveFont, topic->m_entry, false, m_database, NULL, m_buttons, m_count);
			break;
		}
	}

	if (m_topics->m_count == i) {
		m_child = NULL;
	}
}

// Returns c_msgArchive while the reader stays open.
int32_t ArchiveReader::Run()
{
	int32_t result;
	int32_t button;

	if (m_child) {
		result = m_child->Run();
		if (result != c_msgArchive) {
			delete m_child;
			m_child = NULL;
		}

		return result == c_msgReaderBack ? c_msgArchive : result;
	}

	if (m_currentPage) {
		m_currentPage->TypeStep();
	}

	if (!m_titleGlyph) {
		m_titleGlyph = g_titleFont->AddText(0x140, 0x32, m_title, NULL);
	}

	button = m_menu->HitTest(g_mouseState->m_x, g_mouseState->m_y);
	if (g_keyboardInput->m_key) {
		switch (g_keyboardInput->m_key) {
		case 0xc4:
			button = c_buttonPrevPage;
			g_mouseState->PressButton(0);
			break;
		case 0xc5:
			button = c_buttonNextPage;
			g_mouseState->PressButton(0);
			break;
		case 0xc2:
			button = c_buttonBack;
			g_mouseState->PressButton(0);
			break;
		default:
			break;
		}
	}

	switch (button) {
	case c_buttonExit:
		if (g_mouseState->GetLeftPressed() == 1) {
			return c_msgReaderExit;
		}
		break;
	case c_buttonNextPage:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		NextPage();
		break;
	case c_buttonPrevPage:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		PrevPage();
		break;
	case c_buttonBack:
		if (g_mouseState->GetLeftPressed() != 1) {
			break;
		}
		if (m_page == 0) {
			return c_msgReaderBack;
		}
		else {
			FirstPage();
		}
		break;
	default:
		if (m_database) {
			if (g_mouseState->GetLeftPressed() != 1) {
				break;
			}
			if (button != -1) {
				OpenTopic(button);
			}
		}
		break;
	}

	return c_msgArchive;
}

// Reads an archive entry: a title command, then text, picture and topic commands up to the end
// command. The text is laid out into as many pages as it needs.
void ArchiveReader::Load(int32_t p_entry)
{
	Page* page;
	void* data;
	uint8_t soundStarted;
	int32_t topics;
	int32_t length;
	int32_t offset;
	int16_t command;
	int32_t result;
	uint8_t picture;
	uint8_t pad;
	int16_t topicEntry;
	int32_t size;
	char* text;
	char* rest;
	int32_t i;
	uint32_t wordLength;

	page = NULL;
	data = NULL;
	topics = 0;
	soundStarted = false;
	topics = 0;
	length = 0;
	offset = 0;

	result = m_database->ReadDBItemData(p_entry, offset, &command, 2);
	offset += 2;
	if (result) {
		delete g_videoDriver;
		fprintf(stderr, "Could not access Archive DB entry\n");
		fflush(stderr);
		exit(1);
	}

	if (command != 0x100) {
		delete g_videoDriver;
		fprintf(stderr, "Cmd Title not found\n");
		fprintf(stderr, "Found: %d\n", command);
		fflush(stderr);
		exit(1);
	}

	result = m_database->ReadDBItemString(p_entry, offset, g_archiveTitle);
	offset += (int32_t) strlen(g_archiveTitle) + 1;
	strcpy(m_title, "~");
	strcat(m_title, g_archiveTitle);

	command = -1;
	while (command != -0x100) {
		g_mouseState->ReadMouseState();
		result = m_database->ReadDBItemData(p_entry, offset, &command, 2);
		offset += 2;

		switch (command) {
		case 0x200:
			strcpy(g_archiveText, "");
			text = g_archiveText;
			do {
				result = m_database->ReadDBItemLine(p_entry, offset, g_archiveLine);
				g_mouseState->ReadMouseState();
				if (result != 2) {
					wordLength = strlen(g_archiveLine);
					length += wordLength;
					offset += wordLength + 1;
					g_archiveLine[wordLength] = ' ';
					g_archiveLine[wordLength + 1] = '\0';
					if (!soundStarted && length >= 0x800) {
						g_archiveSound->Start();
						soundStarted = true;
					}
					strcpy(text, g_archiveLine);
					text += wordLength;
				}
			} while (result != 2);
			offset++;

			m_colors[0] = 0xff;
			m_colors[1] = 5;
			for (i = 2; i < 0x100; i++) {
				m_colors[i] = i;
			}

			rest = NULL;
			do {
				if (page == NULL || rest != NULL) {
					page = new Page(m_font, g_videoDriver, m_colors, 0x58, 0x46, 0x1d2, 0xde);
					ExpandCollection(m_pages, page);
				}

				g_mouseState->ReadMouseState();
				if (rest != NULL) {
					rest = page->Layout(rest);
				}
				else {
					rest = page->Layout(g_archiveText);
				}
			} while (rest != NULL);
			page = NULL;
			break;
		case 0x400:
			result = m_database->ReadDBItemData(p_entry, offset, &topicEntry, 2);
			offset += 2;
			result = m_database->ReadDBItemString(p_entry, offset, g_archiveTopicName);
			offset += (int32_t) strlen(g_archiveTopicName) + 1;
			AddTopic(topicEntry, topics++);
			break;
		case 0x300:
			result = m_database->ReadDBItemData(p_entry, offset, &picture, 1);
			offset++;
			result = m_database->ReadDBItemData(p_entry, offset, &pad, 1);
			offset++;
			result = m_database->GetDBItem(picture, &data, &size);
			if (page == NULL) {
				page = new Page(m_font, g_videoDriver, m_colors, 0x58, 0x46, 0x1d2, 0xde);
				ExpandCollection(m_pages, page);
			}
			page->SetBanner(data, size);
			break;
		case -0x100:
			break;
		default:
			delete g_videoDriver;
			fprintf(stderr, "Unknown Archive Command Code\n");
			fflush(stderr);
			exit(1);
			break;
		}
	}
}

ArchiveReader::ArchiveReader(
	char* p_name,
	Font* p_font,
	int32_t p_entry,
	uint8_t p_ownsDatabase,
	TMPackDataBase* p_database,
	Collection* p_pages,
	MainMenuButton* p_buttons,
	int32_t p_count
)
{
	int32_t i;

	m_font = p_font;
	m_titleGlyph = NULL;
	m_currentPage = NULL;
	m_topics = NULL;
	m_titleGlyph = NULL;
	m_ownsDatabase = p_ownsDatabase;
	m_page = -1;
	strcpy(m_title, p_name);
	m_buttons = p_buttons;
	m_count = p_count;
	m_child = NULL;
	CreateCollection(&m_pages, 10, NULL, 4, NULL);
	CreateCollection(&m_topics, 3, NULL, 4, NULL);
	ClearGlyphs();

	m_menu = new ButtonMenu(g_videoDriver, m_font, 0, m_buttons, p_count);
	m_menu->AddButton(p_buttons[c_buttonBack], c_buttonBack, m_menu->m_drawRect);

	if (m_ownsDatabase == true) {
		m_database = new TMPackDataBase(m_title);
		if (m_database == NULL) {
			delete g_videoDriver;
			fprintf(stderr, "Could not open archive database\n");
			fflush(stderr);
			exit(1);
		}
	}
	else {
		m_database = p_database;
	}

	if (p_pages == NULL) {
		Load(p_entry);
	}
	else {
		for (i = 0; i < p_pages->m_count; i++) {
			ExpandCollection(m_pages, CollectionGet(p_pages, i));
		}
		m_page = 0;
	}

	PrevPage();
}

ArchiveReader::~ArchiveReader()
{
	int32_t i;
	Page* page;

	if (m_child) {
		delete m_child;
	}

	for (i = 0; i < m_pages->m_count; i++) {
		page = (Page*) CollectionGet(m_pages, i);
		page->Hide();
		if (m_database) {
			delete page;
		}
	}

	delete m_menu;
	if (m_ownsDatabase == true && m_database) {
		delete m_database;
	}

	if (m_titleGlyph) {
		delete m_titleGlyph;
	}
}
