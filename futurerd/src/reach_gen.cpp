#include "reach_gen.hpp"
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <malloc.h>

using namespace reach;
using node = general::node;

// The 0th node is fake, for representing nodes in R
general::general() { add_node(); }

general::~general() {
  // Stats
#if STATS == 1
  fprintf(stderr, "---------- Reachability stats ----------\n");
#define FMT_STR ">>>%20.20s =\t%lu\n"
  fprintf(stderr, FMT_STR, "R nodes", data.size());
  fprintf(stderr, FMT_STR, "R edges (non-SP)", edges);

  //size_t max_in_count = 0UL, min_in_count = -1UL, sum_in_count = 0;
  size_t max_reach_count = 0UL, min_reach_count = -1UL, sum_reach_count = 0;
  //size_t max_block_count = 0UL, min_block_count = -1UL, sum_block_count = 0;
  for (int i = 1; i < data.size(); ++i) {
    bitset& ba = data[i];
    if (ba.reach_count > max_reach_count) max_reach_count = ba.reach_count;
    if (ba.reach_count < min_reach_count) min_reach_count = ba.reach_count;
    sum_reach_count += ba.reach_count;
  }

  fprintf(stderr, FMT_STR, "max reach count", max_reach_count);
  fprintf(stderr, FMT_STR, "min reach count", min_reach_count);
  fprintf(stderr, ">>>%20.20s =\t%lf\n", "avg reach count",
          ((double)sum_reach_count) / (double)data.size());

  size_t nb = 0;
  size_t maxnb = 0;
  size_t minnb = 0;
  for (int i = 0; i < data.size(); ++i) {
    size_t n = data[i].num_blocks;
    if (n > maxnb) maxnb = n;
    if (n < minnb) minnb = n;
    nb += n;
  }
  double bytes_used = ((double)nb) * sizeof(bitvector::block_t);
  char* units;
  if (bytes_used < 1024) units = "B";
  else if (bytes_used < 1024*1024) {
    units = "KB";
    bytes_used /= 1024.0;
  } else if (bytes_used < 1024*1024*1024) {
    units = "MB";
    bytes_used /= (1024.0*1024.0);
  } else {
    units = "GB";
    bytes_used /= (1024.0*1024.0*1024.0);
  }

  fprintf(stderr, ">>>%zu blocks = %lf %s for Rmat\n",
          nb, bytes_used, units);
  fprintf(stderr, ">>>min=%zu, max=%zu, avg=%lf, row=%zu\n",
          minnb, maxnb,
          ((double)nb)/((double)data.size()),
          data.size());
  fprintf(stderr, "-------------------------\n");
  malloc_stats();

#endif
}

node general::add_node() {
  node id = data.size();
  data.push_back(bitset());
  return id;
}

bool general::precedes(node x, node y) {
  if (x == y) return true;

  // y should definitely be in data
  assert(data.size() > y);

  return data[y].get(x);
}

// void general::add_edges_from(node* from_nodes, int num_from, node to) {

//   assert(data[to].data == nullptr);
  
//   size_t max_size = 0;
//   for (auto i = 0; i < num_from; ++i) {
//     size_t s = data[from_nodes[i]].num_blocks;
//     if (s > max_size) max_size = s;
//   }
//   data[to].unsafe_resize(max_size);

//   for (auto i = 0; i < max_size; ++i) {
//     bitset::block_t b = 0;
//     for (auto j = 0; j < num_from; ++j) {
//       node from = from_nodes[j];
//       if (i < data[from].num_blocks) b |= data[from].data[i];
//     }
//     data[to].data[i] = b;
//   }

// }


// Assumes to has no out edges yet.
void general::add_edge(node from, node to) {
  if (from == to) return;

  if (data[to].data != nullptr) {
    data[to].resize(data[from].num_blocks);

    for (auto i = 0; i < data[from].num_blocks; ++i)
      data[to].data[i] |= data[from].data[i];

  } else { // fast case, no other incoming edges

    data[to] = bitset(data[from]);
  }
  data[to].set(from);

  // In the current implementation, this is slow b/c the set requires
  // a merge also, meaning we need two separate merges.
  // TODO: make R[i,i] always true in the bitsums DS
  // Doing so allows us to only do one merge.
  // data[to].set(from);
  // if (data[to].data == nullptr)
  //   data[to] = data[from];
  // else
    // data[to].merge(data[from]);

#if STATS
  edges++;
#endif


}
