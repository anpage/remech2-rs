#ifndef MISSIONSETUP_H
#define MISSIONSETUP_H

#include "types.h"

struct MissionTable;

// The functions and globals of missionsetup.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void SetUpStarMission(struct MissionTable* p_table);
	uint32_t FindEventList(char* p_name);
	void PostEventToList(char* p_name, int32_t p_types, uint16_t p_target);

#ifdef __cplusplus
}
#endif

#endif // MISSIONSETUP_H
