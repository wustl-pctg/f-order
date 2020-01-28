// A single-level order-maintenance data structure implemented as a
// linked list. Inserts cannot fail.
#pragma once
#include "common.hpp"

namespace om {

template <class node = basic_node>
class list {
protected:
  node* m_head;
  
  static label_t next_label(const node* base);
  
public:
  list(label_t initial_label = MIN_LABEL)
    : m_head(new node(initial_label)) {}
  list(node *n, label_t initial_label = MIN_LABEL)
  : m_head(new(n) node(initial_label)) { assert(m_head); }
  ~list() {} // not necessary

  inline node* first() const { return m_head; }
  static inline bool precedes(const node* x, const node* y)
  { return x->label < y->label; }
  static inline void splice(basic_node* x, basic_node* y)
  { y->next = x->next; x->next = y; }

  node* insert(node* base);
}; // class om_list

} // namespace om

#include "list.tpp"
