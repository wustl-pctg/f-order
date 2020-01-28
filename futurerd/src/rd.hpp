// Race detection class and public API
#pragma once

#include "rd.h"

#include "reach.hpp"
#include "shadow_mem.hpp"
#include "shadow_stack.hpp"

// On the one hand, we only ever want one race detector anyway, so it
// seems silly to use a class with all static data. On the other hand,
// this makes initialization slightly easier when using a static
// library.
class race_detector {
public:
  static shadow_stack<sframe_data> t_sstack;
  static uint64_t t_stack_low_watermark;
  static bool t_clear_stack;
  static shadow_mem g_smem;
  static reach_ds g_reach;
  static bool tsan_init;

  static enum rd_policy g_policy;
  static size_t g_num_races;
  static int check_disabled;
  static bool shadow_enabled;

  // I use these as a return type as a simple way of knowing what kind
  // of race was found, so I start at 1.
  enum race_type { WW = 1, RW, WR};

  using access_t = shadow_mem::access_t;

  race_detector();
  ~race_detector();
  
  //void reset() = delete; // re-enter Cilk after a break? stack_low_watermark?
  //void print_state() = delete; // FILE *output = stdout

  static inline size_t num_races() { return g_num_races; }
  //static inline void enable_checking() { t_checking_disabled = false; }
  //static inline void disable_checking() { t_checking_disabled = true; }
  static void enable_checking();
  static void disable_checking();
  static inline void disable_shadowing() { shadow_enabled = false; }
  static inline bool should_check() { return check_disabled == 0 && shadow_enabled; }
  static void mark_stack_allocate(void* addr);

  // @todo{ Read environment variables for setting race-reporting policy.}
  static void set_policy(enum rd_policy p); // not thread-safe
  static void report_race(addr_t addr, addr_t last_rip, addr_t this_rip,
                          race_type rt);
  static void check_access(bool is_read, addr_t rip, addr_t addr, size_t mem_size);

  static smem_data* active();

private:
  static void handle_write(access_t* slot, addr_t rip, addr_t addr,
                    size_t mem_size, smem_data *current);
  static void handle_read(access_t* slot, addr_t rip, addr_t addr,
                    size_t mem_size, smem_data *current);
}; // struct race_detector
