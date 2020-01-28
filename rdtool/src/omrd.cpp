// A wrapper around my OM data structure, made specifically with the
// extra features necessary for race detection.
#include "omrd.h"
#include <internal/abi.h> // for __cilkrts_get_nworkers();
#include <cilk/cilk_api.h>
#include <cilk/batcher.h> // for cilK_batchify
#include <cstdio> // temporary
#include "om/om.c"
#include "stack.h"

#define HALF_BITS ((sizeof(label_t) * 8) / 2)
//#define HALF_BITS 8
#define DEFAULT_HEAVY_THRESHOLD HALF_BITS
static label_t g_heavy_threshold = DEFAULT_HEAVY_THRESHOLD;

static volatile int g_batch_in_progress = 0;
volatile size_t g_relabel_id = 0;
size_t g_num_relabel_lock_tries = 0;

//void batch_relabel(void* _ds, void* data, size_t size, void* results);

// Brian Kernighan's algorithm
size_t count_set_bits(label_t label)
{
  size_t count = 0;
  while (label) {
    label &= (label - 1);
    count++;
  }
  return count;
}

//int is_heavy(om_node* n) { return count_set_bits >= g_heavy_threshold; }
int is_heavy(om_node* n)
{
  label_t next_lab = (n->next) ? n->next->label : MAX_LABEL;
  return (next_lab - n->label) < (1L << (64 - g_heavy_threshold));
}

#define CAS(loc,old,nval) __sync_bool_compare_and_swap(loc,old,nval)
void batch_relabel(void* _ds, void* data, size_t size, void* results);

/// @todo break this into a FPSPController class (or something similar)
#include "rd.h" // for local_mut definition
local_mut* g_worker_mutexes;
//#include <../runtime/worker_mutex.h>
#include "worker_mutex.h"
mutex g_relabel_mutex;
#define LOCK_SUCCESS 1

//pthread_spinlock_t g_relabel_mutex;
//__cilkrts_worker *g_relabel_owner;
//#define LOCK_SUCCESS 0

int g_batch_owner_id = -1;
__thread int insert_failed = 0;
__thread int failed_at_relabel = -1;
//__thread int t_in_batch = 0;


extern "C" int cilk_tool_om_try_lock_all(__cilkrts_worker* w)
{
  // If we haven't failed an insert, we came here because someone had
  // our lock when we wanted to insert. Don't don't even try to
  // contend on the lock.
  if (!insert_failed) return 0;
  if ((g_relabel_id & 0x1) == 1) return 0;

  // fprintf(stderr, "Worker %d trying to start batch %zu\n", w->self, g_num_relabels);
  // assert(failed_at_relabel < ((int)g_num_relabels));
  // failed_at_relabel = g_num_relabels;

  //  __sync_fetch_and_add(&g_num_relabel_lock_tries, 1);
  // RDTOOL_INTERVAL_BEGIN(RELABEL_LOCK);
  //int result = pthread_spin_trylock(&g_relabel_mutex);
  int result = __cilkrts_mutex_trylock(w, &g_relabel_mutex);
  // RDTOOL_INTERVAL_END(RELABEL_LOCK);

  if (result != LOCK_SUCCESS) return 0;
  //  fprintf(stderr, "Relabel lock acquired by %d\n", g_relabel_mutex.owner->self);
  //  assert(t_in_batch == 1);

  cilk_set_next_batch_owner();
  int p = __cilkrts_get_nworkers();
  for (int i = 0; i < p; ++i) {
    pthread_spin_lock(&g_worker_mutexes[i].mut);
  }
  g_batch_in_progress = 1;
  om_assert(g_batch_owner_id == -1);
  g_batch_owner_id = w->self;
  //assert(self == __cilkrts_get_tls_worker()->self);
  return 1;
}

class omrd_t;

//omrd_t *g_english;
//omrd_t *g_hebrew;

//void relabel(omrd_t *_ds);

void join_batch(__cilkrts_worker *w, omrd_t *ds)
{
  DBG_TRACE(DEBUG_BACKTRACE, "Worker %i calling batchify.\n", w->self);
  //  assert(self == __cilkrts_get_tls_worker()->self);
  //  t_in_batch++;
  cilk_batchify(batch_relabel, (void*)ds, 0, sizeof(int));
  //  assert(self == __cilkrts_get_tls_worker()->self);
  //  assert(t_in_batch == 1);

  if (w->self == g_batch_owner_id) {
    g_batch_in_progress = 0;
    g_batch_owner_id = -1;
    for (int i = 0; i < __cilkrts_get_nworkers(); ++i) {
      pthread_spin_unlock(&g_worker_mutexes[i].mut);
    }
    //    fprintf(stderr, "Relabel lock owner is %d\n", g_relabel_mutex.owner->self);
    //pthread_spin_unlock(&g_relabel_mutex);
    __cilkrts_mutex_unlock(t_worker, &g_relabel_mutex);
  }
  //  t_in_batch--;
  //  assert(t_in_batch == 0);
  //  assert(self == __cilkrts_get_tls_worker()->self);
}

class omrd_t {
private:
  AtomicStack_t<tl_node*> m_heavy_nodes;
  om* m_ds;
  om_node* m_base;

  void add_heavy(om_node* base)
  {
    assert(g_batch_in_progress == 0);
    m_heavy_nodes.add(om_get_tl(base));
    om_assert(base->list->above->level == MAX_LEVEL);
  }

