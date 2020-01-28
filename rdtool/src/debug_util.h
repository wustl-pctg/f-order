#ifndef __DEBUG_UTIL_H__
#define __DEBUG_UTIL_H__

#include <assert.h>

// debug_level is a bitmap
//   1 is basic debugging (old level 1)
//   2 is debug the backtrace
enum debug_levels {
  DEBUG_NONE = 0,
  DEBUG_BASIC      = 1,
  DEBUG_BACKTRACE  = 2,
  DEBUG_CALLBACK   = 4,
  DEBUG_MEMORY     = 8
};

__attribute__((noreturn)) 
void die(const char *fmt, ...);
void debug_printf(int level, const char *fmt, ...);

#if TOOL_DEBUG > DEBUG_NONE
static int debug_level = 0;//DEBUG_CALLBACK; //| DEBUG_MEMORY;
#else
static int debug_level = 0;
#endif

#if TOOL_DEBUG > DEBUG_NONE
#define WHEN_TOOL_DEBUG(stmt) do { stmt } while(0);
#define om_assert(c)                                          \
  do { if (!(c)) { die("%s:%d assertion failure: %s\n",       \
                       __FILE__, __LINE__, #c);} } while (0)
#define locked_assert(c)
// A deadlock involved getting the worker deque lock in batchify
// prevents using this
/* do { pthread_spin_lock(&g_worker_mutexes[self]); \ */
/* assert(c); \ */
/* pthread_spin_unlock(&g_worker_mutexes[self]); \ */
/* } while (0) */
#else
#define WHEN_TOOL_DEBUG(stmt)
#define om_assert(c)
#define locked_assert(c)
#endif

#if TOOL_DEBUG > DEBUG_NONE
// debugging assert to check that the tool is catching all the runtime events
// that are supposed to match up (i.e., has event begin and event end)
enum EventType_t { ENTER_FRAME = 1, ENTER_HELPER = 2, SPAWN_PREPARE = 3,
                   DETACH = 4, CILK_SYNC = 5, LEAVE_FRAME_OR_HELPER = 6,
                   RUNTIME_LOOP = 7, NONE = 8 };
#endif



#if TOOL_DEBUG > DEBUG_BASIC
#define DBG_TRACE(level,...) debug_printf(level, __VA_ARGS__)
#else
#define DBG_TRACE(level,...)
#endif

#endif
