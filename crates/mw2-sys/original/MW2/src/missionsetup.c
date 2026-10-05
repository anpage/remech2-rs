#include "missionsetup.h"

#include "decomp.h"
#include "error.h"
#include "missiontable.h"
#include "objective.h"
#include "resource.h"
#include "resourcename.h"
#include "starmission.h"
#include "team.h"
#include "types.h"

#include <string.h>
#include <strings.h>

DECOMP_SIZE_ASSERT(MissionEntry, 0x97)

// Sets up star p_table->m_star's mission from its mission table: the mission's times and sounds
// and each objective's, with its conditions.
// Index order: the original scales j in the m_conditions[j] stores (j * 12 as the scaled index).
// FUNCTION: MW2 0x1004da30
void SetUpStarMission(MissionTable* p_table)
{
	MechS32 i;
	MechS32 count;
	MechS32 j;
	MechS32 kind;

	count = (p_table->m_header.m_size - 0x26) / sizeof(MissionEntry);
	if (p_table->m_entries[0].m_type != 0x10) {
		Error(0x4a, NULL);
	}

	g_objectiveTable[p_table->m_star].m_affiliation = g_teams[p_table->m_star].m_affiliation;
	g_objectiveTable[p_table->m_star].m_timeLimit = p_table->m_timeLimit;
	g_objectiveTable[p_table->m_star].m_startTime = -1;
	g_objectiveTable[p_table->m_star].m_endTime = -1;
	g_objectiveTable[p_table->m_star].m_successSpeech = FindResourceIdByName(0xb, p_table->m_successSound);
	strcpy(g_objectiveTable[p_table->m_star].m_successSound, p_table->m_successSound);
	g_objectiveTable[p_table->m_star].m_failSpeech = FindResourceIdByName(0xb, p_table->m_failSound);
	strcpy(g_objectiveTable[p_table->m_star].m_failSound, p_table->m_failSound);
	for (i = 0; i < count; i++) {
		g_objectiveTable[p_table->m_star].m_objectives[i].m_priority = p_table->m_entries[i].m_priority;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_type = p_table->m_entries[i].m_type;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_timeLimit = p_table->m_entries[i].m_timeLimit;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_startTime = -1;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_endTime = -1;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_targetStar = p_table->m_entries[i].m_targetStar;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_targetObjective = p_table->m_entries[i].m_targetObjective;
		if (p_table->m_entries[i].m_listed == 'V') {
			g_objectiveTable[p_table->m_star].m_objectives[i].m_listed = 1;
		}
		else {
			g_objectiveTable[p_table->m_star].m_objectives[i].m_listed = 0;
		}

		if (p_table->m_entries[i].m_requirement == 'M') {
			g_objectiveTable[p_table->m_star].m_objectives[i].m_mandatory = 1;
		}
		else {
			g_objectiveTable[p_table->m_star].m_objectives[i].m_mandatory = 0;
		}

		g_objectiveTable[p_table->m_star].m_objectives[i].m_requiredCount = p_table->m_entries[i].m_requiredCount;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_engagement = p_table->m_entries[i].m_engagement;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_successSpeech =
			FindResourceIdByName(0xb, p_table->m_entries[i].m_successSound);
		strcpy(g_objectiveTable[p_table->m_star].m_objectives[i].m_successSound, p_table->m_entries[i].m_successSound);
		g_objectiveTable[p_table->m_star].m_objectives[i].m_failSpeech =
			FindResourceIdByName(0xb, p_table->m_entries[i].m_failSound);
		strcpy(g_objectiveTable[p_table->m_star].m_objectives[i].m_failSound, p_table->m_entries[i].m_failSound);
		strcpy(g_objectiveTable[p_table->m_star].m_objectives[i].m_name, p_table->m_entries[i].m_title);
		g_objectiveTable[p_table->m_star].m_objectives[i].m_allConditions = p_table->m_entries[i].m_allConditions;
		for (j = 0; j < 8; j++) {
			switch (p_table->m_entries[i].m_conditions[j].m_kind) {
			case 'I':
				kind = 0;
				break;
			case 'C':
				kind = 1;
				break;
			case 'F':
				kind = 3;
				break;
			case 'S':
				kind = 2;
				break;
			default:
				kind = 0;
				break;
			}

			g_objectiveTable[p_table->m_star].m_objectives[i].m_conditions[j].m_kind = kind;
			g_objectiveTable[p_table->m_star].m_objectives[i].m_conditions[j].m_objective =
				p_table->m_entries[i].m_conditions[j].m_objective;
			g_objectiveTable[p_table->m_star].m_objectives[i].m_conditions[j].m_star =
				p_table->m_entries[i].m_conditions[j].m_star;
		}

		g_objectiveTable[p_table->m_star].m_objectives[i].m_state = 0;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_targetCount = 0;
		g_objectiveTable[p_table->m_star].m_objectives[i].m_targets[0] = 0x800;
	}

	g_currentObjective[p_table->m_star] = 0;
	g_objectiveTable[p_table->m_star].m_objectiveCount = count;
	if (p_table->m_star + 1 > g_objectiveCount) {
		g_objectiveCount = p_table->m_star + 1;
	}
}

