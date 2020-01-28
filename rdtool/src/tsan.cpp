#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#include <internal/abi.h>
#include <cilk/batcher.h>
#include "debug_util.h" 
#include "mem_access.h" 
#include "rd.h" 
#include "rd_future.h"
#include "list.h"
#include "check_mem.h"
/* defined in omrd.cpp */
extern "C" void 
do_enter_helper_begin(__cilkrts_stack_frame* sf, void* this_fn, void* rip);
extern "C" void do_detach_begin(__cilkrts_stack_frame* parent);
//extern "C" void do_detach_end();
extern "C" void do_enter_begin();
extern "C" int do_enter_end(__cilkrts_stack_frame* sf, void* rsp);
extern "C" void do_sync_begin(__cilkrts_stack_frame* sf);
extern "C" void do_sync_end();
extern "C" int do_leave_begin(__cilkrts_stack_frame *sf);
extern "C" int do_leave_end();
extern "C" int do_leave_stolen(__cilkrts_stack_frame* sf);
//extern "C" void do_steal_success(__cilkrts_worker* w, __cilkrts_worker* victim,
//                                 __cilkrts_stack_frame* sf);
//extern "C" void CilkSteal(StealType type, void *deque, __cilkrts_worker* w, __cilkrts_worker* victim);
// extern "C" void do_read(uint64_t inst_addr, uint64_t addr, size_t mem_size); 
// extern "C" void do_write(uint64_t inst_addr, uint64_t addr, size_t mem_size);
extern "C" void clear_shadow_memory(size_t start, size_t end); 
extern "C" void do_tool_init(void);
extern "C" void do_tool_print(void);
extern "C" void do_tool_destroy(void);

uint32_t getFutureID();
extern __thread __cilkrts_worker *t_worker;

static bool TOOL_INITIALIZED = false;

/* XXX: Honestly these should be part of detector state */
static bool check_enable_instrumentation = true;
// When either is set to false, no errors are output
//__thread
static bool instrumentation = false;

__thread static int checking_disabled = 1;
// a flag indicating that next time we encounter tsan_func_exit, we should
// clear the shadow memory corresponding to a worker's stack; set in
// leave_frame_begin
__thread static bool clear_stack = false;

// need to clear the accesses to C stack off shadow memory; for that purpose,
// need to keep a stack_low_water_mark (stack grows downward)
// the high_water_mark is simply the rbp (frame base pointer value) stored in
// shadow_frame in leave_begin, since we just need to clear the shadow memory
// when we return from a spawn helper.
//
// Unfortunately, we can't keep track of where the stack is using proc_map,
// since during parallel execution, workers use stacks allocated by the
// runtime, which can be anywhere in the heap.
__thread static uint64_t stack_low_watermark = (uint64_t)(-1);

bool should_check_flag = false;
//void print_debug_info(const char *str) {
  //fprintf(stderr, "%s worker: %d\n", str, self);
//}

//#define print_debug_info(str) fprintf(stderr, "%s worker: %d\n", str, self)
#define print_debug_info(str) 

static void tsan_destroy(void) {}

__attribute__((always_inline))
static void enable_instrumentation() {
  DBG_TRACE(DEBUG_BASIC, "Enable instrumentation.\n");
  instrumentation = true;
}

__attribute__((always_inline))
static void disable_instrumentation() {
  DBG_TRACE(DEBUG_BASIC, "Disable instrumentation.\n");
  instrumentation = false;
}

__attribute__((always_inline))
void enable_checking() {
  //checking_disabled--;
  checking_disabled = 0;
  DBG_TRACE(DEBUG_BASIC, "%d: Enable checking.\n", checking_disabled);
  om_assert(checking_disabled >= 0);
}

__attribute__((always_inline))
void disable_checking() {
  om_assert(checking_disabled >= 0);
  //checking_disabled++;
  checking_disabled = 1;
  DBG_TRACE(DEBUG_BASIC, "%d: Disable checking.\n", checking_disabled);
}

extern "C" void __tsan_init()
{
  if(TOOL_INITIALIZED) return;

  atexit(tsan_destroy);
  TOOL_INITIALIZED = true;
  enable_checking();
}

