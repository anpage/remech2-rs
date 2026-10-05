#ifndef BWDKEYWORDS_H
#define BWDKEYWORDS_H

#include "types.h"

// The indices of the node tags in g_bwdTypeCodes, by keyword (g_bwdKeywordNames).
enum {
	c_bwdRev = 1,
	c_bwdDtbl = 2,
	c_bwdPlanet = 3,
	c_bwdPaletteGroup = 4,
	c_bwdTerrain = 5,
	c_bwdClimate = 6, // its keyword name is "planet" too
	c_bwdLight = 7,
	c_bwdWindow = 8,
	c_bwdStart = 9,
	c_bwdNow = 10,
	c_bwdScenarioTable = 11,
	c_bwdMissionTable = 12,
	c_bwdBitmapPrj = 14,
	c_bwdBitmapIdList = 15,
	c_bwdBitmapSection = 16,
	c_bwdBitmapEnable = 17,
	c_bwdFramePrj = 18,
	c_bwdHorizonMap = 19,
	c_bwdGroundMap = 20,
	c_bwdSkyMap = 21,
	c_bwdPolyOffset = 22,
	c_bwdBlockXform = 23,
	c_bwdRep = 24,
	c_bwdEndRep = 25,
	c_bwdBlock = 26,
	c_bwdElseBlock = 27,
	c_bwdEndBlock = 28,
	c_bwdObject = 29,
	c_bwdAnimFile = 30,
	c_bwdScrounge = 31,
	c_bwdThing = 32,
	c_bwdGamepiece = 33,
	c_bwdCptFile = 34,
	c_bwdPitFile = 35,
	c_bwdVptFile = 36,
	c_bwdHudFile = 37,
	c_bwdMgdFile = 38,
	c_bwdEyeObj = 39,
	c_bwdGameThing = 40,
	c_bwdObjLoc = 41,
	c_bwdBooyowThing = 42,
	c_bwdXplode = 43,
	c_bwdNavPoint = 44,
	c_bwdNavObject = 45,
	c_bwdLightObj = 46,
	c_bwdTask = 47,
	c_bwdPosition = 48,
	c_bwdRotate = 49,
	c_bwdInclude = 50,
	c_bwdGroup = 51,
	c_bwdGpSpec = 52,
	c_bwdMangleOff = 53,
	c_bwdMangleOn = 54,
	c_bwdAnim2d = 57,
	c_bwdStar = 58,
	c_bwdView = 59,
	c_bwdPof = 60,
	c_bwdAffiliation = 61,
	c_bwdMusic = 63,
	c_bwdAnimSound = 64,
	c_bwdLuma = 65,
	c_bwdPath = 66,
	c_bwdFormation = 67,
	c_bwdHiddenText = 68
};

// The globals of bwdkeywords.c.
#ifdef __cplusplus
extern "C"
{
#endif

	extern MechChar g_bwdVersion[5];
	extern MechChar* g_bwdKeywordNames[0x48];
	extern MechChar g_bwdExtension[8];
	extern MechU32 g_bwdTypeCodes[0x48];

#ifdef __cplusplus
}
#endif

#endif // BWDKEYWORDS_H
