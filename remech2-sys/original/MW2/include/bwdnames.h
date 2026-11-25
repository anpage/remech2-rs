#ifndef BWDNAMES_H
#define BWDNAMES_H

#include "bwdname.h"
#include "types.h"

// The functions and globals of bwdnames.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern struct BwdNameNode* g_bwdNames;
	extern struct BwdNameNode* g_bwdNamesTail;

	int32_t AddBwdName(BwdName* p_name);
	int16_t* FindBwdName(BwdName* p_name);
	void FreeBwdNames(void);

#ifdef __cplusplus
}
#endif

#endif // BWDNAMES_H
