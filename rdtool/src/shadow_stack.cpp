#include <iostream>
#include <cstdlib> // for getenv

#include <internal/abi.h>
#include <cilk/batcher.h>
#include "rd.h"
#include "print_addr.h"
#include "shadow_mem.h"
#include "mem_access.h"

#include "stack.h"
#include "stat_util.h"

#include "omrd.cpp" /// Hack @todo fix linking errors
#include "om/blist.h"

#include <pthread.h>
#include "list.h"
#include "pool.h"
#include "rd_future.h"
#include "test_interface.h"
#include "current.h"
#include <atomic>
//#define print_debug_info(str) fprintf(stderr, "%s worker: %d\n", str, self)
#define print_debug_info(str) 

int g_tool_init = 0;
int g_first_frame = 1;
__thread bool called_future = false;
extern bool should_check_flag;

Memory g_memory;
__thread PrivateMemory p_memory = { NULL, 0, 0, NULL, 0, 0 };

void *g_deque = NULL;
__thread Current current;
std::atomic<uint32_t> future_id;
typedef struct FrameData_s {
  om_node* current_english;
  om_node* current_hebrew;
  om_node* cont_english;
  om_node* cont_hebrew;
  om_node* sync_english;
  om_node* sync_hebrew;
  om_node* steal_cont_english;
  om_node* steal_cont_hebrew;
  om_node* steal_sync_english;
  om_node* steal_sync_hebrew;
  List *steal_current_list;

  List *current_list;
  List *sync_list;
  List *ori_list;
  //pthread_spinlock_t sync_lock;
  uint32_t flags;
  //size_t id;
  om_node* dag;
  uint32_t future_id;
} FrameData_t;

//AtomicStack_t<FrameData_t>* frames;
Pool<AtomicStack_t<FrameData_t>> *pools;

extern size_t reader[16];
extern size_t writer[16];
size_t total_readers = 0;
size_t total_writers = 0;

extern "C" void do_tool_init(void);
extern "C" bool FuturePrecedes(om_node *e1, om_node *h1, om_node *e2, om_node *h2, List *list);
extern "C" void __cilkrts_reset_FOM_SD();
//FrameData_t* get_frame() { return frames[self].head(); }

void *ExternalRecordE() {
    return pools[self].active_deque_->head()->current_english;
}

uint32_t getFutureID() {
    FrameData_t *f = pools[self].active_deque_->head();
    return f->future_id;
}

void *ExternalRecordH() {
    return pools[self].active_deque_->head()->current_hebrew;
}

bool ExternalPrecedes(void *e1, void *h1, void *e2, void *h2) {
    return om_precedes((om_node*)e1, (om_node*)e2) && om_precedes((om_node*)h1, (om_node*)h2);
}

//query procedure
bool PrecedesOM(Frame a, Frame b) {
    return om_precedes(a.english, b.english) && om_precedes(a.hebrew, b.hebrew);
}

bool PrecedesFuture(Frame a, Frame b) {
    return true;
    if (a.dag == b.dag || PrecedesOM(a, b)) {
        return true;
    } else {
        //return b.list->Search(a.english, a.hebrew, a.dag);
        return false;
    }
}
size_t total_query = 0;
size_t total_nonsp_query = 0;
bool Precedes(Current a, Current b) {
    //total_query++;
    if (a.future_id == b.future_id 
        && om_precedes(a.english, b.english)
        && om_precedes(a.hebrew, b.hebrew)) {
        return true;
    } else {
        //total_nonsp_query++;
        //return b.list->Search(a.english, a.hebrew, a.dag);
        return b.list->Search(a.english, a.hebrew, a.future_id);
        //return false;
    }
}



bool ExternalFuturePrecedes(void *e1, void *h1) {
    FrameData_t *f = pools[self].active_deque_->head();
    return FuturePrecedes((om_node*)e1, (om_node*)h1, f->current_english, f->current_hebrew, f->current_list);
}

Frame Record() {
    FrameData_t *f = pools[self].active_deque_->head();
    Frame frame;
    frame.english = f->current_english;
    frame.hebrew  = f->current_hebrew;
    frame.dag      = f->dag;
    frame.list    = f->current_list;
    //std::cout << (size_t)f->current_english << std::endl;
    //std::cout << f->id << std::endl;
    return frame;
}

void ExternalPrint() {
  return;
  fprintf(stderr, "PrintList is called by worker: %d\n", self);
  pools[self].active_deque_->head()->current_list->PrintList();
}

void print_debug_info(const char *str);

void insertion(om_node *node) {
    __cilkrts_worker* w = __cilkrts_get_tls_worker();
    t_worker = w;
    for (int i = 0; i < 100; i++) {
        g_english->insert(w, node);
    }
}

int ExternalTest() {
    __cilkrts_worker* w = __cilkrts_get_tls_worker();
    t_worker = w; 
    //do_tool_init();
    print_debug_info("external test");    
    om_node* base = g_english->get_base();
    om_node* node1 = g_english->insert(w,base);
    om_node* node2 = g_english->insert(w,base);
    om_node* node3 = g_english->insert(w,base);
    om_node* node4 = g_english->insert(w,base);
    om_node* node5 = g_english->insert(w,base);

    cilk_spawn insertion(node1);
    cilk_spawn insertion(node2);
    cilk_spawn insertion(node3);
    //cilk_spawn insertion(node4);
    
    cilk_sync;

    if (om_precedes(node5, node2)) fprintf(stderr, "correct\n");

    //for (int i = 0; i < 500; i++) {
    //    g_english->insert(w, base);
        //fprintf(stderr, "inserted\n");
    //}
    self = __cilkrts_get_tls_worker()->self;
    print_debug_info("external test done");
    return 0;
}

__attribute__((always_inline)) void enable_checking();
__attribute__((always_inline)) void disable_checking();

// XXX Need to synchronize access to it when update
ShadowMem<MemAccessList_t> shadow_mem;

