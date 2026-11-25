#ifndef MECHRELOAD_H
#define MECHRELOAD_H

#include "decomp.h"
#include "mech.h"
#include "mechsegment.h"
#include "object.h"
#include "rememberedmech.h"
#include "types.h"

// The functions and globals of mechreload.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern int32_t g_reloadingPlayer;
	extern RememberedMech g_rememberedMechs[60];
	extern MechSegment* g_mechSegments[60];

	int32_t ReloadPlayerMech(int32_t p_player, int32_t p_force);
	void RememberLoadMech(Mech* p_mech, char* p_name, int32_t p_id, char* p_config);
	void RememberMechSegments(Mech* p_mech);
	SceneObject* RestoreMechSegments(MechSegment* p_segment);
	MechSegment* SaveMechSegments(SceneObject* p_obj);

#ifdef __cplusplus
}
#endif

#endif // MECHRELOAD_H
