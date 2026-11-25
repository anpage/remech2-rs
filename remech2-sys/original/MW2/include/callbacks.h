#ifndef CALLBACKS_H
#define CALLBACKS_H

#include "types.h"

// The callback's event: 0 when created, 1 when due, 2 when removed, -1 from SignalTask.
// Its data is the task text of the BWD stream's task record (BwdExecuteStream).
typedef int32_t (*TimedCallbackFn)(int32_t p_event, char* p_data, int32_t p_clock, int32_t p_period);

typedef struct TimedCallback {
	TimedCallbackFn m_fn;
	void* m_data;
	int32_t m_period;
	int32_t m_lastTime;
	int32_t m_nextTime;
	struct TimedCallback* m_next;
} TimedCallback;

// The functions and globals of callbacks.c that other units use.
#ifdef __cplusplus
extern "C"
{
#endif

	extern TimedCallback* g_currentCallback;

	TimedCallback* GetCurrentCallback(void);
	TimedCallback* CreateDetachedTask(TimedCallback** p_list, TimedCallbackFn p_fn, int32_t p_period, char* p_data);
	void RemoveTask(TimedCallback** p_list, TimedCallback* p_callback);
	void RemoveAllTasks(TimedCallback** p_list);
	void SignalTask(TimedCallback** p_list, TimedCallback* p_callback);
	void SignalAllTasks(TimedCallback** p_list);
	void** GetCallbackData(TimedCallback* p_callback);
	void RunTimedCallbacks(TimedCallback** p_list);
	int32_t GetTimedCallbackSize(void);

#ifdef __cplusplus
}
#endif

#endif // CALLBACKS_H
