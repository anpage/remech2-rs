#ifndef CAMPAIGNMISSION_H
#define CAMPAIGNMISSION_H

#include "types.h"

#pragma pack(1)
// One mission of a clan campaign. Each campaign's table holds its 16 missions in order, then a
// NULL-scenario entry titled "Retired".
struct CampaignMission {
	char* m_scenario; // "yellSCN1"
	uint8_t m_trial;  // 1 on the four trials
	char* m_title;
};
#pragma pack()

#endif // CAMPAIGNMISSION_H
