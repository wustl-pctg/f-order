// This file serves only to choose the reachability algorithm in use.
// I'm very open to suggestions for better ways to do this...
#pragma once

#if defined STRUCTURED_FUTURES

#include "reach_structured.hpp"
using reach_ds = reach::structured;

#elif defined NONBLOCKING_FUTURES

#include "reach_nonblock.hpp"
using reach_ds = reach::nonblock;

#else
#error Unsupported future type
#endif

// Reachability data structure "interface" requirements
using sframe_data = reach_ds::sframe_data;
using smem_data = reach_ds::smem_data;
using sfut_data = reach_ds::sfut_data;