void init_strand(__cilkrts_worker* w, FrameData_t* init)
{
  self = w->self;
  t_worker = w;
  //return;
  FrameData_t* f;
  if (init) {
    f = pools[w->self].active_deque_->head();
    assert(!(init->flags & FRAME_HELPER_MASK));
    //assert(!init->current_english);
    //assert(!init->current_hebrew);

    //    pthread_spin_lock(&g_worker_mutexes[self]);
    if (init->steal_cont_english != NULL) {
        f->current_english = init->steal_cont_english;
        f->current_hebrew  = init->steal_cont_hebrew;
        f->sync_english    = init->steal_sync_english;
        f->sync_hebrew     = init->steal_sync_hebrew;
        f->current_list    = init->steal_current_list;
    } else {
        f->current_english = init->cont_english;
        f->current_hebrew  = init->cont_hebrew;
    }

    f->cont_english    = NULL;
    f->cont_hebrew     = NULL;
    //f->current_english = init->cont_english;
    //f->current_hebrew = init->cont_hebrew;
    //f->sync_english = init->sync_english;
    //f->sync_hebrew = init->sync_hebrew;
    //f->cont_english = NULL;
    //f->cont_hebrew = NULL;
    //f->flags = init->flags;
    //    pthread_spin_unlock(&g_worker_mutexes[self]);
  }  else {
    assert(pools[w->self].active_deque_->empty());
    pools[w->self].active_deque_->push();
    f = pools[w->self].active_deque_->head();
    f->flags = 0;

    f->current_english = g_english->get_base();
    f->current_hebrew = g_hebrew->get_base();
    f->cont_english = NULL;
    f->cont_hebrew = NULL;
    f->sync_english = NULL;
    f->sync_hebrew = NULL;
    f->current_list = new List();
    
    //if (f->current_english->head == NULL) {
    //    ListPointer *lp = new ListPointer;
    //    lp->p = f->current_list;
    //    lp->next = NULL;
    //    f->current_english->head = lp;
    //} else {
    //    ListPointer *lp = new ListPointer;
    //    lp->p = f->current_list;
    //    lp->next = f->current_english->head;
    //    f->current_english->head = lp;
    //}
    
    f->sync_list = NULL;
    //f->id = 0;
    f->dag = f->current_english;
    f->future_id = future_id.fetch_add(1); 
    g_deque = (void*)pools[w->self].active_deque_;
    if (p_memory.mem == NULL) p_memory = g_memory.grab_mem();
    current.english = f->current_english;
    current.hebrew  = f->current_hebrew;
    current.list    = f->current_list;
    //current.dag     = f->dag;
    current.future_id = f->future_id;
  }
}


extern "C" void do_tool_init(void) 
{
  //static bool init = false;
  //if (init == true) return;
  //init = true;
  if (g_tool_init == 1) return;

  //std::cout << "cilk_tool_init" << std::endl;
  
  for (int i = 0; i < 16; i++) {
    reader[i] = 0;
    writer[i] = 0;
  }

  int p = __cilkrts_get_nworkers();
  g_memory.init(p, 20000000 * 16 / p);

  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_init called.\n");
#if STATS > 1
  size_t num_events = NUM_INTERVAL_TYPES * p;
  g_timing = new struct padded_time[num_events]();
  // g_timing_events = new struct padded_time[num_events];
  // g_timing_event_starts = new struct padded_time[num_events];
  // g_timing_event_ends = new struct padded_time[num_events];
  for (int i = 0; i < num_events; ++i) {
    g_timing[i].start = g_timing[i].elapsed = 0;
    // g_timing_events[i].t = EMPTY_TIME_POINT;
    // g_timing_event_starts[i].t = EMPTY_TIME_POINT;
    // g_timing_event_ends[i].t = EMPTY_TIME_POINT;
  }
#endif
  //frames = new AtomicStack_t<FrameData_t>[p];
  pools = new Pool<AtomicStack_t<FrameData_t>>[p];
  //  g_worker_mutexes = new pthread_spinlock_t[p];
  g_worker_mutexes = (local_mut*)memalign(64, sizeof(local_mut)*p);
  for (int i = 0; i < p; ++i) {
    pthread_spin_init(&g_worker_mutexes[i].mut, PTHREAD_PROCESS_PRIVATE);
  }
  //pthread_spin_init(&g_relabel_mutex, PTHREAD_PROCESS_PRIVATE);
  __cilkrts_mutex_init(&g_relabel_mutex);
  g_english = new omrd_t();
  g_hebrew = new omrd_t();

  char *s_thresh = getenv("HEAVY_THRESHOLD");
  if (s_thresh) {
    label_t thresh = (label_t)atol(getenv("HEAVY_THRESHOLD"));
    g_english->set_heavy_threshold(thresh);
    g_hebrew->set_heavy_threshold(thresh);
  }

  // XXX: Can I assume this is called before everything?
  //read_proc_maps();
  g_tool_init = 1;
  should_check_flag = true;
  //fprintf(stderr, "init done!\n");

  //  RDTOOL_INTERVAL_BEGIN(TOOL);
  // g_timing[0].start = rdtsc();
  // fprintf(stderr, "rdtsc: %llu\n", rdtsc());
}

extern size_t g_num_relabel_lock_tries;

