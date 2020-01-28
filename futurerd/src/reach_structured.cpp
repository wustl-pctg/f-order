#include "reach_structured.hpp"
#include <cassert>
#include <cstdio>

//#define LOG fprintf(stderr, "In %s\n", __PRETTY_FUNCTION__);
#define LOG

#if STATS == 1
#define STAT(stmt) stmt
#else
#define STAT(stmt)
#endif

namespace reach {

structured::structured(sframe_data *initial) { init(initial); }

void structured::init(sframe_data *initial) { create_strand(initial); }

structured::smem_data* structured::active(sframe_data *f) { return f->Sbag; }

/********** Parallelism Creation **********/
// p is the parent (caller), and f is the current strand
void structured::begin_strand(sframe_data *f, sframe_data *p) {
  f->Sbag = p->Sbag;
  f->Pbag = nullptr;
}

void structured::create_strand(sframe_data *f) {
  STAT(bags++);
  f->Sbag = new spbag(spbag::bag_kind::S);
  f->Pbag = nullptr; // will create this lazily when we need it
}

void structured::at_future_create(sframe_data *f) { create_strand(f); }
// f is the Cilk function spawning, which called the helper
void structured::at_spawn(sframe_data *f, sframe_data *helper)
{ LOG; create_strand(helper); }

/********** Parallelism Deletion **********/
void structured::at_future_get(sframe_data *f, sfut_data *fut) {
  assert(fut->put_strand);
  STAT(merges++);
  f->Sbag->merge(fut->put_strand);
}

// A sync is equivalent to calling "get" on all the futures that were
// "spawned" in this block
void structured::at_sync(sframe_data *f) { LOG;
  // make sure it's a real sync (XXX: ???)
  if (!f->Pbag) return;

  STAT(merges++);
  f->Sbag->merge(f->Pbag);
  spbag::find(f->Sbag)->set_kind(spbag::bag_kind::S);
  f->Pbag = nullptr;
}

bool structured::precedes_now(sframe_data *curr, smem_data *last_access) {
  //return last_access->precedes_now();
  bool res = last_access->precedes_now();
  // if (!res)
  //   std::cerr << "Race between " << last_access->id << " and " << curr->Sbag->id << std::endl;
  return res;
}

/********** Continuations **********/
// Returning from a spawned function or future function
// f is the spawned/created frame, p is the parent frame
void structured::continuation(sframe_data *f, sframe_data *p) {
  assert(f->Sbag);

  if (!p->Pbag) {
    STAT(nullmerges++);
    p->Pbag = f->Sbag;
  } else {
    STAT(merges++);
    p->Pbag->merge(f->Sbag);
  }
  // sometimes this bag may have been merged with an S-bag, so it
  // think it's an Sbag. But now it must be a Pbag. See basic/test6.
  // XXX: is there a cleaner way to do this?
  spbag::find(p->Pbag)->set_kind(spbag::bag_kind::P);
  //p->Pbag->set_kind(spbag::bag_kind::P);
}

// f: helper of the previous spawn, p: parent
void structured::at_spawn_continuation(sframe_data *f, sframe_data *p)
{ LOG; continuation(f,p); }

void structured::at_future_finish(sframe_data *f, sframe_data *p, sfut_data *fut) {
  fut->put_strand = f->Sbag;
  spbag::find(f->Sbag)->set_kind(spbag::bag_kind::P);
}

structured::~structured() {
#if STATS == 1
  fprintf(stderr, "---------- Structured Stats ----------\n");
#define FMT_STR ">>>%20.20s =\t%lu\n"
  fprintf(stderr, FMT_STR, "bags created", bags);
  fprintf(stderr, FMT_STR, "merges", merges);
  fprintf(stderr, FMT_STR, "'null' merges", nullmerges);
  fprintf(stderr, "-------------------------\n");
#endif
}

} // namespace reach
