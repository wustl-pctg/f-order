#ifndef _STAT_UTIL_H
#define _STAT_UTIL_H
#include <cilk/cilk.h>
#include <internal/abi.h>
#include <cpuid.h>

extern __thread __cilkrts_worker* t_worker;
extern __thread int self;

typedef enum {
  TOOL = 0,
  /* TOOL_INSERT, */
  /* FAST_PATH, */
  /* SLOW_PATH, */
  /* RELABEL_LOCK, */
  //  RELABEL,
  //  INSERT,
  QUERY,
  NUM_INTERVAL_TYPES,
} interval_types;

extern const char* g_interval_strings[NUM_INTERVAL_TYPES];

#if STATS > 1
#include <time.h>

/* struct padded_time { */
/*   struct timespec t; */
/*   char pad[64 - sizeof(struct timespec)]; */
/* }; */
/* extern struct padded_time *g_timing_events; */
/* extern struct padded_time *g_timing_event_starts; */
/* extern struct padded_time *g_timing_event_ends; */
/* extern struct timespec EMPTY_TIME_POINT; */
/* #define INTERVAL_CAST(x) x */
#define TIDX(i) ((NUM_INTERVAL_TYPES * self) + (i))
/* #define RDTOOL_INTERVAL_BEGIN(i) \ */
/*   clock_gettime(CLOCK_MONOTONIC, &g_timing_event_starts[TIDX(i)].t); */
/* #define RDTOOL_INTERVAL_END(i) \ */
/*   clock_gettime(CLOCK_MONOTONIC, &g_timing_event_ends[TIDX(i)].t); \ */
/*   g_timing_events[TIDX(i)].t.tv_sec += g_timing_event_ends[TIDX(i)].t.tv_sec - g_timing_event_starts[TIDX(i)].t.tv_sec; \ */
/*   g_timing_events[TIDX(i)].t.tv_nsec += g_timing_event_ends[TIDX(i)].t.tv_nsec - g_timing_event_starts[TIDX(i)].t.tv_nsec - 26; \ */
/*   g_timing_event_starts[TIDX(i)].t = EMPTY_TIME_POINT; */
/* #define GET_TIME_MS(i) (g_timing_events[TIDX(itype)].t.tv_sec * 1000.0L) + \ */
/*   (g_timing_events[TIDX(itype)].t.tv_nsec / 1000000.0L); */

#define CPU_MHZ 2394.146
static __attribute__((always_inline))
unsigned long long rdtsc(void)
{
  unsigned hi, lo;
  __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
  __get_cpuid_max(0, NULL);
  return (unsigned long long)lo + (((unsigned long long)hi) << 32);
}

struct padded_time {
  uint64_t start;
  uint64_t elapsed;
  /* char pad[64 - (2*sizeof(uint64_t))]; */
};
extern struct padded_time *g_timing;
extern const uint64_t EMPTY_TIME_POINT;
#define RDTOOL_INTERVAL_BEGIN(i) g_timing[TIDX(i)].start = rdtsc();
#define RDTOOL_INTERVAL_END(i) g_timing[TIDX(i)].elapsed += rdtsc() - g_timing[TIDX(i)].start;
#define GET_TIME_MS(i) g_timing[TIDX(i)].elapsed / (CPU_MHZ * 1000)

#else // no stats
#define RDTOOL_INTERVAL_BEGIN(i)
#define RDTOOL_INTERVAL_END(i)
#endif // if STATS > 1

#if STATS > 0
/* struct padded_stat { */
/*   size_t s; */
/*   char pad[64 - sizeof(size_t)]; */
/* }; */
/* struct stats_s { */
/*   padded_stat num_relabels; */
/*   padded_stat num_inserts; */
/*   padded_stat num_reads; */
/*   padded_stat num_writes; */
/*   padded_stat num_queries; */
/* }; */
/* extern struct stats_s g_stats; */
/* #define GET_STAT(x) g_stats.x.s; */
/* #define SET_STAT(x, y) g_stats.x.s = y; */
/* #define INCR_STAT(x, i) g_stats.x.s += i; */
extern unsigned long g_num_relabels;
extern unsigned long g_num_inserts;
extern unsigned long g_num_reads;
extern unsigned long g_num_writes;
extern unsigned long g_num_queries;
extern unsigned long g_relabel_size;
extern size_t g_heavy_node_info[16384];
#endif

#if STATS > 0
#define RD_STATS(...) __VA_ARGS__
#else
#define RD_STATS(...)
#endif


#endif // _STAT_UTIL_H