// outside world (including runtime).
// Non-inlined version for user code to use
extern "C" void __om_enable_checking() {
  checking_disabled--;
  om_assert(checking_disabled >= 0);
  DBG_TRACE(DEBUG_BASIC, "External enable checking (%d).\n", checking_disabled);
}

// Non-inlined version for user code to use
extern "C" void __om_disable_checking() {
  om_assert(checking_disabled >= 0);
  checking_disabled++;
  DBG_TRACE(DEBUG_BASIC, "External disable checking (%d).\n", checking_disabled);
}

extern "C" void __om_disable_instrumentation() {
  DBG_TRACE(DEBUG_BASIC, "Disable instrumentation.\n");
  check_enable_instrumentation = false;
  instrumentation = false;
}

__attribute__((always_inline))
static bool should_check() {
  return(instrumentation && checking_disabled == 0); 
}

/* Need to get the stack_low_watermark (where the stack ends) so that
 * we can clean the shadow mem corresponding to cactus stack.
 */
extern "C" void __tsan_func_entry(void *pc){ }

extern "C" void cilk_tool_init(void)
{
  disable_checking();
  //  disable_instrumentation();
  do_tool_init();
  enable_checking();
}

/** @todo this doesn't actually get called...
 */
extern "C" void cilk_tool_print(void)
{
  disable_checking();
  //  disable_instrumentation();
  //do_tool_print();
  // fprintf(stderr, "--- Thread Sanitizer Stats ---\n");
  // fprintf(stderr, "Reads: %zu\n", g_num_reads);
  // fprintf(stderr, "Writes: %zu\n", g_num_writes);
  // fprintf(stderr, "Total Accesses: %zu\n", g_num_reads + g_num_writes);
  enable_checking();
}

extern "C" void cilk_tool_destroy(void)
{
  disable_checking();
  disable_instrumentation();
  do_tool_destroy();
  //enable_checking();
}

/* We would like to clear the shadow memory correponding to the cactus
 * stack whenever we leave a Cilk function.  Unfortunately, variables are 
 * still being read after cilk_leave_frame_begin (such as return value 
 * stored on stack, so we really have to clean the shadow mem for stack 
 * at the vary last moment, right before we return, which seems to be 
 * where tsan_func_exit is called.  So instead, we set a thread-local 
 * flag to tell this worker to clean the shadow mem correponding to its 
 * stack at cilk_leave_begin, but check the flag and actually do the cleaning
 * in __tsan_func_exit.
 */
extern "C" void __tsan_func_exit() { 
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  disable_checking();

  uint64_t res = (uint64_t) __builtin_frame_address(0);
  if(stack_low_watermark > res) {
    stack_low_watermark = res;
    DBG_TRACE(DEBUG_CALLBACK, 
              "tsan_func_exit, set stack_low_watermark %lx.\n", res);
  }

  if(clear_stack) {
    // the spawn helper that's exiting is calling tsan_func_exit, 
    // so the spawn helper's base pointer is the stack_high_watermark
    // to clear (stack grows downward)
    uint64_t stack_high_watermark = (uint64_t)__builtin_frame_address(1);
    DBG_TRACE(DEBUG_CALLBACK, 
              "tsan_func_exit, stack_high_watermark: %lx.\n", 
              stack_high_watermark);
    om_assert( stack_low_watermark != ((uint64_t)-1) );
    om_assert( stack_low_watermark <= stack_high_watermark );

    clear_shadow_memory(stack_low_watermark, stack_high_watermark);
    // now the high watermark becomes the low watermark
    stack_low_watermark = stack_high_watermark;
    clear_stack = 0;
  }
  enable_checking();
}

extern "C" void __tsan_vptr_read(void **vptr_p) {}
extern "C" void __tsan_vptr_update(void **vptr_p, void *new_val) {}


extern "C" void cilk_enter_begin() {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_enter_begin");
  //return;
  disable_checking();
  DBG_TRACE(DEBUG_CALLBACK, "cilk_enter_begin.\n");
  
  do_enter_begin();
  om_assert(TOOL_INITIALIZED);
}

