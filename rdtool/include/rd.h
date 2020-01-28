#ifndef _RD_H
#define _RD_H

#include <pthread.h>
#include <stdint.h>
#include <malloc.h> // for memalign

extern "C" void cilk_tool_print(void);
extern "C" void cilk_tool_destroy(void);
extern "C" void __om_enable_checking(); 
extern "C" void __om_disable_checking();
extern "C" void __om_disable_instrumentation();
extern "C" void __om_init();

extern "C" void record_mem_helper(bool is_read, uint64_t inst_addr, uint64_t addr,
                                  uint32_t mem_size);


#define PADDING char pad[(64 - sizeof(pthread_spinlock_t))]

//enum frame_t { FRAME_USER = 0, FRAME_HELPER };
#define FRAME_HELPER_MASK 0x1
#define FRAME_FULL_MASK 0x2

typedef struct local_mut_s {
  pthread_spinlock_t mut;
  PADDING;
} local_mut;

// If you try to grab one of these while owning a worker lock, e.g. in
// cilk_steal_success, you will deadlock if there is a batch relabel
// in progress.
//pthread_spinlock_t* g_worker_mutexes; 
//local_mut* g_worker_mutexes;

#endif // _RD_H
