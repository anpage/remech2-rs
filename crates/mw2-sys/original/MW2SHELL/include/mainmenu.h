#ifndef MAINMENU_H
#define MAINMENU_H

#include "tmpackdatabase.h"
#include "types.h"

// The functions and globals of mainmenu.cpp that other units use.
void* AllocateAllowNew(MechS32 p_size);
void DrawMainMenu(TMPackDataBase* p_database, MechS32*);

#endif // MAINMENU_H