// Returns the objectives that wait (state 0, 1 or 7) on the event list p_name, marking them as
// waiting (7): bit j for objective j, bit 16 + i for mission table i.
// Stack-slot permutation: every local.
// FUNCTION: MW2 0x1004e35b
MechU32 FindEventList(MechChar* p_name)
{
	MechU32 unk0x04;
	MechS32 j;
	MechU8 state;
	MechS32 i;
	MechU32 lists;

	lists = 0;
	if (!*p_name) {
		return 0;
	}

	for (i = 0; i < g_objectiveCount; i++) {
		if (g_missionTables[i]) {
			for (j = 0; j < g_objectiveTable[i].m_objectiveCount; j++) {
				state = g_objectiveTable[i].m_objectives[j].m_state;
				if (state == 0 || state == 1 || state == 7) {
					unk0x04 = (MechU8) g_objectiveTable[i].m_objectives[j].m_targets[0];
					if (!strcasecmp(p_name, g_missionTables[i]->m_entries[j].m_name)) {
						lists |= 1 << j;
						lists |= (1 << i) << 16;
						g_objectiveTable[i].m_objectives[j].m_state = 7;
					}
				}
			}
		}
	}

	return lists;
}

// Adds the target p_target to the waiting objectives on the event list p_name whose type is in
// p_types.
// The objective's address scales i and j in the opposite order (index order), and stack-slot
// permutation: count, i, j and state.
// FUNCTION: MW2 0x1004e4e6
void PostEventToList(MechChar* p_name, MechS32 p_types, MechU16 p_target)
{
	MechS32 count;
	MechS32 j;
	MechU8 state;
	MechS32 i;

	for (i = 0; i < g_objectiveCount; i++) {
		if (g_missionTables[i]) {
			for (j = 0; j < 48; j++) {
				state = g_objectiveTable[i].m_objectives[j].m_state;
				if (state == 7 && !strcasecmp(p_name, g_missionTables[i]->m_entries[j].m_name) &&
					(p_types & g_objectiveTable[i].m_objectives[j].m_type) &&
					g_objectiveTable[i].m_objectives[j].m_targetCount < 40) {
					count = g_objectiveTable[i].m_objectives[j].m_targetCount;
					g_objectiveTable[i].m_objectives[j].m_targetCount++;
					g_objectiveTable[i].m_objectives[j].m_targets[count] = p_target;
					FUN_1001cde1(p_target);
				}
			}
		}
	}
}

// Walks the waiting objectives of the lists p_lists (FindEventList's bits) and does nothing
// with them.
// Stack-slot permutation: i and j.
// FUNCTION: MW2 0x1004e69c
void FlushEventLists(MechU32 p_lists)
{
	MechS32 j;
	MechS32 i;

	for (i = 0; i < g_objectiveCount; i++) {
		if (g_missionTables[i]) {
			for (j = 0; j < 48; j++) {
				if ((p_lists & (1 << j)) && (p_lists & ((1 << i) << 16)) &&
					g_objectiveTable[i].m_objectives[j].m_state == 7) {
				}
			}
		}
	}
}
