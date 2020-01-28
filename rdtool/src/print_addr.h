#ifndef PRINT_ADDR_H
#define PRINT_ADDR_H

#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

// The context in which the access is made; user = user code, 
// update = update methods for a reducer object; reduce = reduce method for 
// a reducer object 
/* enum AccContextType_t { USER = 1, UPDATE = 2, REDUCE = 3 }; */
// W stands for write, R stands for read
enum RaceType_t { RW_RACE = 1, WW_RACE = 2, WR_RACE = 3 };


typedef struct RaceInfo_t {
  uint64_t first_inst;   // instruction addr of the first access
  uint64_t second_inst;  // instruction addr of the second access
  uint64_t addr;         // addr of memory location that got raced on
  enum RaceType_t type; // type of race

  RaceInfo_t(uint64_t _first, uint64_t _second,
             uint64_t _addr, enum RaceType_t _type) :
    first_inst(_first), second_inst(_second), addr(_addr), type(_type)
    { }

  bool is_equivalent_race(const struct RaceInfo_t& other) const {
    /*
    if( (type == other.type && 
         first_inst == other.first_inst && second_inst == other.second_inst) ||
        (first_inst == other.second_inst && second_inst == other.first_inst &&
         ((type == RW_RACE && other.type == WR_RACE) ||
          (type == WR_RACE && other.type == RW_RACE))) ) {
      return true;
    } */
    // Angelina: It turns out that, Cilkscreen does not care about the race
    // types.  As long as the access instructions are the same, it's considered
    // as a duplicate.
    if( (first_inst == other.first_inst && second_inst == other.second_inst) ||
        (first_inst == other.second_inst && second_inst == other.first_inst) ) {
      return true;
    }
    return false;
  }
} RaceInfo_t;

void read_proc_maps(); 
void delete_proc_maps(); 
void print_race_report();
int get_num_races_found(); 
void report_race(uint64_t first_inst, uint64_t second_inst, 
                 uint64_t addr, enum RaceType_t race_type);

#if defined (__cplusplus)
}
#endif

#endif
