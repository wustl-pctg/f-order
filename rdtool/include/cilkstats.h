// Public interface to cilkstats tool
#ifndef _CILKSTATS_H
#define _CILKSTATS_H

// Manual control over memory checking.
void __cilkstats_disable_checking();
void __cilkstats_enable_checking();

// Getting stat values, just in case.
size_t __cilkstats_get_num_reads();
size_t __cilkstats_get_num_writes();
size_t __cilkstats_get_num_mem_accesses();
size_t __cilkstats_get_num_spawns();
size_t __cilkstats_get_num_syncs();
size_t __cilkstats_get_num_cilk_functions();

#endif // ifndef _CILKSTATS_H