/*
extern "C" void do_tool_print(void)
{
  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_print called.\n");
  // g_timing[0].elapsed = rdtsc() - g_timing[0].start;
  // fprintf(stderr, "rdtsc: %llu\n", rdtsc());
  // int self = 0;
  //  RDTOOL_INTERVAL_END(TOOL);
  // fprintf(stderr, "real: %lu\n", g_timing[0].elapsed);
  // fprintf(stderr, "real2: %lf\n", g_timing[0].elapsed / (CPU_MHZ * 1000));
  // fprintf(stderr, "Time: %lf\n", GET_TIME_MS(TOOL));

#if STATS > 0
  int p = __cilkrts_get_nworkers();
#endif
#if STATS > 1
  for (int itype = 0; itype < NUM_INTERVAL_TYPES; ++itype) {
    char s[32];
    if (g_interval_strings[itype] == NULL) continue;
    strncpy(s, g_interval_strings[itype], 31);
    strcat(s, ":");
    fprintf(stderr, "%-20s\t", s);
    
    long double total = 0;
    for (int i = 0; i < p; ++i) {
      int self = i;
      long double t = GET_TIME_MS(itype);
      fprintf(stderr, "% 10.2Lf", t);
      total += t;
    }
    fprintf(stderr, "\n");
    fprintf(stderr, "Total %s:\t%Lf\n", g_interval_strings[itype], total);
  }

  // fprintf(stderr, "\nTotal relabel lock acquires:\t%zu\n",
  // g_num_relabel_lock_tries);
  // double insert_time = 0;
  // double query_time = 0;
  // for (int i = 0; i < p; ++i) {
  //   int self = i;
  //   fprintf(stderr, "Index for %d is %d\n", i, TIDX(INSERT));
  //   insert_time += (g_timing_events[TIDX(INSERT)].tv_sec * 1000.0) +
  //     (g_timing_events[TIDX(INSERT)].tv_nsec / 1000000.0);
  //   query_time += (g_timing_events[TIDX(QUERY)].tv_sec * 1000.0) +
  //     (g_timing_events[TIDX(QUERY)].tv_nsec / 1000000.0);
  // }
  // fprintf(stderr, "%-20s\t%lf\n", "Total insert time:", insert_time);
  // fprintf(stderr, "%-20s\t%lf\n", "Total query time:", query_time);


#endif
#if STATS > 0  
  std::cout << "Num relabels: " << g_num_relabels << std::endl;

  size_t min = (unsigned long)-1;
  size_t max = 0;
  size_t total = 0;
  for (int i = 0; i < g_num_relabels; ++i) {
    int n = g_heavy_node_info[i];
    if (n < min) min = n;
    if (n > max) max = n;
    total += n;
  }
  fprintf(stderr, "Heavy node min: %zu\n", min);
  fprintf(stderr, "Heavy node max: %zu\n", max);
  fprintf(stderr, "Heavy node median: %zu\n", g_heavy_node_info[g_num_relabels/2]);
  fprintf(stderr, "Heavy node mean: %.2lf\n", total / (double)g_num_relabels);
  fprintf(stderr, "Heavy node total: %zu\n", total);
  // std::cout << "Num inserts: " << g_num_inserts << std::endl;
  // std::cout << "Avg size per relabel: " << (double)g_relabel_size / (double)g_num_relabels << std::endl;
  std::cout << "English OM memory used: " << g_english->memsize() << std::endl;
  std::cout << "Hebrew OM memory used: " << g_hebrew->memsize() << std::endl;
  size_t sssize = 0;
  for (int i = 0; i < p; ++i) sssize += frames[i].memsize();
  std::cout << "Shadow stack(s) total size: " << sssize << std::endl;
  // fprintf(stderr, "--- Thread Sanitizer Stats ---\n");
  // fprintf(stderr, "Reads: %zu\n", g_num_reads);
  // fprintf(stderr, "Writes: %zu\n", g_num_writes);
  // fprintf(stderr, "Total Accesses: %zu\n", g_num_reads + g_num_writes);
  // fprintf(stderr, "Total Queries: %zu\n", g_num_reads + g_num_queries);
  //  fprintf(stderr, "MemAccess_ts allocated: %zu\n", num_new_memaccess);
#endif

}*/

extern "C" void do_tool_destroy(void) 
{
  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_destroy called.\n");

  //do_tool_print();
  //std::cout << "cilk_tool_destroy" << std::endl;
  g_tool_init = 0;
  g_first_frame = 1;
  delete g_hebrew;
  delete g_english;
  //delete[] frames;
  delete[] pools;
  pools = NULL;
  //assert(0);
  g_memory.destroy();

  for (int i = 0; i < 16; i++) {
    total_readers += reader[i];
    total_writers += writer[i];
  }

  __cilkrts_reset_FOM_SD();

  for (int i = 0; i < __cilkrts_get_nworkers(); ++i) {
    pthread_spin_destroy(&g_worker_mutexes[i].mut);
  }
  //pthread_spin_destroy(&g_relabel_mutex);
  __cilkrts_mutex_destroy(t_worker, &g_relabel_mutex);

  //  delete[] g_worker_mutexes;
  free(g_worker_mutexes);
#if STATS > 1
  delete[] g_timing;
  // delete[] g_timing_events;
  // delete[] g_timing_event_starts;
  // delete[] g_timing_event_ends;
#endif
  //  delete_proc_maps(); /// @todo shakespeare has problems compiling print_addr.cpp
}

extern "C" om_node* get_current_english()
{
  __cilkrts_worker* w = __cilkrts_get_tls_worker();
  if (!pools) return NULL;
  return pools[w->self].active_deque_->head()->current_english;
}

extern "C" om_node* get_current_hebrew()
{
  __cilkrts_worker* w = __cilkrts_get_tls_worker();
  if (!pools) return NULL;
  return pools[w->self].active_deque_->head()->current_hebrew;
}


extern "C" AtomicStack_t<FrameData_t>* do_steal_success(__cilkrts_worker* w, __cilkrts_worker* victim,
                                 __cilkrts_stack_frame* sf, AtomicStack_t<FrameData_t> *victim_deque)
{
  //return;
  DBG_TRACE(DEBUG_CALLBACK, 
            "%d: do_steal_success, stole %p from %d.\n", w->self, sf, victim->self);
  //frames[w->self].reset();
  //FrameData_t* loot = frames[victim->self].steal_top(frames[w->self]);
  //om_assert(!(loot->flags & FRAME_HELPER_MASK));
  //init_strand(w, loot);
  if (pools[w->self].active_deque_ == NULL) pools[w->self].NewDeque();
  AtomicStack_t<FrameData_t>* self_deque = pools[w->self].active_deque_;
  self_deque->reset();
  FrameData_t* loot = victim_deque->steal_top(*self_deque);
  init_strand(w, loot);//is this necessary?
  return self_deque;
}

