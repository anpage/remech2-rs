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
	MechU32 FindEventList(MechChar* p_name);
	void PostEventToList(MechChar* p_name, MechS32 p_types, MechU16 p_target);
	void FlushEventLists(MechU32 p_lists);

#ifdef __cplusplus
}
#endif

#endif // MISSIONSETUP_H
