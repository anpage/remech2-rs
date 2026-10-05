#ifndef RESETGLOBALS_H
#define RESETGLOBALS_H

// The original sim was a DLL, loaded fresh for every mission, so each mission began with every
// global at its initial value. Now SimMain runs many times in one process. ResetSimGlobals, called
// first thing in SimMain, keeps a copy of every global the first time and copies it back after.
// Every global the sim defines must be listed in resetglobals.c.
#ifdef __cplusplus
extern "C"
{
#endif

	void ResetSimGlobals(void);

#ifdef __cplusplus
}
#endif

#endif // RESETGLOBALS_H
