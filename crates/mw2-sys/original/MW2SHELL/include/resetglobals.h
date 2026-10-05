#ifndef RESETGLOBALS_H
#define RESETGLOBALS_H

// The original shell was a DLL, loaded fresh each time the game went back to it, so each run began
// with every global at its initial value. Now ShellMain runs many times in one process.
// ResetShellGlobals, called first thing in ShellMain, keeps a copy of every global the first time
// and copies it back after. Every global the shell defines must be listed in resetglobals.cpp.
#ifdef __cplusplus
extern "C"
{
#endif

	void ResetShellGlobals();

#ifdef __cplusplus
}
#endif

#endif // RESETGLOBALS_H