extern "C" void cilk_enter_helper_begin(__cilkrts_stack_frame* sf, 
                                        void* this_fn, void* rip) {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_enter_helper_begin"); 
  //return;
  disable_checking();
  DBG_TRACE(DEBUG_CALLBACK, "cilk_enter_helper_begin.\n");
  do_enter_helper_begin(sf, this_fn, rip);
}

extern "C" void cilk_enter_end(__cilkrts_stack_frame *sf, void *rsp) {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_enter_end"); 
  //return;
  int first_frame = do_enter_end(sf, rsp);
  if(first_frame && __builtin_expect(check_enable_instrumentation, 0)) {
    check_enable_instrumentation = false;
    // turn on instrumentation now for this worker
    enable_instrumentation();
    // reset low watermark here, since we are possibly using a brand new stack
    stack_low_watermark = (uint64_t)(-1);
  }
  enable_checking();
  //  DBG_TRACE(DEBUG_CALLBACK, "leaving cilk_enter_end.\n");
}

extern "C" void cilk_spawn_prepare() {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_spawn_prepare"); 
  //return;
  disable_checking();
  //  DBG_TRACE(DEBUG_CALLBACK, "cilk_spawn_prepare.\n");
}


extern "C" void cilk_steal_success(__cilkrts_worker* w, __cilkrts_worker* victim,
                                   __cilkrts_stack_frame* sf)
{
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  //  DBG_TRACE(DEBUG_CALLBACK, "%i stealing from %i.\n", w->self, victim->self);
  self = w->self;
  print_debug_info("cilk_steal_success"); 
  //return;
  om_assert(!should_check());
  //do_steal_success(w, victim, sf);
  CilkSteal(kNormal, NULL, NULL, w, victim);
}

/// in_continuation == 0 when a spawn helper is called, i.e. the first
/// time that
/// if (!setjmp(...)) { call spawn_helper }
/// is reached. Otherwise, in_continuation == 1, i.e. the runtime
/// resumes the continuation by longjmping
extern "C" void cilk_spawn_or_continue(int in_continuation) {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  //self = __cilkrts_get_tls_worker()->self;
  print_debug_info("cilk_spawn_or_continue"); 
  //return;
  enable_checking();
  DBG_TRACE(DEBUG_CALLBACK, "leaving cilk_spawn_or_continue.\n");
}

extern "C" void 
cilk_detach_begin(__cilkrts_stack_frame *parent_sf) { 
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_detach_begin"); 
  //return;
  disable_checking(); 
  DBG_TRACE(DEBUG_CALLBACK, "cilk_detach_begin.\n");
  do_detach_begin(parent_sf);
}

extern "C" void cilk_detach_end() { 
  //  do_detach_end();
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_detach_end"); 
  //return;
  DBG_TRACE(DEBUG_CALLBACK, "leaving cilk_detach_end.\n");
  enable_checking(); 
}

extern "C" void cilk_sync_begin(__cilkrts_stack_frame* sf) {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_sync_begin"); 
  //return;
  disable_checking();
  DBG_TRACE(DEBUG_CALLBACK, "cilk_sync_begin.\n");
  om_assert(TOOL_INITIALIZED);
  do_sync_begin(sf);
}

extern "C" void cilk_sync_end() {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  om_assert(TOOL_INITIALIZED);
  print_debug_info("cilk_sync_end"); 
  //return;
  do_sync_end();
  DBG_TRACE(DEBUG_CALLBACK, "leaving cilk_sync_end.\n");
  enable_checking();
}

extern "C" void cilk_resume(__cilkrts_stack_frame *sf) { print_debug_info("cilk_resume"); enable_checking(); }

extern "C" void cilk_leave_begin(__cilkrts_stack_frame* sf) {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_leave_begin"); 
  //return;
  disable_checking();
  DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_begin.\n");
  int is_last_frame = do_leave_begin(sf);
  if(is_last_frame) {
    disable_instrumentation();
    check_enable_instrumentation = true;
  }

  if(sf->flags & CILK_FRAME_DETACHED) {
    // This is the point where we need to set flag to clear accesses to stack 
    // out of the shadow memory.  The spawn helper of this leave_begin
    // is about to return, and we want to clear stack accesses below 
    // (and including) spawn helper's stack frame.  Set the flag here
    // and the stack will be cleared in tsan_func_exit.
    clear_stack = true;
  }
}

