#ifndef GAMEKEYS_H
#define GAMEKEYS_H

#include "types.h"

// The functions and globals of gamekeys.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_missionTimerStopped;
	extern int32_t g_feetToTorso;
	extern int32_t g_timeCompressionCheat;
	extern int32_t g_statusMessage;
	extern int32_t g_mechViewMode;
	extern int32_t g_missionEnded;
	extern int32_t g_chatLength;
	extern int32_t g_missionResolved;
	extern int32_t g_frontViewForRear;
	extern int32_t g_missionEndTime;
	extern int32_t g_speechFlushTime;
	extern int32_t g_overrideShutdown;
	extern char g_typedKeys[0xf];

	int32_t AppendTypedKey(int16_t p_key);
	int32_t TypedCodeMatches(char* p_code);
	void HandleCheatInput(int16_t p_key);
	void ShiftCharacter(char* p_char);
	int32_t HandleChatKey(uint32_t p_keyCode);
	void HandleGameKeys(uint16_t p_key);
	void RunGameKey(int32_t p_key);

#ifdef __cplusplus
}
#endif

#endif // GAMEKEYS_H
