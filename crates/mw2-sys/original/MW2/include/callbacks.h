#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "decomp.h"
#include "types.h"

// The callback's event: 0 when created, 1 when due, 2 when removed, -1 from SignalTask.
// Its data is the task text of the BWD stream's task record (BwdExecuteStream).
typedef MechS32 (*TimedCallbackFn)(MechS32 p_event, MechChar* p_data, MechS32 p_clock, MechS32 p_period);

// SIZE 0x18
typedef struct TimedCallback {
	TimedCallbackFn m_fn;         // 0x00
	void* m_data;                 // 0x04
	MechS32 m_period;             // 0x08
	MechS32 m_lastTime;           // 0x0c
	MechS32 m_nextTime;           // 0x10
	struct TimedCallback* m_next; // 0x14
} TimedCallback;

// The functions and globals of callbacks.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern TimedCallback* g_currentCallback;

	TimedCallback* GetCurrentCallback(void);
	TimedCallback* CreateDetachedTask(TimedCallback** p_list, TimedCallbackFn p_fn, MechS32 p_period, MechChar* p_data);
	void RemoveTask(TimedCallback** p_list, TimedCallback* p_callback);
	void RemoveAllTasks(TimedCallback** p_list);
	void SignalTask(TimedCallback** p_list, TimedCallback* p_callback);
	void SignalAllTasks(TimedCallback** p_list);
	void** GetCallbackData(TimedCallback* p_callback);
	void RunTimedCallbacks(TimedCallback** p_list);
	MechS32 GetTimedCallbackSize(void);

#ifdef __cplusplus
}
#endif

#endif // CALLBACKS_H