extern "C" void cilk_leave_end() {
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_leave_end"); 
  //return;
  enable_checking();
  DBG_TRACE(DEBUG_CALLBACK, "leaving cilk_leave_end.\n");
  // int is_last_frame =
  do_leave_end();
}

// This is called when cilkrts_c_THE_exception_check realizes the
// parent is stolen and control should longjmp into the runtime
// (switch to scheduling fiber). Thus we don't need to
// enable_checking, but otherwise need to do the same as cilk_leave_end()
extern "C" void __attribute__((noinline))
cilk_leave_stolen(__cilkrts_worker* w,
                  __cilkrts_stack_frame *saved_sf,
                  int is_original,
                  char* stack_base)
{ 
  //self = w->self;
  //print_debug_info("cilk_leave_stolen");
  //return;
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_leave_stolen"); 
  if (instrumentation) {

    om_assert(clear_stack == true); 

    //  __tsan_func_exit();
    // We can't just call __tsan_func_exit because now the stack now
    //  looks like:
    // Spawning (stolen) function
    // __cilk_spawn_helper
    // __cilkrts_leave_frame
    // __cilkrts_c_THE_exception_check (not when lto enabled)
    // cilk_leave_stolen (not when lto enabled)

    uint64_t stack_high_watermark;
    /// @todo is the 'is_original' necessary? 
    if (is_original)
      stack_high_watermark = (uint64_t)__builtin_frame_address(3);
    else
      stack_high_watermark = (uint64_t)stack_base;

    // DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_stolen, stack_high_watermark: %lx.\n", 
    //           stack_high_watermark);
    om_assert( stack_low_watermark != ((uint64_t)-1) );
    om_assert( stack_low_watermark <= stack_high_watermark );

    clear_shadow_memory(stack_low_watermark, stack_high_watermark);
    // We're jumping back to the runtime, but keep this in case we do a
    // provably good steal
    stack_low_watermark = stack_high_watermark;
    clear_stack = 0;
  }

  do_leave_stolen(saved_sf);
}

// Currently having a linking issue that requires me to define this,
// even though the weak symbol version defined the runtime should be used.
extern "C" void cilk_sync_abandon(__cilkrts_stack_frame* sf) { print_debug_info("cilk_sync_abandon"); }

/** We are done with a stack (fiber), and so should clear the
    associated shadow memory. Originally I wanted to use
    cilk_sync_abandon for this, which already exists as a ZC_INTRINSIC
    call. Unfortunately when cilk_sync_abandon is called, the
    associated fiber has already been freed (the worker is calling
    from a scheduling fiber), so it may already be recycled and in
    use. So I have added the following function, which is called right
    before longjmp_into_runtime from __cilkrts_c_sync.
*/
extern "C" void cilk_done_with_stack(__cilkrts_stack_frame *sf_at_sync,
                                     char* stack_base)
{
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  print_debug_info("cilk_donw_with_stack"); 
  //return;
  om_assert(stack_base != NULL);
  uint64_t stack_high_watermark = (uint64_t)stack_base;
  om_assert(stack_low_watermark != ((uint64_t)-1) );
  om_assert(stack_low_watermark <= stack_high_watermark);
  clear_shadow_memory(stack_low_watermark, stack_high_watermark);
  stack_low_watermark = ((uint64_t)-1);
  clear_stack = 0;
  //  printf("cilk_done_with_stack: w: %i, sf: %p, sb: %p\n", self, sf_at_sync, stack_base);

  // If we got here, then we called cilk_sync_begin with no
  // corresponding cilk_sync_end, so checking is already disabled.
  //  disable_checking();
}

// The worker is about to jump back into user code, either because (1)
// it randomly stole work, or (2) it is resuming a suspended frame.
extern "C" void cilk_continue(__cilkrts_stack_frame* sf, char* new_sp)
{ 
 if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
 //self = sf->worker->self;
 self = __cilkrts_get_tls_worker()->self;
 print_debug_info("cilk_continue"); 
 //return;
 if (new_sp != NULL) {
    stack_low_watermark = (uint64_t) new_sp; //GET_SP(sf);
  }
  //  printf("cilk_continue: w: %i, sf: %p, sp: %p\n", self, sf, new_sp);
  // Set int cilk_spawn_or_continue
  //  enable_checking();
}