extern "C" void cilk_return_to_first_frame(__cilkrts_worker* w,
                                           __cilkrts_worker* team,
                                           __cilkrts_stack_frame* sf)
{
 
  //fprintf(stderr, "cilk_return_to_first_frame worker: %d\n", team->self);
  //return;
  DBG_TRACE(DEBUG_CALLBACK, 
            "%d: Transfering shadow stack %p to original worker %d.\n", 
            w->self, sf, team->self);
  //if (w->self != team->self) frames[w->self].transfer(frames[team->self]);
  //return;
  if (pools == NULL) return; 
  if (w->self != team->self) {
    if (pools[team->self].active_deque_ == NULL) pools[team->self].NewDeque();
    pools[w->self].active_deque_->transfer(*pools[team->self].active_deque_);
  }
}

extern "C" void do_enter_begin()
{
  //return;
  //if (g_first_frame == 1) assert(0);
  if (self == -1 || g_first_frame == 1) { 
    return; 
  }
  pools[self].active_deque_->push();
  //DBG_TRACE(DEBUG_CALLBACK, "%d: do_enter_begin, new size: %d.\n", 
  //          self, frames[self].size());
  FrameData_t* parent = pools[self].active_deque_->ancestor(1);
  FrameData_t* f = pools[self].active_deque_->head();
  f->flags = 0;
  f->cont_english = f->cont_hebrew = NULL;
  f->sync_english = f->sync_hebrew = NULL;
  f->current_english = parent->current_english;
  f->current_hebrew = parent->current_hebrew;
  f->dag = parent->dag;
  f->future_id = parent->future_id;

  f->current_list = parent->current_list;
  f->sync_list    = NULL;
}

extern "C" void do_enter_helper_begin(__cilkrts_stack_frame* sf, void* this_fn, void* rip)
{ 
  DBG_TRACE(DEBUG_CALLBACK, "%d: do_enter_helper_begin, sf: %p, parent: %p.\n", 
            self, sf, sf->call_parent);
}

// XXX: the doc says the sf is frame pointer to parent frame, but actually
// it's the pointer to the helper shadow frame. 
extern "C" void do_detach_begin(__cilkrts_stack_frame* sf)
{
  //return;
  __cilkrts_worker* w = __cilkrts_get_tls_worker();
  //assert(w->self == 0);
  // can't be empty now that we init strand in do_enter_end
  //om_assert(!frames[w->self].empty());

  AtomicStack_t<FrameData_t> *stack = pools[self].active_deque_;  
  //frames[w->self].push_helper();
  stack->push_helper();

  //DBG_TRACE(DEBUG_CALLBACK, 
  //          "%d: do_detach_begin, sf: %p, parent: %p, stack size: %d.\n", 
  //          self, sf, sf->call_parent, frames[self].size());

  FrameData_t* parent = stack->ancestor(1);
  FrameData_t* f = stack->head();
  assert(parent + 1 == f);

  assert(!(parent->flags & FRAME_HELPER_MASK));
  assert(f->flags & FRAME_HELPER_MASK);
  //  f->flags = FRAME_HELPER_MASK;

  if (!parent->sync_english) { // first of spawn group
    om_assert(!parent->sync_hebrew);
    parent->sync_english = g_english->insert(w, parent->current_english);
    parent->sync_hebrew = g_hebrew->insert(w, parent->current_hebrew);

    //pthread_spin_init(&parent->sync_lock, PTHREAD_PROCESS_PRIVATE);
    parent->sync_list = new List();
    parent->sync_list->init_lock();
    assert(parent->sync_list->Empty());
    parent->ori_list = parent->current_list;

    DBG_TRACE(DEBUG_CALLBACK, 
              "%d:  do_detach_begin, first of spawn group, "
              "setting parent sync_eng %p and sync heb %p.\n",
              self, parent->sync_english, parent->sync_hebrew);
  } else {
    DBG_TRACE(DEBUG_CALLBACK, 
              "%d: do_detach_begin, NOT first spawn, "
              "parent sync eng %p and sync heb: %p.\n", 
              self, parent->sync_english);
  }

  f->current_english = g_english->insert(w, parent->current_english);
  parent->cont_english = g_english->insert(w, f->current_english);

  parent->cont_hebrew = g_hebrew->insert(w, parent->current_hebrew);
  f->current_hebrew = g_hebrew->insert(w, parent->cont_hebrew);
  
  // DBG_TRACE(DEBUG_CALLBACK, 
  //           "do_detach_begin, f curr eng: %p and parent cont eng: %p.\n", 
  //           f->current_english, parent->cont_english);
  // DBG_TRACE(DEBUG_CALLBACK, 
  //           "do_detach_begin, f curr heb: %p and parent cont heb: %p.\n", 
  //           f->current_hebrew, parent->cont_hebrew);

  f->cont_english = NULL;
  f->cont_hebrew = NULL;
  f->sync_english = NULL; 
  f->sync_hebrew = NULL;
  
  
  parent->current_english = NULL; 
  parent->current_hebrew = NULL;

  parent->steal_cont_english = NULL;
  parent->steal_cont_hebrew  = NULL;
  parent->steal_sync_english = NULL;
  parent->steal_sync_hebrew  = NULL;
  parent->steal_current_list = NULL;

  f->current_list = parent->current_list;
  f->dag = parent->dag;
  f->future_id = parent->future_id;
  f->sync_list = NULL;
  current.english = f->current_english;
  current.hebrew  = f->current_hebrew;
  current.list    = f->current_list;
  //current.dag     = f->dag;
  current.future_id = f->future_id;
}

