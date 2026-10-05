#ifndef APP_H
#define APP_H

// The window the game runs in, implemented on the Rust side (src/app). The game keeps its own
// loops and calls MechAppPump from them where it used to look for window messages. What happens
// to the window that the game cares about is posted to the message queue (messages.h).

#ifdef __cplusplus
extern "C"
{
#endif

	// Lets the window handle the events that are waiting for it, without blocking. Does not return
	// once the window is closed.
	void MechAppPump(void);
	// The same, after waiting for an event
	void MechAppWait(void);
	// Non-zero while the window has the keyboard focus
	int MechAppActive(void);

#ifdef __cplusplus
}
#endif

#endif // APP_H
