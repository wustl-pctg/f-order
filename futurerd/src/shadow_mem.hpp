#pragma once
#include <cstdio>
#include "mem_access.hpp"

//template < typename mem >
class shadow_mem {
public:
  using access_t = MemAccessList_t;
  //using addr_t = access_t::addr_t;
  using rd_info_t = smem_data*;

  struct addr_info_t {
    access_t last_reader;
    access_t last_writer;
  };

private:
  struct shadow_tbl { access_t *shadow_entries[1<<LOG_TBL_SIZE]; };

  struct shadow_tbl **shadow_dir;

  access_t** find_slot(addr_t key, bool alloc);
  
public:
  shadow_mem();
  access_t* find(addr_t key);

  //  return the value at the memory location when insert occurs
  //  If the value returned != val, insertion failed because someone
  //  else got to the slot first.  
  access_t * insert(addr_t key, access_t *val);
  // void update(access_t *slot, bool is_read,
  //             addr_t addr, rd_info_t strand, addr_t rip);

  void erase(addr_t key);
  void clear(addr_t start, addr_t end);

};
