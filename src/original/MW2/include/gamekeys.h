#ifndef GAMEKEYS_H
#define GAMEKEYS_H

#include "types.h"

// The functions and globals of gamekeys.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechS32 g_missionTimerStopped;
	extern MechS32 g_feetToTorso;
	extern MechS32 g_timeCompressionCheat;
	extern MechS32 g_statusMessage;
	extern MechS32 g_mechViewMode;
	extern MechS32 g_missionEnded;
	extern MechS32 g_chatLength;
	extern MechS32 g_missionResolved;
	extern MechS32 g_frontViewForRear;
	extern MechS32 g_missionEndTime;
	extern MechS32 g_speechFlushTime;
	extern MechS32 g_overrideShutdown;
	extern MechChar g_typedKeys[0xf];

	MechS32 AppendTypedKey(MechS16 p_key);
	MechS32 TypedCodeMatches(MechChar* p_code);
	void HandleCheatInput(MechS16 p_key);
	void ShiftCharacter(MechChar* p_char);
	MechS32 HandleChatKey(MechU32 p_keyCode);
	void HandleGameKeys(MechS32 p_unk0x00, MechS32 p_unk0x04, MechU16 p_key);
	void RunGameKey(MechS32 p_key);

#ifdef __cplusplus
}
#endif

#endif // GAMEKEYS_H
