#include <iostream>
#include <cstdlib> // for getenv

#include <internal/abi.h> // for __cilkrts_stack_frame
#include <cilk/batcher.h>
#include "rd.h"
#include "print_addr.h"
#include "shadow_mem.h"
#include "mem_access.h"

#include "stack.h"
#include "stat_util.h"

#include "omrd.cpp" // Hack @todo fix linking errors

int g_tool_init = 0;

om_node* base_english = NULL;
om_node* base_hebrew = NULL;

// XXX Need to synchronize access to it when update
static ShadowMem<MemAccessList_t> shadow_mem;

void init_strand(__cilkrts_worker* w, __cilkrts_stack_frame *sf)
{
  self = w->self;
  t_worker = w;

  FrameData_t* f;
  if (sf) {
    assert(!(init->flags & FRAME_HELPER_MASK));
    assert(!init->current_english);
    assert(!init->current_hebrew);

    f->current_english = init->cont_english;
    f->current_hebrew = init->cont_hebrew;
    f->sync_english = init->sync_english;
    f->sync_hebrew = init->sync_hebrew;
    f->cont_english = NULL;
    f->cont_hebrew = NULL;
    f->flags = init->flags;
  }  else {
    assert(frames[w->self].empty());
    frames[w->self].push();
    f = frames[w->self].head();
    f->flags = 0;

    f->current_english = g_english->get_base();
    f->current_hebrew = g_hebrew->get_base();
    f->cont_english = NULL;
    f->cont_hebrew = NULL;
    f->sync_english = NULL;
    f->sync_hebrew = NULL;
  }
}


extern "C" void do_tool_init(void) 
{
  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_init called.\n");
  int p = __cilkrts_get_nworkers();
#if STATS > 1
  g_nproc = p;
  g_timing_events = (double*)malloc(sizeof(double) * NUM_INTERVAL_TYPES * p);
  g_timing_event_starts = (struct timespec*)malloc(sizeof(struct timespec) * NUM_INTERVAL_TYPES * p);
  g_timing_event_ends = (struct timespec*)malloc(sizeof(struct timespec) * NUM_INTERVAL_TYPES * p);
  EMPTY_TIME_POINT.tv_sec = 0; EMPTY_TIME_POINT.tv_nsec = 0;
  for (int i = 0; i < (NUM_INTERVAL_TYPES*p); ++i) {
    g_timing_events[i] = INTERVAL_CAST(ZERO_DURATION);
    g_timing_event_starts[i].tv_sec = EMPTY_TIME_POINT.tv_sec;
    g_timing_event_starts[i].tv_nsec = EMPTY_TIME_POINT.tv_nsec;
    g_timing_event_ends[i].tv_sec = EMPTY_TIME_POINT.tv_sec;
    g_timing_event_ends[i].tv_nsec = EMPTY_TIME_POINT.tv_nsec;
  }
#endif
  frames = new AtomicStack_t<FrameData_t>[p];
  //  g_worker_mutexes = new pthread_spinlock_t[p];
  g_worker_mutexes = (local_mut*)memalign(64, sizeof(local_mut)*p);
  for (int i = 0; i < p; ++i) {
    pthread_spin_init(&g_worker_mutexes[i].mut, PTHREAD_PROCESS_PRIVATE);
  }
  pthread_spin_init(&g_relabel_mutex, PTHREAD_PROCESS_PRIVATE);
  g_english = new omrd_t();
  g_hebrew = new omrd_t();

  char *s_thresh = getenv("HEAVY_THRESHOLD");
  if (s_thresh) {
    label_t thresh = (label_t)atol(getenv("HEAVY_THRESHOLD"));
    g_english->set_heavy_threshold(thresh);
    g_hebrew->set_heavy_threshold(thresh);
  }

  // XXX: Can I assume this is called before everything?
  read_proc_maps();
  g_tool_init = 1;
}


