#include <cassert>
#include <cstdio> // debugging

#include "shadow_mem.hpp"
using access_t = shadow_mem::access_t;

access_t** shadow_mem::find_slot(addr_t key, bool alloc) {
  /* I think this volatile is necessary and sufficient ... */
  shadow_tbl *volatile *dest = &(shadow_dir[key>>LOG_TBL_SIZE]);
  shadow_tbl *tbl = *dest;

  if (!alloc && !tbl) return nullptr;
    
  if (tbl == nullptr) {
    struct shadow_tbl *new_tbl = new struct shadow_tbl();
    do {
      tbl = __sync_val_compare_and_swap(dest, tbl, new_tbl);
    } while(tbl == nullptr);
    assert(tbl != nullptr);

    if(tbl != new_tbl) { // someone got to the allocation first
      delete new_tbl; 
    }
  }
  access_t** slot =  &tbl->shadow_entries[key&((1<<LOG_TBL_SIZE) - 1)];
  return slot;
}

shadow_mem::shadow_mem() {
  shadow_dir = 
    new struct shadow_tbl *[1<<(48 - LOG_TBL_SIZE - LOG_KEY_SIZE)]();
}

access_t* shadow_mem::find(addr_t key) {
  access_t **slot = find_slot(key, false);
  if (slot == nullptr)
    return nullptr;
  return *slot;
}

//  return the value at the memory location when insert occurs
//  If the value returned != val, insertion failed because someone
//  else got to the slot first.  
access_t*  shadow_mem::insert(addr_t key, access_t *val) {
  access_t **slot = find_slot(key, true);
  access_t *old_val = *slot;
  *slot = val;
  return old_val;
}

void shadow_mem::erase(addr_t key) {
  access_t **slot = find_slot(key, false);
  if (slot != nullptr)
    *slot = nullptr;
}

// void shadow_mem::update(access_t *slot, bool is_read,
//                         addr_t addr, rd_info_t strand, addr_t rip) {
//   assert(slot != nullptr);
//   access_t *access = (is_read) ? &slot->last_reader : &slot->last_writer;
//   *access = access_t(strand, rip);
// }

void shadow_mem::clear(addr_t start, addr_t end) {
  assert(ALIGN_BY_NEXT_MAX_GRAIN_SIZE(end) == end);
  assert(start <= end);

  while(start != end) {
    erase(ADDR_TO_KEY(start));
    start += GRAIN_SIZE;
  }
}
