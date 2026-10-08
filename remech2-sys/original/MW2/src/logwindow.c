#include "logwindow.h"

#include "log.h"
#include "types.h"

#include <stdarg.h>
#include <stdio.h>

// FUNCTION: MW2 0x1003a432
void DebugPrintInternal(const MechChar* p_format, ...)
{
	MechChar message[0x100];
	va_list args;

	va_start(args, p_format);
	vsnprintf(message, sizeof(message), p_format, args);
	va_end(args);

	MechLogDebug(message);
}
