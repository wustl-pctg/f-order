#include "bitvector.hpp"
#include <cstdint> // uint64_t
#include <cstdlib> // malloc
#include <cstdio>

__attribute__ ((const))
static inline uint64_t p2(uint64_t x)
{
  return uint64_t(1) << (64 - __builtin_clzl(x - 1));
}

bitvector::bitvector(const bitvector& that) {
  num_blocks = 0; // ensure resize really happens
  unsafe_resize(that.num_blocks);
  std::memcpy(data, that.data, num_blocks * sizeof(block_t));
}

void bitvector::unsafe_resize(size_t new_size) {
  size_t s = sizeof(block_t) * new_size;
  block_t* new_data = (block_t*) malloc(s);
  if (!new_data) printf("Failed on alloc of size %zu\n", s),
  assert(new_data);
  std::memcpy(new_data, data, sizeof(block_t) * num_blocks);
  free(data);
  data = new_data;
  num_blocks = new_size;
}

void bitvector::resize(size_t new_size) {
  if (new_size <= num_blocks) return;
  size_t old_size = num_blocks;
  unsafe_resize(new_size);
  std::memset(data + old_size, 0,
              (new_size - old_size) * sizeof(block_t));
}

void bitvector::set(size_t y) {
  uint64_t block = y / BITS_PER_BLOCK;
  // size_t new_size = num_blocks;
  // while (block >= new_size) new_size *= 2;
  // resize(new_size);
  if (block >= num_blocks) {
    size_t new_size = (block) ? p2(block+1) : 1;
    assert(block < new_size);
    resize(new_size);
  }
  assert(block < num_blocks);

  // Debugging
  block_t mask = MASK(y);
#if STATS == 1
  if (!(data[block] & mask)) reach_count++;
#endif
  data[block] |= mask;
}

bool bitvector::get(size_t y) const {
  size_t block = y / BITS_PER_BLOCK;
  if (block >= num_blocks) return false;
  return data[block] & MASK(y);
}

// void bitvector::check(size_t y, bool expected) const {
//   size_t block = y / BITS_PER_BLOCK;
//   if (block >= num_blocks) return;

//   bool result = (data[block] & MASK(y));
//   if (result != expected) {
//     printf("Expected %d but got %d\n", expected, !expected);
//     printf("Block %u of %zu\n", block, num_blocks);
//     printf("Block: %lu, mask %zu\n", data[block], MASK(y));
//     assert(false);
//   }
// }