/* return 1 if we are entering top-level user frame and 0 otherwise */
extern "C" int do_enter_end (__cilkrts_stack_frame* sf, void* rsp)
{
  //__cilkrts_worker* w = sf->worker;
  __cilkrts_worker* w = __cilkrts_get_tls_worker();
  if (__cilkrts_get_batch_id(w) != -1) return 0;

  if (pools[w->self].active_deque_->empty()) {
    self = w->self;
    g_first_frame = 0;
    init_strand(w, NULL); // enter_counter already set to 1 in init_strand

    //DBG_TRACE(DEBUG_CALLBACK,
    //          "%d: do_enter_end, sf %p, parent %p, size %d.\n",
    //          self, sf, sf->call_parent, frames[self].size());
    return 1;
  } else {
    //DBG_TRACE(DEBUG_CALLBACK,
    //          "%d: do_enter_end, sf %p, parent %p, size %d.\n",
    //          self, sf, sf->call_parent, frames[self].size());
    return 0;
  }
}

extern "C" void do_sync_begin (__cilkrts_stack_frame* sf)
{
  //return;
  //DBG_TRACE(DEBUG_CALLBACK, "%d: do_sync_begin, sf %p, size %d.\n", 
  //          self, sf, frames[self].size());

  om_assert(self != -1);
  om_assert(!pools[self].active_deque_->empty());

  //if (__cilkrts_get_batch_id(sf->worker) != -1) return;

  FrameData_t* f = pools[self].active_deque_->head();

  // XXX: This should never happen
  // if (f->flags & FRAME_HELPER_MASK) { // this is a stolen frame, and this worker will be returning to the runtime shortly
  //  assert(0);
  //printf("do_sync_begin error.\n");
  //  return; /// @todo is this correct? this only seems to happen on the initial frame, when it is stolen
  // }

  assert(!(f->flags & FRAME_HELPER_MASK));
  om_assert(f->current_english); 
  om_assert(f->current_hebrew);
  om_assert(!f->cont_english); 
  om_assert(!f->cont_hebrew);

  if (f->sync_english) { // spawned
    om_assert(f->sync_hebrew);
    //    DBG_TRACE(DEBUG_CALLBACK, "function spawned (in cilk_sync_begin) (worker %d)\n", self);

    f->current_english = f->sync_english;
    f->current_hebrew = f->sync_hebrew;
    // DBG_TRACE(DEBUG_CALLBACK, 
    //           "do_sync_begin, setting eng to %p.\n", f->current_english);
    // DBG_TRACE(DEBUG_CALLBACK, 
    //           "do_sync_begin, setting heb to %p.\n", f->current_hebrew);
    f->sync_english = NULL; 
    f->sync_hebrew = NULL;

    if (f->current_list != f->ori_list) {
      //pthread_spin_lock(&f->sync_lock);
      f->sync_list->lock();
      //std::cout << "lock: " << std::dec << self << " " << std::hex << (void*)&f->sync_lock << std::endl;
      //std::cout << "merge: 605 " << std::dec << self << " " << std::hex << (void*)f->sync_list << std::endl;
      f->sync_list->Merge(*f->current_list);

      //parent->sync_list->PrintList();
      //pthread_spin_unlock(&f->sync_lock);
      f->sync_list->unlock();
    }

  } else { // function didn't spawn, or this is a function-ending sync
    om_assert(!f->sync_english); 
    om_assert(!f->sync_hebrew);
  }
}

extern "C" void do_sync_end()
{
  //return;
  //DBG_TRACE(DEBUG_CALLBACK, "%d: do_sync_end, size %d.\n", 
  //          self, frames[self].size());
  FrameData_t* f = pools[self].active_deque_->head();
  //if (f->sync_list != NULL) f->current_list = f->sync_list;
  //f->sync_list = NULL;
  //pthread_spin_destroy(&f->sync_lock);
  //ExternalPrint();
  //std::cout << (void*)f->current_list << std::endl;
  if (f->sync_list != NULL) {
    f->sync_list->destroy_lock();
    if (!f->sync_list->Empty()) {
      f->current_list = f->sync_list;//todo: is it safe to release current list here?
      
      //if (f->current_english->head == NULL) {
      //  ListPointer *lp = new ListPointer;
      //  lp->p = f->current_list;
      //  lp->next = NULL;
      //  f->current_english->head = lp;
      //} else {
      //  ListPointer *lp = new ListPointer;
      //  lp->p = f->current_list;
      //  lp->next = f->current_english->head;
      //  f->current_english->head = lp;
      //}
    
    } else {
      delete f->sync_list;
    }

    f->sync_list = NULL;
    //pthread_spin_destroy(&f->sync_lock);
  }

  current.english = f->current_english;
  current.hebrew  = f->current_hebrew;
  current.list    = f->current_list;
  //current.dag     = f->dag;
  current.future_id = f->future_id;
  //std::cout << (void*)f->current_list << std::endl;
  //ExternalPrint();
}

