/* Timed callbacks: a list of TimedCallback entries, each calling its function every m_period
   ticks of the game clock until the function returns 0. */
#include "callbacks.h"

#include "clock.h"
#include "decomp.h"
#include "poolsizes.h"
#include "simmain.h"
#include "staticmem.h"
#include "types.h"

// The callback being created, run or removed.
TimedCallback* g_currentCallback = NULL;

TimedCallback* GetCurrentCallback(void)
{
	return g_currentCallback;
}

TimedCallback* CreateDetachedTask(TimedCallback** p_list, TimedCallbackFn p_fn, MechS32 p_period, MechChar* p_data)
{
	TimedCallback* callback;

	callback = StaticPoolAlloc(sizeof(TimedCallback), g_staticPoolTags[3]);
	if (callback == NULL) {
		return NULL;
	}

	callback->m_fn = p_fn;
	callback->m_data = NULL;
	callback->m_period = p_period;
	callback->m_lastTime = g_currentClock;
	callback->m_nextTime = callback->m_lastTime;
	callback->m_next = *p_list;
	*p_list = callback;
	g_currentCallback = *p_list;
	if (p_fn != NULL && !p_fn(0, p_data, g_currentClock, p_period)) {
		RemoveTask(p_list, g_currentCallback);
		return NULL;
	}

	return callback;
}

void RemoveTask(TimedCallback** p_list, TimedCallback* p_callback)
{
	TimedCallback* callback;

	if (p_callback == NULL) {
		return;
	}

	if (*p_list == p_callback) {
		*p_list = p_callback->m_next;
	}
	else {
		for (callback = *p_list; callback != NULL; callback = callback->m_next) {
			if (callback->m_next == p_callback) {
				callback->m_next = p_callback->m_next;
				break;
			}
		}
	}

	g_currentCallback = p_callback;
	p_callback->m_fn(2, NULL, g_currentClock, p_callback->m_period);
	MechHeapFree(g_primaryHeap, p_callback->m_data);
}

void RemoveAllTasks(TimedCallback** p_list)
{
	TimedCallback* callback;

	for (callback = *p_list; callback != NULL; callback = callback->m_next) {
		RemoveTask(p_list, callback);
	}
}

void SignalTask(TimedCallback** p_list, TimedCallback* p_callback)
{
	p_callback->m_fn(-1, NULL, g_currentClock, p_callback->m_period);
}

void SignalAllTasks(TimedCallback** p_list)
{
	TimedCallback* callback;

	for (callback = *p_list; callback != NULL; callback = callback->m_next) {
		SignalTask(p_list, callback);
	}
}

void** GetCallbackData(TimedCallback* p_callback)
{
	return &p_callback->m_data;
}

void RunTimedCallbacks(TimedCallback** p_list)
{
	for (g_currentCallback = *p_list; g_currentCallback != NULL; g_currentCallback = g_currentCallback->m_next) {
		if (g_currentCallback->m_fn != NULL && g_currentCallback->m_nextTime <= g_currentClock) {
			g_currentCallback->m_nextTime = g_currentCallback->m_period + g_currentClock;
			if (!g_currentCallback->m_fn(1, NULL, g_currentClock, g_currentCallback->m_period)) {
				RemoveTask(p_list, g_currentCallback);
			}
			else {
				g_currentCallback->m_lastTime = g_currentClock;
			}
		}
	}
}

MechS32 GetTimedCallbackSize(void)
{
	return sizeof(TimedCallback);
}
