#ifndef BRIEFING_H
#define BRIEFING_H

#include "tmpackdatabase.h"
#include "types.h"

class ArchiveReader;
class ButtonMenu;
class Page;

// The functions and globals of briefing.cpp that other units use.
extern MechChar g_unk0x10071cd8[0x04];
extern ButtonMenu* g_briefingMenu;
extern Page* g_briefingPage;
extern Collection* g_briefingPages;
extern ArchiveReader* g_situationReader;
void DrawBriefing(TMPackDataBase* p_database, char* p_scenario, MechS32 p_campaign);

#endif // BRIEFING_H