typedef void*(*malloc_t)(size_t);
static malloc_t real_malloc = NULL;

extern "C" void* malloc(size_t s) {

  //  DBG_TRACE(DEBUG_CALLBACK, "malloc called.\n");
  // make it 8-byte aligned; easier to erase from shadow mem
  uint64_t new_size = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(s);

  // Don't try to init, since that needs malloc.  
  if (real_malloc==NULL) {
    real_malloc = (malloc_t)dlsym(RTLD_NEXT, "malloc");
    char *error = dlerror();
    if (error != NULL) {
      fputs(error, stderr);
      fflush(stderr);
      abort();
    }
  }
  void *r = real_malloc(new_size);

  //  if (t_worker && t_worker && __cilkrts_get_batch_id(t_worker) != -1) return r;
  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return r;

  if(TOOL_INITIALIZED && should_check()) {
    clear_shadow_memory((size_t)r, (size_t)r+new_size);
  }

  return r;
}

// Assuming tsan_read/write is inlined, the stack should look like this:
//
// -------------------------------------------
// | user func that is about to do a memop   |
// -------------------------------------------
// | __tsan_read/write[1-16]                 |
// -------------------------------------------
// | backtrace (assume __tsan_read/write and | 
// |            get_user_code_rip is inlined)|
// -------------------------------------------
//
// In the user program, __tsan_read/write[1-16] are inlined
// right before the corresponding read / write in the user code.
// the return addr of __tsan_read/write[1-16] is the rip for the read / write
/*
static inline void tsan_read(void *addr, size_t mem_size, void *rip) {
  //  if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  om_assert(TOOL_INITIALIZED);
  if(should_check()) {
#if STATS > 0
    //    __sync_fetch_and_add(&g_num_reads, 1);
#endif
    disable_checking();
    DBG_TRACE(DEBUG_MEMORY, "%s read %p\n", __FUNCTION__, addr);
    om_assert(mem_size <= 16);
    record_mem_helper(true, (uint64_t)rip, (uint64_t)addr, mem_size);
    enable_checking();
  } else {
    DBG_TRACE(DEBUG_MEMORY, "SKIP %s read %p\n", __FUNCTION__, addr);
  }
}

static inline void tsan_write(void *addr, size_t mem_size, void *rip)
{
  //if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return;
  om_assert(TOOL_INITIALIZED);
  if(should_check()) {
#if STATS > 0
    //    __sync_fetch_and_add(&g_num_writes, 1);
#endif
    disable_checking();
    DBG_TRACE(DEBUG_MEMORY, "%s wrote %p\n", __FUNCTION__, addr);
    om_assert(mem_size <= 16);
    record_mem_helper(false, (uint64_t)rip, (uint64_t)addr, mem_size);
    enable_checking();
  } else {
    DBG_TRACE(DEBUG_MEMORY, "SKIP %s wrote %p\n", __FUNCTION__, addr);
  }
}*/
size_t reader[16];
size_t writer[16];

extern size_t total_readers;
extern size_t total_writers;
size_t total_f0 = 0;
size_t actual_total_f0 = 0;

static inline
void tsan_access(bool is_read, void *addr, size_t mem_size, void *rip) {  
#if STATS == 1
  if (is_read) g_reads++;
  else g_writes++;
#endif
  //if (!rd::should_check()) return;
  //if (is_read) reader++;
  //else         writer++;
  //if (is_read) total_readers++;
  //else         total_writers++;
  //if (!instrumentation) total_readers++;
  if (current.list == NULL) return;
  //if (!should_check()) return;
  disable_checking(); 
  //if (is_read) total_readers++;//reader[self]++;
  //else         total_writers++;//writer[self]++;
  //if (current.future_id == 0) total_f0++;
  //if (getFutureID() == 0) total_f0++;
  //if (getFutureID() == 0) actual_total_f0++;

  assert(mem_size <= 16);
  check_access(is_read, (addr_t)rip, (addr_t)addr, mem_size);
  enable_checking();
}

