#pragma once
#include <cstdint>
#include "current.h"
//#include "reach.hpp"
#include "stat_util.h"
//using rd_info_t = smem_data*;
using addr_t = uint64_t;

#define LOG_KEY_SIZE  4
#define LOG_TBL_SIZE 20

// macro for address manipulation for shadow mem
#define ADDR_TO_KEY(addr) ((addr_t) ((addr_t)addr >> LOG_KEY_SIZE))

#define GRAIN_SIZE 4
#define LOG_GRAIN_SIZE 2
#define MAX_GRAIN_SIZE (1 << LOG_KEY_SIZE)
#define NUM_SLOTS (MAX_GRAIN_SIZE / GRAIN_SIZE)
 
// a mask that keeps all the bits set except for the least significant bits
// that represent the max grain size
#define MAX_GRAIN_MASK (~(addr_t)(MAX_GRAIN_SIZE-1))

// If the value is already divisible by MAX_GRAIN_SIZE, return the value; 
// otherwise return the previous / next value divisible by MAX_GRAIN_SIZE.
#define ALIGN_BY_PREV_MAX_GRAIN_SIZE(addr) ((addr_t) (addr & MAX_GRAIN_MASK))
#define ALIGN_BY_NEXT_MAX_GRAIN_SIZE(addr)                  \
  ((addr_t) ((addr+(MAX_GRAIN_SIZE-1)) & MAX_GRAIN_MASK))

// compute (addr % 16) / GRAIN_SIZE 
#define ADDR_TO_MEM_INDEX(addr)                                       \
  (((addr_t)addr & (addr_t)(MAX_GRAIN_SIZE-1)) >> LOG_GRAIN_SIZE)

// compute size / GRAIN_SIZE 
#define SIZE_TO_NUM_GRAINS(size) (size >> LOG_GRAIN_SIZE)
class MemAccess_t {
public:
  //rd_info_t rd;
  Current current;
  addr_t rip;
  MemAccess_t(Current c, addr_t r): current(c), rip(r) {}
};

class MemAccessList_t {
  //private:
public:
  addr_t start_addr;
  // XXX: Can we store the actual object here instead of a pointer to it?
  MemAccess_t* readers[NUM_SLOTS] = {};
  MemAccess_t* writers[NUM_SLOTS] = {};

  pthread_spinlock_t readers_lock;
  pthread_spinlock_t writers_lock;
public:
  MemAccessList_t(addr_t addr, bool is_read, Current current, addr_t rip, size_t mem_size);
  ~MemAccessList_t();

  // MemAccess_t** readers_start() { return &readers[0]; }
  // MemAccess_t** readers_end() { return &readers[NUM_SLOTS]; }
  // MemAccess_t** writers_start() { return &writers[0]; }
  // MemAccess_t** writers_end() { return &writers[NUM_SLOTS]; }

  // Someday:
  //typedef typename std::array<MemAccess_t*>::iterator iterator;
  //typedef typename std::array<MemAccess_t*>::const_iterator const_iterator;

  //iterator begin() {return m_data.begin();}
  //const_iterator begin() const {return m_data.begin();}
  //const_iterator cbegin() const {return m_data.cbegin();}
  //iterator end() {return m_data.end();}
  //const_iterator end() const {return m_data.end();}
  //const_iterator cend() const {return m_data.cend();}
  
}; // enc class MemAccessList_t
