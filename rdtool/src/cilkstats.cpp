#include <cstdio>
#include <cstdlib>
#include <internal/abi.h>
#include "cilkstats.h"

#define TOOL_ASSERT(x) assert(x) /// @todo refactor

// Allow programers to disable checking for a particular thread.
__thread static int t_checking_disabled = 1;
__attribute__((always_inline)) static void disable_checking() {
  TOOL_ASSERT(t_checking_disabled >= 0);
  t_checking_disabled++; 
}
__attribute__((always_inline)) static void enable_checking() {
  TOOL_ASSERT(t_checking_disabled > 0);
  t_checking_disabled--;
}
void __cilkstats_disable_checking() { disable_checking(); }
void __cilkstats_enable_checking() { enable_checking(); }

// Global instrumentation enable/disable
static bool g_instr_enabled = false;
static inline bool should_check() { return g_instr_enabled && (t_checking_disabled == 0);  }

struct stats_s {
  size_t num_reads;
  size_t num_writes;
  size_t num_spawns;
  size_t num_syncs;
  size_t num_cilk_functions;
} g_stats = {0};

#define GET(field) ({ __sync_synchronize(); g_stats.field; })
#define INCR(field) __sync_fetch_and_add(&g_stats.field, 1)

// Define public getters.
#define GETTER(field) size_t __cilkstats_get_ ## field () { return GET(field); }
GETTER(num_reads)
GETTER(num_writes)
GETTER(num_spawns)
GETTER(num_syncs)
GETTER(num_cilk_functions)
size_t __cilkstats_get_num_mem_accesses() { 
  __sync_synchronize(); 
  return GET(num_reads) + GET(num_writes); 
}

static void print_stats(FILE *output = stdout) {
  fprintf(output, "----- Cilk Stats -----\n");

  size_t num_mem_accesses = __cilkstats_get_num_mem_accesses();
  if (num_mem_accesses > 0) {
    fprintf(output, "Reads: %zu\n", GET(num_reads));
    fprintf(output, "Writes: %zu\n", GET(num_writes));
    fprintf(output, "Total memory accesses: %zu\n", num_mem_accesses);
  } else {
    fprintf(output,
	    "No memory accesses reported: "
	    "Not compiled with -fsanitize=thread or manually disabled.\n");
  }

  size_t num_cilk_functions = GET(num_cilk_functions);
  if (num_cilk_functions > 0) {
    fprintf(output, "Spawns: %zu\n", GET(num_spawns));
    fprintf(output, "Syncs: %zu\n", GET(num_syncs));
    fprintf(output, "Cilk functions: %zu\n", GET(num_cilk_functions));
  } else {
    fprintf(output, "No cilk functions reported: "
	    "Not compiled with -fcilktool or no cilk functions were entered.\n");
  }
}

extern "C" {

/*** Cilk tool stat collection (spawns, syncs, and cilk functions) ***/

void cilk_tool_init(void) { }
void cilk_tool_destroy(void) { }

void cilk_enter_begin() {
  disable_checking(); 
  INCR(num_cilk_functions);
}
void cilk_enter_helper_begin(__cilkrts_stack_frame* sf, void* this_fn, void* rip) { 
  disable_checking(); 
  INCR(num_spawns);
}
void cilk_enter_end(__cilkrts_stack_frame *sf, void *rsp) { enable_checking(); }

void cilk_sync_begin(__cilkrts_stack_frame* sf) {
  disable_checking();
  INCR(num_syncs);
}
void cilk_sync_end() { enable_checking(); }
void cilk_resume(__cilkrts_stack_frame* sf) { enable_checking(); }

void cilk_spawn_prepare() { disable_checking(); }
void cilk_spawn_or_continue(int in_continuation) { enable_checking(); }
//void cilk_continue(__cilkrts_stack_frame* sf, char* new_sp) { }

void cilk_detach_begin(__cilkrts_stack_frame* sf, void* this_fn, void* rip) { disable_checking(); }
void cilk_detach_end() { enable_checking(); }

void cilk_leave_begin(__cilkrts_stack_frame* sf) { disable_checking(); }
void cilk_leave_end() { enable_checking(); }

void cilk_steal_success(__cilkrts_worker *w, __cilkrts_worker *victim,
			__cilkrts_stack_frame *sf) {
  TOOL_ASSERT(t_checking_disabled > 0);
  // We don't need to enable checking here b/c this worker will
  // immediately execute cilk_spawn_or_continue
}

/*** Thread sanitizer stat collection (memory accesses) ***/

void __tsan_destroy() { g_instr_enabled = false; print_stats(); }
void __tsan_init() {

  // For some reason __tsan_init gets called twice...
  static int init = 0;
  if (init) return;
  init = 1;

  atexit(__tsan_destroy);
  g_instr_enabled = true; 
  enable_checking();
}

static inline void
tsan_read(void *addr, size_t mem_size, void *rip) { 
  if (should_check()) INCR(num_reads);
}

static inline void
tsan_write(void *addr, size_t mem_size, void *rip) {
  if (should_check()) INCR(num_writes);
}

void __tsan_read1(void *addr) { tsan_read(addr, 1, __builtin_return_address(0)); }
void __tsan_read2(void *addr) { tsan_read(addr, 2, __builtin_return_address(0)); }
void __tsan_read4(void *addr) { tsan_read(addr, 4, __builtin_return_address(0)); }
void __tsan_read8(void *addr) { tsan_read(addr, 8, __builtin_return_address(0)); }
void __tsan_read16(void *addr) { tsan_read(addr, 16, __builtin_return_address(0)); }
void __tsan_write1(void *addr) { tsan_write(addr, 1, __builtin_return_address(0)); }
void __tsan_write2(void *addr) { tsan_write(addr, 2, __builtin_return_address(0)); }
void __tsan_write4(void *addr) { tsan_write(addr, 4, __builtin_return_address(0)); }
void __tsan_write8(void *addr) { tsan_write(addr, 8, __builtin_return_address(0)); }
void __tsan_write16(void *addr) { tsan_write(addr, 16, __builtin_return_address(0)); }
void __tsan_func_entry(void *pc){ }
void __tsan_func_exit() { }
void __tsan_vptr_read(void **vptr_p) {}
void __tsan_vptr_update(void **vptr_p, void *new_val) {}

} // extern C
