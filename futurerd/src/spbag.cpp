#include "spbag.hpp"
#include "utils/chunked_list.hpp"

#include <cassert>

void* spbag::operator new(std::size_t sz) {
  static utils::chunked_list<spbag> allocator;
  return (void*) allocator.get_next();
}

// void spbag::data_swap(spbag *x, spbag *y) {
//   bag_kind tmp;
//   tmp = x->kind;
//   x->kind = y->kind;
//   y->kind = tmp;
// }

// If we do merging by rank in union-find, I think we'll need to set
// the result to be the right kind - Sbag or Pbag.
void spbag::merge(spbag *that) {
  spbag *root = static_cast<spbag*>(utils::uf::merge(this,that));
  if (root != this) {
    assert(root == find(that));
    //data_swap(this,that);
    //data_swap(this, root);
    root->kind = this->kind;
  }
}
