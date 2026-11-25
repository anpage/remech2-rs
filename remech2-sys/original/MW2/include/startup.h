#ifndef STARTUP_H
#define STARTUP_H

#include "types.h"

// The functions and globals of startup.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	void PrintVersion(void);
	int32_t StartupCheckStub(void);

#ifdef __cplusplus
}
#endif

#endif // STARTUP_H
