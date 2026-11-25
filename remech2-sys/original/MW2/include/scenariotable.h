#ifndef SCENARIOTABLE_H
#define SCENARIOTABLE_H

#include "bwdrecord.h"
#include "types.h"

// The scenario table: the names ExecuteInclude substitutes for "^", in turn.
typedef struct ScenarioTable {
	BwdRecord m_header;
	char m_names[1][0xc]; // up to the record's end
} ScenarioTable;

#endif // SCENARIOTABLE_H
