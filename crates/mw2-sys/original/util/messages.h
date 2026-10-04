#ifndef MESSAGES_H
#define MESSAGES_H

// The game's message queue, implemented on the Rust side (src/messages.rs). Originally the
// window's: the shell's screens post their transitions to it, and the window's input and
// activation arrive through it. There is one queue and one handler, the running module's.
// Nothing here waits or looks at the window.

#include "types.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

	// The messages the game uses that Windows defined, with their Windows numbers: the game's own
	// start at c_mechMsgUser.
	enum {
		c_mechMsgDestroy = 0x0002,
		c_mechMsgPaint = 0x000f,
		c_mechMsgClose = 0x0010,
		c_mechMsgQuit = 0x0012, // wParam is the exit code
		c_mechMsgActivateApp = 0x001c,
		// Key messages: wParam is the Windows virtual key. lParam has the character the key types
		// without modifiers in bits 0 to 7 (0 if it isn't ASCII), the key's scan code in bits 16 to
		// 23 and whether that is an extended one in bit 24.
		c_mechMsgKeyFirst = 0x0100,
		c_mechMsgKeyDown = 0x0100,
		c_mechMsgKeyUp = 0x0101,
		c_mechMsgSysKeyDown = 0x0104,
		c_mechMsgSysKeyUp = 0x0105,
		c_mechMsgKeyLast = 0x0108,
		c_mechMsgCommand = 0x0111,
		c_mechMsgMouseMove = 0x0200,
		c_mechMsgUser = 0x0400
	};

	typedef struct MechMessage {
		MechU32 m_message;
		size_t m_wParam;
		MECH_INTPTR m_lParam;
	} MechMessage;

	typedef MECH_INTPTR (*MechMessageHandler)(MechU32 p_message, size_t p_wParam, MECH_INTPTR p_lParam);

	// Sets what handles sent messages, NULL for nothing. Returns the handler it replaces.
	MechMessageHandler MechSetMessageHandler(MechMessageHandler p_handler);
	// Adds a message to the back of the queue
	void MechPostMessage(MechU32 p_message, size_t p_wParam, MECH_INTPTR p_lParam);
	// Calls the handler now and returns its result, 0 if there is no handler
	MECH_INTPTR MechSendMessage(MechU32 p_message, size_t p_wParam, MECH_INTPTR p_lParam);
	// Copies the oldest queued message numbered from p_first to p_last, or the oldest of all when
	// both are 0, to p_message, and takes it off the queue if p_remove. Returns 0 if there is none.
	int MechPeekMessage(MechMessage* p_message, MechU32 p_first, MechU32 p_last, int p_remove);

#ifdef __cplusplus
}
#endif

#endif // MESSAGES_H