#define ALIGNED(addr,amt) (((uint64_t)(addr) & (amt-1)) == 0)
#define PTRADD(addr,amt) ((void*)((uint64_t)(addr)+(amt)))

// Sometimes clang produces unaligned vector code...I am not sure why.
static inline
void tsan16_access(bool is_read, void* addr, void *rip) {
  // it should at least be 8-byte aligned
  // assert(((uint64_t)addr & 0b111UL) == 0);
  // if (((uint64_t)addr & 0xfUL) == 0) // 16-byte aligned
  //   tsan_access(is_read, addr, 16, __builtin_return_address(0));
  // else
  
  if (!should_check()) return;
  disable_checking();
  assert(ALIGNED(addr,4));
  if (ALIGNED(addr,16))
    tsan_access(is_read, addr, 16, __builtin_return_address(0));
  else if (ALIGNED(addr,8)) {
    tsan_access(is_read, addr, 8, __builtin_return_address(0));
    tsan_access(is_read, PTRADD(addr,8), 8, __builtin_return_address(0));
  } else { // 4 byte aligned
    tsan_access(is_read, addr, 4, __builtin_return_address(0));
    tsan_access(is_read, PTRADD(addr,4), 4, __builtin_return_address(0));
    tsan_access(is_read, PTRADD(addr,8), 4, __builtin_return_address(0));
    tsan_access(is_read, PTRADD(addr,12), 4, __builtin_return_address(0));
  }
  enable_checking();
}


extern "C" void 
__tsan_read1(void *addr) { tsan_access(true, addr, 1, __builtin_return_address(0)); }
extern "C" void 
__tsan_read2(void *addr) { tsan_access(true, addr, 2, __builtin_return_address(0)); }
extern "C" void 
__tsan_read4(void *addr) { tsan_access(true, addr, 4, __builtin_return_address(0)); }
extern "C" void 
__tsan_read8(void *addr) { tsan_access(true, addr, 8, __builtin_return_address(0)); }
extern "C" void 
__tsan_read16(void *addr) { tsan16_access(true, addr, __builtin_return_address(0)); }
extern "C" void 
__tsan_write1(void *addr) { tsan_access(false, addr, 1, __builtin_return_address(0)); }
extern "C" void 
__tsan_write2(void *addr) { tsan_access(false, addr, 2, __builtin_return_address(0)); }
extern "C" void 
__tsan_write4(void *addr) { tsan_access(false, addr, 4, __builtin_return_address(0)); }
extern "C" void 
__tsan_write8(void *addr) { tsan_access(false, addr, 8, __builtin_return_address(0)); }
extern "C" void 
__tsan_write16(void *addr) { tsan16_access(false, addr, __builtin_return_address(0)); }

/*
typedef enum {
  __tsan_memory_order_relaxed,
  __tsan_memory_order_consume,
  __tsan_memory_order_acquire,
  __tsan_memory_order_release,
  __tsan_memory_order_acq_rel,
  __tsan_memory_order_seq_cst
} __tsan_memory_order;

typedef long     __tsan_atomic64;  // NOLINT

extern "C" __tsan_atomic64 __tsan_atomic64_exchange(volatile __tsan_atomic64 *a,
    __tsan_atomic64 v, __tsan_memory_order mo) { 
    printf("tsan_exchange\n");
    return 1024;
}

extern "C" __tsan_atomic64 __tsan_atomic64_compare_exchange_val(
    volatile __tsan_atomic64 *a, __tsan_atomic64 c, __tsan_atomic64 v,
    __tsan_memory_order mo, __tsan_memory_order fail_mo) { 
    printf("tsan_exchange_compare\n");
    return 1024;
}*/

extern "C" bool __a_compare_exchange_n(__cilkrts_deque_link *volatile *volatile tail, __cilkrts_deque_link **t, __cilkrts_deque_link *l, bool b, int sm, int fm) {
    return __atomic_compare_exchange_n(tail, t, l, b, sm, fm);
}


extern "C" __cilkrts_deque_link* __a_exchange_n(__cilkrts_deque_link *volatile *volatile tail, __cilkrts_deque_link *p, int m) {
    return __atomic_exchange_n(tail, p, m); 
}

