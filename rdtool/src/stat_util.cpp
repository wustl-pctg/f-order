#include <internal/abi.h>
#include "stat_util.h"

__thread __cilkrts_worker* t_worker = NULL;
__thread int self = -1;

#if STATS > 1

// struct padded_time *g_timing_events;
// struct padded_time *g_timing_event_starts;
// struct padded_time *g_timing_event_ends;
// struct timespec EMPTY_TIME_POINT = {0,0};
struct padded_time *g_timing;
const uint64_t EMPTY_TIME_POINT = 0;

const char* g_interval_strings[NUM_INTERVAL_TYPES] = {
  NULL,
  // "Relabel time",
  // "Insert time",
  "Query time",
};
#endif

#if STATS > 0
// struct stats_s g_stats = {0};
unsigned long g_num_relabels = 0;
unsigned long g_num_inserts = 0;
unsigned long g_relabel_size = 0;

unsigned long g_num_reads;
unsigned long g_num_writes;
unsigned long g_num_queries;

size_t g_heavy_node_info[16384];

#endif
