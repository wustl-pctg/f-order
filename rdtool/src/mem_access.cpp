#include <cassert>

#include "mem_access.h"

MemAccessList_t::MemAccessList_t(addr_t addr, bool is_read, 
                                 Current curr,
                                 addr_t rip, size_t mem_size) 
  : start_addr( ALIGN_BY_PREV_MAX_GRAIN_SIZE(addr) ) {

  // Should be brace-initialized in the class definition
  // for (int i{0}; i < NUM_SLOTS; ++i) {
  //   readers[i] = writers[i] = nullptr;
  // }

  const int start = ADDR_TO_MEM_INDEX(addr);
  const int grains = SIZE_TO_NUM_GRAINS(mem_size);
  assert(start >= 0 && start < NUM_SLOTS && (start + grains) <= NUM_SLOTS);

  if (is_read)
    for (int i{start}; i < (start + grains); ++i)
      readers[i] = new MemAccess_t{curr, rip};
  else
    for (int i{start}; i < (start + grains); ++i)
      writers[i] = new MemAccess_t{curr, rip};

  pthread_spin_init(&readers_lock, PTHREAD_PROCESS_PRIVATE);
  pthread_spin_init(&writers_lock, PTHREAD_PROCESS_PRIVATE);
}

MemAccessList_t::~MemAccessList_t() {
  for(int i=0; i < NUM_SLOTS; i++) {
    if(readers[i]) {
      delete readers[i];
      readers[i] = nullptr;
    }
  }
  
  for(int i=0; i < NUM_SLOTS; i++) {
    if(writers[i]) {
      delete writers[i];
      writers[i] = nullptr;
    }
  }

  pthread_spin_destroy(&readers_lock);
  pthread_spin_destroy(&writers_lock);
}
