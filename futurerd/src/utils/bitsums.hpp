#include <cstddef>
#include <cstdint>
#include <cstdlib> // malloc, b/c I don't want to pay for initialization
#include <cstring> // memcpy
#include <cassert>

// Is this worth it?
// What if we are unlikely to have long streaks of 1s?
// Then maybe we should just keep a sorted array of the "1" locations...
// You pay the overhead of keeping it sorted to get O(\log k) search time
// But it costs O(n) time to "squish" in this version anyway...

/** Answer:
    Yes, it is likely we have long streaks of 1s, since many tasks
    will be reachable from many previous tasks, especially when we're
    more than halfway through with a program. These tasks are likely
    to be near each other.
 */

struct bitsums {
  using count_t = uint64_t;
  using index_t = uint32_t;

  count_t* data;
  size_t size; // Number of count_t's, not number of "bits"
  size_t cap;

  bitsums() : data(nullptr), size(0), cap(0) {}

  // Our bitsets represent reachability, so they have an id and are
  // reachable from themselves.
  bitsums(index_t id) : size(2), cap(2) {
    data = (count_t*) malloc(sizeof(count_t) * cap);
    assert(data);
    data[0] = id;
    data[1] = id+1;
  }

  // Copy constructor
  // Const?
  bitsums(bitsums& that) : size(that.size), cap(that.cap) {
    data = (count_t*) malloc(sizeof(count_t) * cap);
    assert(data);
    std::memcpy(data, that.data, size * sizeof(count_t));
  }

  // Const?
  bitsums& operator=(bitsums& src) {
    if (this != &src) {
      // this->~bitsums();
      // new (this) bitsums(src);
      size = src.size; cap = src.cap;
      data = (count_t*) realloc(data, sizeof(count_t)*cap);
      std::memcpy(data, src.data, size * sizeof(count_t));
    }
    return *this;
  }

  bitsums(bitsums&& that) : data(that.data), size(that.size), cap(that.cap) {
    that.data = nullptr;
    that.size = that.cap = 0;
  }

  // Assumed to be a "new" node, meaning {id,id+1}
  // Using stack memory for mem will cause an abort in the
  // destructor. Need a flag.
  //bitsums(count_t* mem) : data(mem), size(2), cap(2) {}

  ~bitsums() {
    free(data);
    // paranoia
    size = 0;
    cap = 0;
  }

  // Adding the first edge to a vertex...
  // void init(index_t x) {
  //   assert(size == 2 && cap == 4);
  //   index_t id = data[0];
  //   assert(id != x);

  //   index_t mn = std::min(id, x);
  //   index_t mn = std::max(id, x);

  //   data[0] = mn;
  //   data[1] = mn+1;
  //   data[2] = mx-1;
  //   data[3] = mx;
  // }


  bool get(size_t x);
  void set(size_t x);
  void merge(bitsums& other);
  static void merge(bitsums& a, bitsums& b, bitsums& other);
  //void multimerge(bitsums** sets, size_t num_sets);

private:
  void squish();
  void resize(size_t new_size);
  void validate();
  index_t find(size_t x);
  void take_from(bitsums& o, size_t from);
}; // struct bitsums
