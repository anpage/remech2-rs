#ifndef MW2PRJ_H
#define MW2PRJ_H

#include "types.h"

// The indices of g_resourceTypeTags, named after the tags.
enum ResourceTag {
	c_resTagSnds = 0,
	c_resTagCel,
	c_resTagXyc,
	c_resTagShp,
	c_resTagFont,
	c_resTagMenu,
	c_resTagDisp,
	c_resTagXmid,
	c_resTagPal,
	c_resTagTable,
	c_resTagPoly,
	c_resTagText,
	c_resTagAnim,
	c_resTagMgeo,
	c_resTagHud,
	c_resTagCpit,
	c_resTagVpt,
	c_resTagMpit,
	c_resTagBwd,
	c_resTagVer,
	c_resTagAit,
	c_resTagMek,
	c_resTagLuma,
	c_resTagMus,
	c_resTagGif,
	c_resTagNtxt,
};

// The indices of g_resourceTypeExtensions, named after the extensions.
enum ResourceExtension {
	c_resExtSfl = 0,
	c_resExtXel,
	c_resExtXyc,
	c_resExtShp,
	c_resExtFnt,
	c_resExtMenuDll,
	c_resExtDispDll,
	c_resExtXmi,
	c_resExtCol,
	c_resExtTbl,
	c_resExtWtb,
	c_resExtXxt,
	c_resExt3di,
	c_resExtMgi,
	c_resExtHdi,
	c_resExtCpi,
	c_resExtVpi,
	c_resExtPit,
	c_resExtBwd,
	c_resExtAit,
	c_resExtMek,
	c_resExtLum,
	c_resExtMus,
	c_resExtGif,
	c_resExtTxt,
};

// The functions and globals of mw2prj.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern char* g_resourceTypeTags[26];
	extern MechS32 g_mw2PrjHandle;

#ifdef __cplusplus
}
#endif

#endif // MW2PRJ_H
