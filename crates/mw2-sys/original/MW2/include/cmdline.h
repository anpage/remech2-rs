#ifndef CMDLINE_H
#define CMDLINE_H

#include "decomp.h"
#include "types.h"

// The functions of cmdline.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 ProcessCmdLineArgs(MechChar* p_cmdLine, undefined4* p_flags, MechChar* p_mission);

#ifdef __cplusplus
}
#endif

#endif // CMDLINE_H
