#ifndef MECHBAY_H
#define MECHBAY_H

#include "mechchassis.h"
#include "screenfield.h"
#include "tmpackdatabase.h"
#include "types.h"

#include <stddef.h>

// The functions and globals of mechbay.cpp that other units use.
extern MechChassis g_mechChassis[];
extern MechS32 g_pickStarMech;

void ShowFields(ScreenField* p_tabs);
void RedrawFields(ScreenField* p_tabs);
void HideFields(ScreenField* p_tabs);
ScreenField* FindFieldAt(ScreenField* p_tabs, MechS32 p_x, MechS32 p_y);
void DrawMechBay(TMPackDataBase* p_database, MechS32 p_campaign, size_t p_wParam);

#endif // MECHBAY_H
