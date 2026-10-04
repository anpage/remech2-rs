#ifndef GAMECD_H
#define GAMECD_H

#include "types.h"

// The functions and globals of gamecd.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechChar FindGameCdDrive(void);
	MechS32 GetGameCdNumber(void);

#ifdef __cplusplus
}
#endif

#endif // GAMECD_H
