#ifndef CMDLINE_H
#define CMDLINE_H

#include "types.h"

// The functions of cmdline.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	int32_t ProcessCmdLineArgs(char* p_cmdLine, char* p_mission);

#ifdef __cplusplus
}
#endif

#endif // CMDLINE_H
