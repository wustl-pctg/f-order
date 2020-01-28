// Should be a reachability data structure ONLY
#pragma once
#include <cstdint>

#include "spbag.hpp"

namespace reach {
class structured {
public:

  struct sframe_data {
    spbag *Sbag, *Pbag;
    sframe_data() : Sbag(nullptr), Pbag(nullptr) { }
  };

  using smem_data = spbag;

  struct sfut_data {
    spbag *put_strand;
  };

  structured() {}
  //structured() = delete;
  structured(sframe_data *initial);
  void init(sframe_data *initial);
  ~structured();
  smem_data* active(sframe_data *f);

  // Parallelism creation
  void at_future_create(sframe_data *f);
  void at_spawn(sframe_data *f, sframe_data *helper);

  // Parallelism deletion
  void at_future_get(sframe_data *f, sfut_data*);
  void at_sync(sframe_data *f);

  // Continuations
  void at_spawn_continuation(sframe_data *f, sframe_data *p);
  void at_future_finish(sframe_data *f, sframe_data *p, sfut_data *fut);

  // Actually start a "strand" function that has already been "created"
  void begin_strand(sframe_data *f, sframe_data *p);
  bool precedes_now(sframe_data *f, smem_data *last_access);

private:
  // Helper function for parallelism creation (spawns + create_future)
  // Public for use by nonblock (should be friend class...)
  // ANGE: Actually no, create_strand was never called in nonblock, so it
  // never calls back to the structured one, either
  // ROB: create_strand is called from at_spawn and
  // at_future_create. These are called from nonblock.
  void create_strand(sframe_data *f);
  // Helper function for continuations
  void continuation(sframe_data *f, sframe_data *p);

#if STATS == 1
  uint64_t bags = 0;
  uint64_t merges = 0;
  uint64_t nullmerges = 0;
#endif

}; // class structured
} // namespace reach
