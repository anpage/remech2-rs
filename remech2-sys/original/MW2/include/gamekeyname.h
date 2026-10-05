#ifndef GAMEKEYNAME_H
#define GAMEKEYNAME_H

#include "types.h"

#pragma pack(push, 1)

// A GAMEKEY.MAP action name and the game key action it stands for.
// SIZE 0x05
typedef struct GameKeyName {
	MechChar* m_name; // 0x00
	MechU8 m_action;  // 0x04
} GameKeyName;

#pragma pack(pop)

#endif // GAMEKEYNAME_H
