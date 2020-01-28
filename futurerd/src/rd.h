// Public API
#pragma once

enum rd_policy {
  RD_EXIT = 0,
  RD_CONTINUE = 1, // *print* and continue
  RD_SILENT = 2,
  RD_LAST_POLICY,
};

extern "C" {

#include <stddef.h> // size_t

// Not thread-safe
void futurerd_set_policy(enum rd_policy p);
size_t futurerd_num_races();
void futurerd_enable_checking();
void futurerd_disable_checking();
void futurerd_disable_shadowing();
void futurerd_should_check();
void futurerd_mark_stack_allocate(void* addr);

} // extern C