extern "C" void do_tool_print(void)
{
  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_print called.\n");

#if STATS > 0
  int p = __cilkrts_get_nworkers();
#endif
#if STATS > 1
  // for (int i = 0; i < NUM_INTERVAL_TYPES; ++i) {
  //   assert(g_timing_event_starts[i].tv_sec == EMPTY_TIME_POINT.tv_sec);
  //   assert(g_timing_event_starts[i].tv_nsec == EMPTY_TIME_POINT.tv_nsec);
  //   std::cout << "\t\t" << g_timing_events[i];
  // }
  // std::cout << std::endl;
  std::cout << "Fast Path time: ";
  for (int i = 0; i < p; ++i) {
    std::cout << g_timing_events[(i*p) + FAST_PATH] << std::endl;
  }
  std::cout << "Slow Path time: ";
  for (int i = 0; i < p; ++i) {
    std::cout << g_timing_events[(i*p) + SLOW_PATH] << std::endl;
  }
#endif
#if STATS > 0  
  std::cout << "Num relabels: " << g_num_relabels << std::endl;
  std::cout << "Num inserts: " << g_num_inserts << std::endl;
  std::cout << "Avg size per relabel: " << (double)g_relabel_size / (double)g_num_relabels << std::endl;
  std::cout << "English OM memory used: " << g_english->memsize() << std::endl;
  std::cout << "Hebrew OM memory used: " << g_hebrew->memsize() << std::endl;
  size_t sssize = 0;
  for (int i = 0; i < p; ++i) sssize += frames[i].memsize();
  std::cout << "Shadow stack(s) total size: " << sssize << std::endl;
#endif
}

extern "C" void do_tool_destroy(void) 
{
  DBG_TRACE(DEBUG_CALLBACK, "cilk_tool_destroy called.\n");

  do_tool_print();
  delete g_hebrew;
  delete g_english;
  delete[] frames;

  for (int i = 0; i < __cilkrts_get_nworkers(); ++i) {
    pthread_spin_destroy(&g_worker_mutexes[i].mut);
  }
  pthread_spin_destroy(&g_relabel_mutex);

  //  delete[] g_worker_mutexes;
  free(g_worker_mutexes);
#if STATS > 1
  free(g_timing_events);
  free(g_timing_event_starts);
  free(g_timing_event_ends);
#endif
  //  delete_proc_maps(); /// @todo shakespeare has problems compiling print_addr.cpp
}

extern "C" om_node* get_current_english()
{
  __cilkrts_worker* w = __cilkrts_get_tls_worker();
  if (!frames) return NULL;
  return frames[w->self].head()->current_english;
}

extern "C" om_node* get_current_hebrew()
{
  __cilkrts_worker* w = __cilkrts_get_tls_worker();
  if (!frames) return NULL;
  return frames[w->self].head()->current_hebrew;
}


extern "C" void do_steal_success(__cilkrts_worker* w, __cilkrts_worker* victim,
                                 __cilkrts_stack_frame* sf)
{
  DBG_TRACE(DEBUG_CALLBACK, "Worker %i stole from %i.\n", w->self, victim->self);

  frames[w->self].reset();
  FrameData_t* loot = frames[victim->self].steal_top(frames[w->self]);
  printf("Worker %i stole %s from %i.\n", w->self, loot->func_name, victim->self);
  om_assert(!(loot->flags & FRAME_HELPER_MASK));
  init_strand(w, loot);
}

extern "C" void cilk_return_to_first_frame(__cilkrts_worker* w, __cilkrts_worker* team,
					   __cilkrts_stack_frame* sf)
{
  DBG_TRACE(DEBUG_CALLBACK, "Transfering shadow stack from %d to original thread %d.\n", self, team->self);
  if (self != team->self) frames[self].transfer(frames[team->self]);
}

extern "C" void do_enter_begin()
{
  if (self == -1) return;
  frames[self].push();
  FrameData_t* parent = frames[self].ancestor(1);
  FrameData_t* f = frames[self].head();
  f->flags = 0;
  f->cont_english = f->cont_hebrew = NULL;
  f->sync_english = f->sync_hebrew = NULL;
  f->current_english = parent->current_english;
  f->current_hebrew = parent->current_hebrew;
}

  //  DBG_TRACE(DEBUG_CALLBACK, "do_detach_begin, parent sf %p.\n", sf->call_parent);
extern "C" void do_enter_helper_begin(__cilkrts_stack_frame* sf, void* this_fn, void* rip) { }

