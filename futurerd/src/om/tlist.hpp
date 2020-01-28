// The top-level of an order-maintenance data structure, implemented
// as a linked list.
#include "list.hpp"

namespace om {

using tl_node = om::basic_node;

class tlist : public list<tl_node> {
public:
  using node = tl_node;
  tlist() {}
  ~tlist() {} // not needed yet
  void relabel();
  void insert_many(tl_node* start);
}; // class tlist

} // namespace om

#include "tlist.tpp"
