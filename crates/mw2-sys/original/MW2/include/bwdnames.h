#ifndef BWDNAMES_H
#define BWDNAMES_H

#include "bwdname.h"
#include "decomp.h"
#include "types.h"

// The functions and globals of bwdnames.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern struct BwdNameNode* g_bwdNames;
	extern undefined4 g_unk0x100a9474;
	extern struct BwdNameNode* g_bwdNamesTail;

	MechS32 AddBwdName(BwdName* p_name);
	MechS16* FindBwdName(BwdName* p_name);
	void FreeBwdNames(void);

#ifdef __cplusplus
}
#endif

#endif // BWDNAMES_H