  om_node* try_insert(om_node* base = NULL)
  {
    assert(g_batch_in_progress == 0);
    om_node* n;
    if (base == NULL) n = om_insert_initial(m_ds);
    else {
      //      RDTOOL_INTERVAL_BEGIN(INSERT);
      n = om_insert(m_ds, base);
      //      RDTOOL_INTERVAL_END(INSERT);
    }

    if (base) om_assert(base->list->above);
    if (n) om_assert(n->list->above);
    return n;
  }

  om_node* slow_path(__cilkrts_worker *w, om_node* base)
  {
    //    RDTOOL_INTERVAL_BEGIN(SLOW_PATH);
    om_node *n = NULL;
    pthread_spinlock_t* mut = &g_worker_mutexes[w->self].mut;
    //fprintf(stderr, "slow_path: %d\n", w->self);
    /// Assert: mut is owned by self
    while (!n) {
      insert_failed = 1;
      if (!base->list->heavy && CAS(&base->list->heavy, 0, 1)) {
        // DBG_TRACE(DEBUG_BACKTRACE,
        //           "Could not fit into list of size %zu, not heavy.\n",
        //           base->list->size);
        add_heavy(base);
      }
      __cilkrts_set_batch_id(w);

      pthread_spin_unlock(mut);
      //fprintf(stderr, "Worker %d trying to start batch %zu\n", w->self, g_num_relabels);
      join_batch(w, this); // try to start
    
      insert_failed = 0; // Don't start a batch, just join if one has started.
      while (pthread_spin_trylock(mut) != 0) {
        //fprintf(stderr, "Worker %d joining batch %zu from slow_path\n", w->self, g_num_relabels);
        //fprintf(stderr, "join_batch worker: %d\n", w->self);
        join_batch(w, this);
      }

      n = try_insert(base);
      //      assert(n);
    }
    assert(insert_failed == 0);
    //    RDTOOL_INTERVAL_END(SLOW_PATH);
    return n;
  }

public:

  omrd_t()
  {
    m_ds = om_new();
    m_base = try_insert();
  }

  ~omrd_t()
  {
    om_free(m_ds);
  }

  om_node* get_base() { return m_base; }
  AtomicStack_t<tl_node*>* get_heavy_nodes() { return &m_heavy_nodes; }
  om* get_ds() { return m_ds; }

  om_node* insert(__cilkrts_worker* w, om_node* base)
  {
    //    RDTOOL_INTERVAL_BEGIN(FAST_PATH);
    pthread_spinlock_t* mut = &g_worker_mutexes[w->self].mut;
    while (pthread_spin_trylock(mut) != 0) {
      //fprintf(stderr, "Worker %d joining batch %zu from fast path\n", w->self, g_num_relabels);
      join_batch(w, this);
    }

    om_node* n = try_insert(base);
    //    RDTOOL_INTERVAL_END(FAST_PATH);
    if (!n) {
      n = slow_path(w, base);
      assert(!is_heavy(n));
    }
    assert(n->list == base->list);
    if (!n->list->heavy && is_heavy(n)) {
      if (CAS(&base->list->heavy, 0, 1)) {
        // DBG_TRACE(DEBUG_BACKTRACE,
        //           "List marked as heavy with %zu items.\n", n->list->size);
        add_heavy(base);
      }
    }
    pthread_spin_unlock(mut);
    //    RD_STATS(__sync_fetch_and_add(&g_num_inserts, 1));
    return n;
  }

  label_t set_heavy_threshold(label_t new_threshold)
  {
    label_t old = g_heavy_threshold;
    g_heavy_threshold = new_threshold;
    return old;
  }

  size_t memsize()
  {
    size_t ds_size = om_memsize(m_ds);
    size_t hnode_size = m_heavy_nodes.memsize();
    return ds_size + hnode_size + sizeof(void*) + sizeof(omrd_t);
  }
};

/// These are pointers so that we can control when we get
/// freed. Sometimes I want to access them after main(), in a special
/// program destructor.
omrd_t *g_english;
omrd_t *g_hebrew;

void relabel(omrd_t *_ds)
{
  om* ds = _ds->get_ds();
  //  om_verify(ds);
  AtomicStack_t<tl_node*> *heavy_nodes = _ds->get_heavy_nodes();
  if (!heavy_nodes->empty()) {
    om_relabel(ds, heavy_nodes->at(0), heavy_nodes->size());

    // Sequential only!
#if STATS > 0
    g_heavy_node_info[g_num_relabels] = heavy_nodes->size();
    g_num_relabels++;
#endif
    //    RD_STATS(__sync_fetch_and_add(&g_num_relabels, 1));
    //    RD_STATS(g_relabel_size += heavy_nodes->size());

    heavy_nodes->reset();
  } else {
    //    RD_STATS(__sync_fetch_and_add(&g_num_empty_relabels, 1));
  }
  //  om_verify(ds);
}

// We actually only need the DS.
/// @todo It would be great define batch functions as class methods...
void batch_relabel(void* _ds, void* data, size_t size, void* results)
{
  //  RD_STATS(DBG_TRACE(DEBUG_BACKTRACE, "Begin relabel %zu.\n", g_num_relabels));
  //fprintf(stderr, "start relabel\n");
  g_relabel_id++;
  asm volatile("": : :"memory");
  // fprintf(stderr, "Worker %d starting relabeling phase %zu\n", self, g_num_relabels);

  // omrd_t* ds = (omrd_t*)_ds;
  // relabel(ds);

  // You might as well try to relabel both structures...or should you?
  //  RDTOOL_INTERVAL_BEGIN(RELABEL);
  cilk_spawn relabel(g_english);
  relabel(g_hebrew);
  cilk_sync;
  g_relabel_id++;
  //  RDTOOL_INTERVAL_END(RELABEL);
  // fprintf(stderr, "Ending relabeling phase %zu\n", g_num_relabels-1);
}
