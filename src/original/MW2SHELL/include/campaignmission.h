#ifndef CAMPAIGNMISSION_H
#define CAMPAIGNMISSION_H

#include "decomp.h"
#include "types.h"

#pragma pack(1)
// SIZE 0x09
// One mission of a clan campaign. Each campaign's table holds its 16 missions in order, then a
// NULL-scenario entry titled "Retired".
struct CampaignMission {
	MechChar* m_scenario; // 0x00 — "yellSCN1"
	undefined m_trial;    // 0x04 — 1 on the four trials
	MechChar* m_title;    // 0x05
};
#pragma pack()

#endif // CAMPAIGNMISSION_H
