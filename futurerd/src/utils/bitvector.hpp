#include <cstring> // memcpy, memset
#include <cassert>
#include <cstdint>

struct bitvector {

  using block_t = unsigned int;
  static constexpr size_t BITS_PER_BLOCK = sizeof(block_t) * 8;
  static constexpr size_t DEFAULT_SIZE = 0;

  // Getting or setting a particular bit in a block
  //#define MASK(index) (1 << ((index) % sizeof(block_t)))
  static inline const size_t MASK(size_t index)
  { return (1 << (index % BITS_PER_BLOCK)); }

  size_t num_blocks = 0;
  block_t* data = nullptr;

  // Debugging
#if STATS == 1
  uint64_t reach_count = 0;
#endif

  bitvector() { resize(DEFAULT_SIZE); }
  bitvector(const bitvector& that);
  bitvector(bitvector&& that) = default;
  bitvector& operator=(const bitvector& src) = default;

  // Doesn't initialize newly allocated blocks
  void unsafe_resize(size_t new_size);

  void resize(size_t new_size);

  // Normal operations
  void set(size_t y);
  bool get(size_t y) const;

  //void check(size_t y, bool expected) const;

};
