#ifndef STARTUP_H
#define STARTUP_H

#include "types.h"

// The functions and globals of startup.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void PrintVersion(void);
	MechS32 StartupCheckStub(void);

#ifdef __cplusplus
}
#endif

#endif // STARTUP_H
