#include <cstddef>

namespace utils {

namespace uf {

struct node {
  static std::size_t global_index;

  std::size_t id;
  std::size_t rank; /// rank is roughly the height
  node* parent = this;

  node() : id(global_index++), rank(0) { }
}; // struct node

node* find(node* x);
node* merge(node* x,  node* y);

} // namespace uf

} // namespace utils