extern "C" void do_detach_begin(__cilkrts_stack_frame* parent_sf)
{
  DBG_TRACE(DEBUG_CALLBACK, "do_detach_end, worker %d.\n", self);
  __cilkrts_worker* w = __cilkrts_get_tls_worker();

  // can't be empty now that we init strand in do_enter_end
  om_assert(!frames[w->self].empty());

  frames[w->self].push_helper();
  FrameData_t* parent = frames[w->self].ancestor(1);
  FrameData_t* f = frames[w->self].head();
  assert(parent + 1 == f);

  assert(!(parent->flags & FRAME_HELPER_MASK));
  assert(f->flags & FRAME_HELPER_MASK);
  //  f->flags = FRAME_HELPER_MASK;

  if (!parent->sync_english) { // first of spawn group
    om_assert(!parent->sync_hebrew);
    DBG_TRACE(DEBUG_BACKTRACE, "first of spawn group\n");

    parent->sync_english = g_english->insert(w, parent->current_english);
    parent->sync_hebrew = g_hebrew->insert(w, parent->current_hebrew);

    // pthread_spin_lock(&g_worker_mutexes[self]);
    // assert(om_precedes(parent->current_english, parent->sync_english));
    // assert(om_precedes(parent->current_hebrew, parent->sync_hebrew));
    // pthread_spin_unlock(&g_worker_mutexes[self]);

    DBG_TRACE(DEBUG_CALLBACK, 
        "do_detach_begin, setting parent sync_eng %p and sync heb %p.\n",
        parent->sync_english, parent->sync_hebrew);
  } else {
    DBG_TRACE(DEBUG_CALLBACK, 
              "do_detach_begin, NOT first spawn, parent sync eng %p.\n", 
              parent->sync_english);
    DBG_TRACE(DEBUG_BACKTRACE, "NOT first of spawn group\n");
  }

  f->current_english = g_english->insert(w, parent->current_english);
  parent->cont_english = g_english->insert(w, f->current_english);

  parent->cont_hebrew = g_hebrew->insert(w, parent->current_hebrew);
  f->current_hebrew = g_hebrew->insert(w, parent->cont_hebrew);
  
  DBG_TRACE(DEBUG_CALLBACK, 
            "do_detach_begin, f curr eng: %p and parent cont eng: %p.\n", 
            f->current_english, parent->cont_english);
  DBG_TRACE(DEBUG_CALLBACK, 
            "do_detach_begin, f curr heb: %p and parent cont heb: %p.\n", 
            f->current_hebrew, parent->cont_hebrew);

  f->cont_english = NULL;
  f->cont_hebrew = NULL;
  f->sync_english = NULL; 
  f->sync_hebrew = NULL;
  parent->current_english = NULL; 
  parent->current_hebrew = NULL;
}

/* return 1 if we are entering top-level user frame and 0 otherwise */
extern "C" int do_enter_end (__cilkrts_stack_frame* sf, void* rsp)
{
  DBG_TRACE(DEBUG_CALLBACK, "do_enter_end, sf %p and worker %d.\n", sf, self);

  __cilkrts_worker* w = sf->worker;
  if (__cilkrts_get_batch_id(w) != -1) return 0;

  if (frames[w->self].empty()) {
    self = w->self;
    t_worker = w;
    sf->current_english = (void*)g_english->get_base();
    sf->current_hebrew = (void*)g_hebrew->get_base();
    sf->cont_english = NULL;
    sf->cont_hebrew = NULL;
    sf->sync_english = NULL;
    sf->sync_hebrew = NULL;
    return 1;
  } else {
    //FrameData_t* f = frames[sf->worker->self].head();
    return 0;
  }
}

extern "C" void do_sync_begin (__cilkrts_stack_frame* sf)
{
  DBG_TRACE(DEBUG_CALLBACK, "do_sync_begin, sf %p and worker %d.\n", sf, self);

  om_assert(self != -1);
  om_assert(!frames[self].empty());

  if (__cilkrts_get_batch_id(sf->worker) != -1) return;

  FrameData_t* f = frames[self].head();
  DBG_TRACE(DEBUG_CALLBACK, "do_sync_begin, shadow frame %p and worker %d.\n", f, self);

  if (f->flags & FRAME_HELPER_MASK) { // this is a stolen frame, and this worker will be returning to the runtime shortly
    //printf("do_sync_begin error.\n");
    return; /// @todo is this correct? this only seems to happen on the initial frame, when it is stolen
  }

  //  assert(!(f->flags & FRAME_HELPER_MASK));
  om_assert(f->current_english); 
  om_assert(f->current_hebrew);
  om_assert(!f->cont_english); 
  om_assert(!f->cont_hebrew);

  if (f->sync_english) { // spawned
    om_assert(f->sync_hebrew);
    DBG_TRACE(DEBUG_CALLBACK, "function spawned (in cilk_sync_begin) (worker %d)\n", self);

    f->current_english = f->sync_english;
    f->current_hebrew = f->sync_hebrew;
    DBG_TRACE(DEBUG_CALLBACK, 
              "do_sync_begin, setting eng to %p.\n", f->current_english);
    DBG_TRACE(DEBUG_CALLBACK, 
              "do_sync_begin, setting heb to %p.\n", f->current_hebrew);
    f->sync_english = NULL; 
    f->sync_hebrew = NULL;
  } else { // function didn't spawn, or this is a function-ending sync
    om_assert(!f->sync_english); 
    om_assert(!f->sync_hebrew);
  }
}