// Noticed that the frame was stolen.
extern "C" void do_leave_stolen(__cilkrts_stack_frame* sf)
{
  //return;
  //DBG_TRACE(DEBUG_CALLBACK, 
  //          "%d: do_leave_stolen, spawning sf %p, size before pop: %d.\n",
  //          self, sf, frames[self].size());
  if (pools[self].active_deque_->size() <= 1) return;
  if (called_future) return;

  FrameData_t* child = pools[self].active_deque_->head();
  FrameData_t* parent = pools[self].active_deque_->ancestor(1);

  // If there is a helper frame on top, this is a real steal. If not,
  // then this is a full frame returning.
  if (child->flags & FRAME_HELPER_MASK) { // Real steal
    om_assert(!(parent->flags & FRAME_HELPER_MASK));
    om_assert(!parent->current_english); om_assert(!parent->current_hebrew);
    om_assert(parent->cont_english); om_assert(parent->cont_hebrew);
    om_assert(parent->sync_english); om_assert(parent->sync_hebrew);

    /** I would expect this would work: */
    // parent->current_english = parent->cont_english;
    // parent->current_hebrew = parent->cont_hebrew;
    // parent->cont_english = parent->cont_hebrew = NULL;

    /** Instead, I seem to have to do the following. Why? */
    pools[self].active_deque_->lock();
    parent->steal_cont_english = parent->cont_english;
    parent->steal_cont_hebrew  = parent->cont_hebrew;
    parent->steal_sync_english = parent->sync_english;
    parent->steal_sync_hebrew  = parent->sync_hebrew;
    parent->steal_current_list = parent->current_list;

    parent->current_english = parent->sync_english;
    parent->current_hebrew = parent->sync_hebrew;
    parent->cont_english = parent->cont_hebrew = NULL;
    parent->sync_english = parent->sync_hebrew = NULL;

    //if (parent->current_list != child->current_list) {
    //  pthread_spin_lock(&parent->sync_lock);
    //  parent->sync_list->Merge(*child->current_list);
    //  pthread_spin_unlock(&parent->sync_lock);
    //}
    //parent->current_list = parent->sync_list;
    pools[self].active_deque_->unlock();
    
    if (parent->steal_current_list != child->current_list) {
      //pthread_spin_lock(&parent->sync_lock);
      parent->sync_list->lock();
      //std::cout << "lock: " << std::dec << self << " " << std::hex << (void*)&parent->sync_lock << std::endl;
      //std::cout << "merge: 693 " << std::dec << self << " " << std::hex << (void*)parent->sync_list << std::endl;
      parent->sync_list->Merge(*child->current_list);
      //pthread_spin_unlock(&parent->sync_lock);
      parent->sync_list->unlock();
    }
    
  } else { // full frame returning
    om_assert(sf == NULL);
    om_assert(!parent->cont_english);
    om_assert(!parent->cont_hebrew);
    parent->current_english = child->current_english;
    parent->current_hebrew = child->current_hebrew;
    parent->current_list   = child->current_list;
  }
  pools[self].active_deque_->pop();
}

/* return 1 if we are leaving last frame and 0 otherwise */
extern "C" int do_leave_begin (__cilkrts_stack_frame *sf)
{
  //DBG_TRACE(DEBUG_CALLBACK, "%d: do_leave_begin, sf %p, size: %d.\n", 
  //          self, sf, frames[self].size());
  called_future = false;
  self = __cilkrts_get_tls_worker()->self;
  return pools[self].active_deque_->empty();
}

extern "C" int do_leave_end()
{
  //DBG_TRACE(DEBUG_CALLBACK, "%d: do_leave_end, size before pop %d.\n", 
  //          self, frames[self].size());
  assert(t_worker);
  if (__cilkrts_get_batch_id(t_worker) != -1) return 0;

  om_assert(self != -1);
  om_assert(!pools[self].active_deque_->empty());

  FrameData_t* child = pools[self].active_deque_->head();
  if (pools[self].active_deque_->size() > 1) {
    //    DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): popping and changing nodes.\n", self);
    FrameData_t* parent = pools[self].active_deque_->ancestor(1);
    if (!(child->flags & FRAME_HELPER_MASK)) { // parent called current
      //      DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): returning from call.\n", self);

      om_assert(!parent->cont_english);
      om_assert(!parent->cont_hebrew);
      parent->current_english = child->current_english;
      parent->current_hebrew = child->current_hebrew;
      parent->current_list   = child->current_list;
    } else { // parent spawned current
      //      DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): parent spawned child.\n", self);
      om_assert(!parent->current_english); om_assert(!parent->current_hebrew);
      om_assert(parent->cont_english); om_assert(parent->cont_hebrew);
      om_assert(parent->sync_english); om_assert(parent->sync_hebrew);
      parent->current_english = parent->cont_english;
      parent->current_hebrew = parent->cont_hebrew;
      parent->cont_english = parent->cont_hebrew = NULL;
      //do something, why here?
      if (parent->current_list != child->current_list) {
        //pthread_spin_lock(&parent->sync_lock);
        parent->sync_list->lock();
        //std::cout << "lock: " << std::dec << self << " " << std::hex << (void*)&parent->sync_lock << std::endl;
        //std::cout << "merge: 765 " << std::dec << self << " " << std::hex << (void*)parent->sync_list << std::endl;
        parent->sync_list->Merge(*child->current_list);
        //parent->sync_list->PrintList();
        //pthread_spin_unlock(&parent->sync_lock);
        parent->sync_list->unlock();
      }
    }
    pools[self].active_deque_->pop();
  }
  // we are leaving the last frame if the shadow stack is empty
  //pools[self].active_deque_->pop();
  FrameData_t *current_frame = pools[self].active_deque_->head();
  current.english = current_frame->current_english;
  current.hebrew  = current_frame->current_hebrew;
  current.list    = current_frame->current_list;
  //current.dag     = current_frame->dag;
  current.future_id = current_frame->future_id;
  return pools[self].active_deque_->empty();
}


//todo: list pointer
// called by record_memory_read/write, with the access broken down 
// into 8-byte aligned memory accesses
/*void
record_mem_helper(bool is_read, uint64_t inst_addr, uint64_t addr,
                  uint32_t mem_size)
{
  om_assert(self != -1);
  FrameData_t *f = pools[self].active_deque_->head();
  MemAccessList_t *val = shadow_mem.find( ADDR_TO_KEY(addr) );
  //  MemAccess_t *acc = NULL;
  MemAccessList_t *mem_list = NULL;

  if( val == NULL ) {
    // not in shadow memory; create a new MemAccessList_t and insert
    mem_list = new MemAccessList_t(addr, is_read, f->current_english, 
                                   f->current_hebrew, inst_addr, mem_size);
    val = shadow_mem.insert(ADDR_TO_KEY(addr), mem_list);
    // insert failed; someone got to the slot first;
    // delete new mem_list and fall through to check race with it 
    if( val != mem_list ) { delete mem_list; }
    else { return; } // else we are done
  }
  // check for race and possibly update the existing MemAccessList_t 
  om_assert(val != NULL);
  val->check_races_and_update(is_read, inst_addr, addr, mem_size, 
                              f->current_english, f->current_hebrew);
}*/

