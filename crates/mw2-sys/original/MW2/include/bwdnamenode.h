#ifndef BWDNAMENODE_H
#define BWDNAMENODE_H

#include "types.h"

#pragma pack(1)
// An entry of overlay.c's list of BWD names, allocated at its packed size.
// SIZE 0xf
typedef struct BwdNameNode {
	struct BwdNameNode* m_next; // 0x00
	MechS16 m_id;               // 0x04
	MechChar m_name[9];         // 0x06
} BwdNameNode;
#pragma pack()

#endif // BWDNAMENODE_H
