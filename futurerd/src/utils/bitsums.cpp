/** A compressed bitset made for reachability computations.

    Conceptually, bs[i][j] means i is reachable FROM j.

    Stored as (inclusive) prefix sum of the counts of consecutive 0s
    and 1s, always starting with 0s.

    Example: 
    The "counts" for 11100000110000111 would be

    0, 3, 5, 2, 4, 3

    So we store

    0, 3, 8, 10, 14, 17
*/
#include "bitsums.hpp"

#include <algorithm> // min
#include <cstdio>

void bitsums::resize(size_t new_cap) {
  if (new_cap <= cap) return;
  count_t *new_data = (count_t*) malloc(sizeof(count_t) * new_cap);
  assert(new_data);
  std::memcpy(new_data, data, size * sizeof(count_t));
  free(data);
  cap = new_cap;
  data = new_data;
}

inline bitsums::index_t bitsums::find(size_t x) {
  if (x < data[0]) return false;
  // binary search
  index_t l = 0, r = size;
  while (l <= r) {
    index_t m = l + (r-l)/2;

    if (data[m] > x)
      r = m;
    else {// data[m] <= x
      if (data[m+1] > x) return m+1;
      l = m;
    }
  }

  assert(false);
}

bool bitsums::get(size_t x) {

  // The first count could be 0, but not any others.
  if (x >= data[size-1]) return false;

  index_t index = find(x);

  // if even, it's in a string of zeros
  bool result = ((index % 2) == 0) ? false : true;
  return result;
}

// Setting a single bit, the counts are just (x, 1) since we'll have
// x zeros and then a 1. So the prefix sum is (x, x+1)
void bitsums::set(size_t x) {

  if (!data) {
    assert(size == 0);
    resize(2);
    data[size++] = x;
    data[size++] = x+1;
    return;
  }

  // TODO: Stack allocate for performance?
  //count_t data[2] = {x, x+1};
  bitsums tmp(x);

  merge(tmp);
}

static inline bool even(unsigned int x) { return x % 2 == 0; }
static inline bool odd(unsigned int x) { return x % 2 == 1; }

// TODO: try to avoid using an extra bitset to merge one into another...
void bitsums::merge(bitsums& a, bitsums& b, bitsums& o) {
  // Assume array a, b, and o (the output).
  // The length is represented by a.size.
  // We use index ai, bi, and oi to index into a, b, and o respectively.

  o.resize(std::max(a.size, b.size));
  assert(o.size == 0);
  
  unsigned int ai,bi,oi;
  oi = ai = bi = 0;
  bool is_counting_zero = true;
  
  // if either a or b runs out, we break
  while(ai < a.size && bi < b.size) {
    
    if (is_counting_zero) {
      assert(even(ai) && even(bi) && even(oi));

      if (ai+1 < a.size && a.data[ai+1] <= b.data[bi]) {
        // zero count at b[bi] spans multiple counts starting at a[ai]
        o.data[oi++] = a.data[ai++];
        o.data[oi++] = a.data[ai++];
      } else if (bi+1 < b.size && b.data[bi+1] <= a.data[ai]) {
        // (Symmetric) zero count at a[ai] spans multiple counts
        // starting at b[bi]
        o.data[oi++] = b.data[bi++];
        o.data[oi++] = b.data[bi++];
      } else {

        // TODO: Can we get rid of this edge case by making sure we
        // always have an even number of entries? This may mean
        // keeping a 0 entry (for 1's) at the end.
        if (a.data[ai] < b.data[bi]) {
          // a has min, take a[ai] unless a runs out,
          // i.e. there are no 1s after the 0s in a[ai]
          // if (ai == a.size-1) o.data[oi] = a.data[ai];
          // else o.data[oi] = b.data[bi];
          if (ai == a.size-1) {
            printf("Strange case a\n");
            o.data[oi] = b.data[bi];
          }
          else o.data[oi] = a.data[ai];
        } else { // symmetric case
          // if (bi == b.size-1) o.data[oi] = b.data[bi];
          // else o.data[oi] = a.data[ai];
          if (bi == b.size-1) {
            printf("Strange case b\n");
            o.data[oi] = a.data[ai];
          }
          else o.data[oi] = b.data[bi];
        }
        // Update everything
        oi++; ai++; bi++;
        is_counting_zero = !is_counting_zero;
      }
      
    } else { // at the counting 1 case
      
      assert(odd(ai) && odd(bi) && odd(oi));

      if (ai+1 < a.size && a.data[ai+1] <= b.data[bi]) {
        // the one count at b[bi] spans multiple counts starting a[ai]
        ai += 2; // skip over the overlaping zeros and ones in a
      } else if (bi+1 < b.size && b.data[bi+1] <= a.data[ai]) {
        // the symmetric case
        bi += 2; // skip over the overlaping zeros and ones in b
      } else {
        // the regular case, and flip the flag
        o.data[oi++] = std::max(a.data[ai++], b.data[bi++]); 
        is_counting_zero = !is_counting_zero;
      }

    }
  }
  o.size = oi;

  // copy over the left over
  if (ai < a.size) o.take_from(a, ai);
  else if(bi < b.size) o.take_from(b, bi);

  o.squish();
    
  
  // just sanity check
  o.validate();
}

// TODO is it correct to just straight copy over? 
void bitsums::take_from(bitsums& o, size_t from) {
  assert(from < o.size);
  assert(size % 2 == from % 2);
  assert(data[size-1] <= o.data[from]);

  resize(size + (o.size - from));

  // while (from < o.size)
  //   data[size++] = o.data[from++];
  memcpy(&data[size], &o.data[from], sizeof(count_t) * (o.size - from));
  size += o.size - from;
}

void bitsums::validate() {
  for (unsigned int i{1}; i < size; ++i)
    assert(data[i-1] < data[i]);
}

// squish the counting ones that span multiple counts
void bitsums::squish() {
  assert(even(size));

  index_t curr = 2;

  for (index_t i = 2; i < size; i += 2) {
    // If we need to squish, just don't increment curr. We'll
    // eventually find something to go there and shift it.
    if (data[i-1] < data[i]) { // let's shift
      data[curr-1] = data[i-1];
      data[curr] = data[i];
      curr += 2;
    }
  }
  data[curr-1] = data[size-1];
  size = curr;
}

void bitsums::merge(bitsums& other) {
  if (other.size == 0) return;
  bitsums a(std::move(*this));
  merge(a, other, *this);
}

// static inline
// void minmax(bitsums** sets, size_t num_sets, size_t i,
//             bitsums::count_t& mn, bitsums::count_t& mx) {
//   mn = -1; // TODO
//   mx = 0;
//   for (unsigned int j = 0; j < num_sets; ++j) {
//     if (i < sets[j]->size)
//       mn = std::min(mn, sets[j]->data[i]);
//     if (i+1 < sets[j]->size)
//       mx = std::max(mx, sets[j]->data[i+1]);
//   }
// }

// void bitsums::multimerge(bitsums** sets, size_t num_sets) {

//   size_t max_size = 0;
//   for (int i = 0; i < num_sets; ++i)
//     if (sets[i]->size > max_size) max_size = sets[i]->size;

//   resize(max_size);

//   index_t i;
//   count_t mn, mx;
//   for (i = 0; i < data.size - 1; i += 2) {
//     minmax(sets, num_sets, i, mn, mx);

//     data[i] = std::min(mn, data[i]);
//     data[i+1] = std::max(mx, data[i+1]);
//   }
//   for (; i < max_size; i += 2) {
//     minmax(sets, num_sets, i, data[i], data[i+1]);
//   }
// }
