#ifndef BWDNAMES_H
#define BWDNAMES_H

#include "bwdname.h"
#include "types.h"

// The functions and globals of bwdnames.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	MechS32 AddBwdName(BwdName* p_name);
	MechS16* FindBwdName(BwdName* p_name);
	void FreeBwdNames(void);

#ifdef __cplusplus
}
#endif

#endif // BWDNAMES_H
