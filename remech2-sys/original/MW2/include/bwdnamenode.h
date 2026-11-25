#ifndef BWDNAMENODE_H
#define BWDNAMENODE_H

#include "types.h"

#pragma pack(1)
// An entry of overlay.c's list of BWD names, allocated at its packed size.
typedef struct BwdNameNode {
	struct BwdNameNode* m_next;
	int16_t m_id;
	char m_name[9];
} BwdNameNode;
#pragma pack()

#endif // BWDNAMENODE_H