// XXX: We can only read 1,2,4,8,16 bytes; optimize later
/* Deprecated
   extern "C" void do_read(uint64_t inst_addr, uint64_t addr, size_t mem_size)
   {
   DBG_TRACE(DEBUG_MEMORY, "record read of %lu bytes at addr %p and rip %p.\n", 
   mem_size, addr, inst_addr);

   // handle the prefix
   uint64_t next_addr = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(addr); 
   size_t prefix_size = next_addr - addr;
   om_assert(prefix_size >= 0 && prefix_size < MAX_GRAIN_SIZE);

   if(prefix_size >= mem_size) { // access falls within a max grain sized block
   record_mem_helper(true, inst_addr, addr, mem_size);
   } else { 
   om_assert( prefix_size <= mem_size );
   if(prefix_size) { // do the prefix first
   record_mem_helper(true, inst_addr, addr, prefix_size);
   mem_size -= prefix_size;
   }
   addr = next_addr;
   // then do the rest of the max-grain size aligned blocks
   uint32_t i;
   for(i = 0; (i+MAX_GRAIN_SIZE) < mem_size; i += MAX_GRAIN_SIZE) {
   record_mem_helper(true, inst_addr, addr + i, MAX_GRAIN_SIZE);
   }
   // trailing bytes
   record_mem_helper(true, inst_addr, addr+i, mem_size-i);
   }
   }

   // XXX: We can only read 1,2,4,8,16 bytes; optimize later
   extern "C" void do_write(uint64_t inst_addr, uint64_t addr, size_t mem_size)
   {
   DBG_TRACE(DEBUG_MEMORY, "record write of %lu bytes at addr %p and rip %p.\n", 
   mem_size, addr, inst_addr);

  // handle the prefix
  uint64_t next_addr = ALIGN_BY_NEXT_MAX_GRAIN_SIZE(addr); 
  size_t prefix_size = next_addr - addr;
  om_assert(prefix_size >= 0 && prefix_size < MAX_GRAIN_SIZE);

  if(prefix_size >= mem_size) { // access falls within a max grain sized block
    record_mem_helper(false, inst_addr, addr, mem_size);
  } else {
    om_assert( prefix_size <= mem_size );
    if(prefix_size) { // do the prefix first
      record_mem_helper(false, inst_addr, addr, prefix_size);
      mem_size -= prefix_size;
    }
    addr = next_addr;
    // then do the rest of the max-grain size aligned blocks
    uint32_t i=0;
    for(i=0; (i+MAX_GRAIN_SIZE) < mem_size; i += MAX_GRAIN_SIZE) {
      record_mem_helper(false, inst_addr, addr + i, MAX_GRAIN_SIZE);
    }
    // trailing bytes
    record_mem_helper(false, inst_addr, addr+i, mem_size-i);
  }
}
*/

// clear the memory block at [start-end) (end is exclusive).
extern "C" void
clear_shadow_memory(size_t start, size_t end) {
  return;
  DBG_TRACE(DEBUG_MEMORY, "Clear shadow memory %p--%p (%u).\n", 
            start, end, end-start);
  om_assert(ALIGN_BY_NEXT_MAX_GRAIN_SIZE(end) == end); 
  om_assert(start < end);
  //  om_assert(end-start < 4096);

  while(start != end) {
    shadow_mem.erase( ADDR_TO_KEY(start) );
    start += MAX_GRAIN_SIZE;
  }
}

size_t number_of_future = 0;
extern "C" void CilkFutureCreate() {
    disable_checking();
    print_debug_info("cilk future create");
    number_of_future++;
    
    //if (self == -1 || pools[self].active_deque_->empty()
    //    || pools[self].active_deque_->head()->flags & FRAME_HELPER_MASK) {
    //    do_enter_begin();
    //    do_enter_end(NULL, NULL);
    //}         
    FrameData_t* parent = pools[self].active_deque_->head();
    Node parent_node(parent->current_english, parent->current_hebrew, parent->current_english, 
                     parent->current_hebrew, parent->dag);

    do_detach_begin(NULL);
    FrameData_t* f = pools[self].active_deque_->head();
    f->dag = parent_node.cnode_e;
    f->future_id = future_id.fetch_add(1);
    //std::cout << "parent future id: " << parent->future_id << " future_id: " << f->future_id << std::endl;

    //List *list_create = new List(&parent_node);
    //f->current_list = new List(*f->current_list, *list_create);
    //delete list_create;
    ///List *old_list = f->current_list;
    ///f->current_list = new List(parent_node);
    //std::cout << "merge: 908 " << std::dec << self << " " << std::hex << (void*)f->current_list << std::endl;
    ///f->current_list->Merge(*old_list);
    if (!f->current_list->Empty()) {
        f->current_list = new List(*f->current_list, parent_node, parent->future_id);
    } else {
        f->current_list = new List(parent_node, parent->future_id);
    }
    
    parent->current_list = f->current_list;

    //if (f->current_english->head == NULL) {
    //    ListPointer *lp = new ListPointer;
    //    lp->p = f->current_list;
    //    lp->next = NULL;
    //    f->current_english->head = lp;
    //} else {
    //    ListPointer *lp = new ListPointer;
    //    lp->p = f->current_list;
    //    lp->next = f->current_english->head;
    //    f->current_english->head = lp;
    //}
 
   current.list = f->current_list;
   current.future_id = f->future_id;
   enable_checking();
}

