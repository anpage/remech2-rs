#include "bwdnames.h"

#include "bwdname.h"
#include "bwdnamenode.h"
#include "decomp.h"
#include "error.h"
#include "loadres.h"
#include "simmain.h"
#include "types.h"

#include <string.h>

// The list of BWD names AddBwdName adds to.
BwdNameNode* g_bwdNames = NULL;

undefined4 g_unk0x100a9474 = 0;

BwdNameNode* g_bwdNamesTail;

// Appends a copy of a BWD name to the list.
MechS32 AddBwdName(BwdName* p_name)
{
	MechS32 result;
	BwdNameNode* node;

	result = 0;
	node = MemAlloc(sizeof(BwdNameNode));
	if (node != NULL) {
		node->m_next = NULL;
		node->m_id = p_name->m_id;
		strncpy(node->m_name, p_name->m_name, 8);
		node->m_name[8] = '\0';

		if (g_bwdNames == NULL) {
			g_bwdNames = node;
		}
		else {
			g_bwdNamesTail->m_next = node;
		}

		g_bwdNamesTail = node;
		result = 1;
	}
	else {
		Error(0x42, NULL);
	}

	return result;
}

// Finds a BWD name in the list: by name for a named entry (-1 or -2), by number otherwise.
MechS16* FindBwdName(BwdName* p_name)
{
	BwdNameNode* node;
	MechS16* found;

	node = g_bwdNames;
	found = NULL;
	while (node != NULL && found == NULL) {
		if (node->m_id == -2 || node->m_id == -1) {
			if (strcmp(node->m_name, p_name->m_name) == 0) {
				found = &node->m_id;
			}
		}
		else if (node->m_id == p_name->m_id) {
			found = &node->m_id;
		}

		node = node->m_next;
	}

	return found;
}

void FreeBwdNames(void)
{
	BwdNameNode* node;
	BwdNameNode* next;

	next = g_bwdNames;
	while (next != NULL) {
		node = next;
		next = next->m_next;
		MechHeapFree(g_primaryHeap, node);
	}

	g_bwdNames = NULL;
	g_bwdNamesTail = NULL;
}
