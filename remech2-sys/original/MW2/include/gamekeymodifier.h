#ifndef GAMEKEYMODIFIER_H
#define GAMEKEYMODIFIER_H

#include "types.h"

#pragma pack(push, 1)

// A GAMEKEY.MAP key name and its key code (or modifier bits).
// SIZE 0x06
typedef struct GameKeyModifier {
	MechChar* m_name; // 0x00
	MechS16 m_code;   // 0x04
} GameKeyModifier;

#pragma pack(pop)

#endif // GAMEKEYMODIFIER_H