extern "C" void* CilkFutureFinish() {
    //can I deallocate the active_deque here?
    disable_checking();
    print_debug_info("cilk future finish");
    

    assert(t_worker);
    if (__cilkrts_get_batch_id(t_worker) != -1) return 0;

    called_future = true;
    
    FrameData_t* child = pools[self].active_deque_->head();
    FrameData_t* parent = pools[self].active_deque_->ancestor(1);
    
    pools[self].active_deque_->lock();
    //if (parent->current_english == NULL) {
    //    parent->current_english = parent->cont_english;
    //    parent->current_hebrew = parent->cont_hebrew;
    //    parent->cont_english = parent->cont_hebrew = NULL;
    //}
    parent->steal_cont_english = parent->cont_english;
    parent->steal_cont_hebrew  = parent->cont_hebrew;
    parent->steal_sync_english = parent->sync_english;
    parent->steal_sync_hebrew  = parent->sync_hebrew;
    parent->steal_current_list = parent->current_list;

    parent->current_english = parent->cont_english;
    parent->current_hebrew  = parent->cont_hebrew;
    parent->cont_english    = parent->cont_hebrew = NULL;

    pools[self].active_deque_->unlock();

    Node node;
    node.cnode_e = node.desc_e = child->current_english;
    node.cnode_h = node.desc_h = child->current_hebrew;
    node.dag     = child->dag;
    //List *list_create = new List(node);
    //List *ret = new List(*child->current_list, *list_create);
    List *ret = new List(*child->current_list, node, child->future_id);
    
    //if (child->current_english->head == NULL) {
    //    ListPointer *lp = new ListPointer;
    //    lp->p = ret;
    //    lp->next = NULL;
    //    child->current_english->head = lp;
    //} else {
    //    ListPointer *lp = new ListPointer;
    //    lp->p = ret;
    //    lp->next = child->current_english->head;
    //    child->current_english->head = lp;
    //}
 
    //delete list_create;

    pools[self].active_deque_->pop();
    FrameData_t *current_frame = pools[self].active_deque_->head();
    current.english = current_frame->current_english;
    current.hebrew  = current_frame->current_hebrew;
    current.list    = current_frame->current_list;
    //current.dag     = current_frame->dag;
    current.future_id = current_frame->future_id;
    enable_checking();
    return ret;
}

//todo: create a new OM node 
extern "C" void* CilkFutureGet(bool suspend, void* list) {
    disable_checking();
    void *ret = NULL;
    if (suspend) {
        print_debug_info("cilk future get suspend");
        //return pools[self].SuspendSelf();
        ret = CilkSuspend();
    } else {
        print_debug_info("cilk future get cont");
        List *list_get = (List *)list;
        FrameData_t* f = pools[self].active_deque_->head();
        f->current_list = new List(*f->current_list, *list_get);
    
        //if (f->current_english->head == NULL) {
        //    ListPointer *lp = new ListPointer;
        //    lp->p = f->current_list;
        //    lp->next = NULL;
        //    f->current_english->head = lp;
        //} else {
        //    ListPointer *lp = new ListPointer;
        //    lp->p = f->current_list;
        //    lp->next = f->current_english->head;
        //    f->current_english->head = lp;
        //}
        current.list = f->current_list;
    }
    enable_checking();
    return ret;
}

extern "C" void* CilkSteal(StealType type, void *deque, void *list, __cilkrts_worker* w, __cilkrts_worker* victim) {
    if (t_worker && __cilkrts_get_batch_id(t_worker) != -1) return NULL;
    
    disable_checking();
    if (p_memory.mem == NULL) p_memory = g_memory.grab_mem();
   
    if (deque == NULL) deque = g_deque;
    AtomicStack_t<FrameData_t> *ret = NULL;
    self = w->self;
    switch (type) {
    case kNormal:
        print_debug_info("cilk steal normal");
        //ret = do_steal_success(w, victim, NULL, pools[victim->self].active_deque_); 
        ret = do_steal_success(w, victim, NULL, (AtomicStack_t<FrameData_t>*)deque);
        break;
    case kResumable://the same as stealing from a suspended deque
        print_debug_info("cilk steal resumable");
        //ret = do_steal_success(w, victim, NULL, ((ShadowDeque<AtomicStack_t<FrameData_t>>*)deque)->deque);
        ret = do_steal_success(w, victim, NULL, (AtomicStack_t<FrameData_t>*)deque);
        break;
    case kSuspended:
        print_debug_info("cilk steal suspended");
        //ret = do_steal_success(w, victim, NULL, ((ShadowDeque<AtomicStack_t<FrameData_t>>*)deque)->deque);
        ret = do_steal_success(w, victim, NULL, (AtomicStack_t<FrameData_t>*)deque);
        break;
    case kResume: {
        print_debug_info("mug a deque");
        //mug the whole victim's deque, deallocate self's active_deque_
        //actually, is it safe to deallocate the active deque here?
        t_worker = w;
        pools[w->self].FreeCurrentDeque();
        //pools[w->self].active_deque_ = pools[w->self].RemoveFromPool((ShadowDeque<AtomicStack_t<FrameData_t>>*)deque);
        pools[w->self].active_deque_ = (AtomicStack_t<FrameData_t>*)deque;
        List *list_get = (List *)list;
        FrameData_t* f = pools[self].active_deque_->head();
        f->current_list = new List(*f->current_list, *list_get);
        
        //if (f->current_english->head == NULL) {
        //    ListPointer *lp = new ListPointer;
        //    lp->p = f->current_list;
        //    lp->next = NULL;
        //    f->current_english->head = lp;
        //} else {
        //    ListPointer *lp = new ListPointer;
        //    lp->p = f->current_list;
        //    lp->next = f->current_english->head;
        //    f->current_english->head = lp;
        //}
break;
    }
    default:
        break;
    }
    FrameData_t* current_frame = pools[self].active_deque_->head();
    current.english = current_frame->current_english;
    current.hebrew  = current_frame->current_hebrew;
    current.list    = current_frame->current_list;
    //current.dag     = current_frame->dag;
    current.future_id = current_frame->future_id;

    enable_checking();
    enable_checking();
    return (void*)ret;
}

extern "C" void* CilkSuspend() {
    disable_checking();
    print_debug_info("cilk suspend");
    void *ret =  (void*)pools[self].SuspendSelf();
    enable_checking();
    return ret;
    //return NULL;
}

extern "C" bool FuturePrecedes(om_node *e1, om_node *h1, om_node *e2, om_node *h2, List *list) {
    //if (om_precedes(e1, e2) && om_precedes(h1, h2)) return true;
    //return list->Search(e1, h1);
    return false;
}




