#ifndef _CHECK_MEM_H_
#define _CHECK_MEM_H_

#include <cstdint>
#include "shadow_mem.h"
#include "mem_access.h"
#include "current.h"
using addr_t = uint64_t;
void check_access(bool is_read, addr_t rip, addr_t addr, size_t mem_size);

extern ShadowMem<MemAccessList_t> shadow_mem;
extern __thread Current current;
bool Precedes(Current a, Current b);
extern volatile size_t g_relabel_id;

#define QUERY_START { size_t relabel_id = 0; \
    do { relabel_id = g_relabel_id; 
#define QUERY_END } while ( !( (relabel_id & 0x1) == 0 &&     \
                               relabel_id == g_relabel_id)); }  

#endif