// Noticed that the frame was stolen.
extern "C" void do_leave_stolen(__cilkrts_stack_frame* sf)
{
  //  DBG_TRACE(DEBUG_CALLBACK, "do_leave_stolen, sf %p and worker %d.\n", sf, self);
  printf("do_leave_stolen, sf %p and worker %d.\n", sf, self);
  //  if (sf == NULL) return; /// Why does this happen???

  // We *should* still have a helper frame on the shadow stack, since
  // cilk_leave_end will not be called
  FrameData_t* parent = frames[self].ancestor(1);
  if (!(frames[self].head()->flags & FRAME_HELPER_MASK))
    printf("Error\n");

  om_assert(frames[self].head()->flags & FRAME_HELPER_MASK);
  om_assert(!(parent->flags & FRAME_HELPER_MASK));
  om_assert(parent->current_english == NULL);
  om_assert(parent->current_hebrew == NULL);
  om_assert(parent->cont_english);
  om_assert(parent->cont_hebrew);
  om_assert(parent->sync_english);
  om_assert(parent->sync_hebrew);

  parent->current_english = parent->sync_english;
  parent->current_hebrew = parent->sync_hebrew;
  parent->cont_english = NULL;
  parent->cont_hebrew = NULL;
  parent->sync_english = NULL;
  parent->sync_hebrew = NULL;

  frames[self].pop();
}

/* return 1 if we are leaving last frame and 0 otherwise */
extern "C" int do_leave_begin (__cilkrts_stack_frame *sf)
{
  return frames[sf->worker->self].empty();
}


extern "C" int do_leave_end()
{
  //  DBG_TRACE(DEBUG_CALLBACK, "do_leave_end, sf %p and worker %d.\n", sf, self);
  DBG_TRACE(DEBUG_CALLBACK, "do_leave_end, worker %d.\n", self);
  assert(t_worker);
  if (__cilkrts_get_batch_id(t_worker) != -1) return 0;

  om_assert(self != -1);
  om_assert(!frames[self].empty());

  FrameData_t* child = frames[self].head();
  if (frames[self].size() > 1) {
    DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): popping and changing nodes.\n", self);
    FrameData_t* parent = frames[self].ancestor(1);
    if (!(child->flags & FRAME_HELPER_MASK)) { // parent called current
      DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): returning from call.\n", self);

      om_assert(!parent->cont_english);
      om_assert(!parent->cont_hebrew);
      parent->current_english = child->current_english;
      parent->current_hebrew = child->current_hebrew;
    } else { // parent spawned current
      DBG_TRACE(DEBUG_CALLBACK, "cilk_leave_end(%d): parent spawned child.\n", self);
      om_assert(!parent->current_english); om_assert(!parent->current_hebrew);
      om_assert(parent->cont_english); om_assert(parent->cont_hebrew);
      om_assert(parent->sync_english); om_assert(parent->sync_hebrew);
      parent->current_english = parent->cont_english;
      parent->current_hebrew = parent->cont_hebrew;
      parent->cont_english = parent->cont_hebrew = NULL;
    }
    frames[self].pop();
  }

  // we are leaving the last frame if the shadow stack is empty
  return frames[self].empty();
}

// called by record_memory_read/write, with the access broken down 
// into 8-byte aligned memory accesses
void
record_mem_helper(bool is_read, uint64_t inst_addr, uint64_t addr,
                  uint32_t mem_size)
{
  om_assert(self != -1);
  FrameData_t *f = frames[self].head();
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
}

// clear the memory block at [start-end) (end is exclusive).
extern "C" void
clear_shadow_memory(size_t start, size_t end) {

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
