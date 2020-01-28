// Union-find data structure
// Pointer-based, for now. Does it matter if I use array-based
// instead?
#include "union_find.hpp"
#include "chunked_list.hpp"

//#include <cstdio>

namespace utils {

namespace uf {

std::size_t node::global_index = 0;

// always link a lower rank node to a higher rank node
// assuming rank works out, x <- y (link y into x)
node* link(node* x, node* y) {
  /*
  if (x == y) return x;
  if (y->rank > x->rank) return link(y,x);

  // guaranteed that x has higher rank than y
  y->parent = x;
  if (x->rank == y->rank) ++x->rank; // fix up the rank if necessary
  return x;
  */
  // ANGE XXX: For now let's just always link y into x
  y->parent = x;
  return x;
}

node* find(node* x) {
  /// cilksan does this by using a list rather than recursion, which
  /// may be faster.
  if (x->parent != x) 
    return x->parent = find(x->parent);
  return x->parent;
}

node* merge(node* x,  node* y) { return link(find(x), find(y)); }

} // namespace uf

} // namespace utils     
