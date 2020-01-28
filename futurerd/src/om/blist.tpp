#include <cassert>

namespace om {

// N / log_2 N = 2^58
static constexpr label_t NODE_INTERVAL = (((label_t)1) << 58);
static constexpr label_t SUBLIST_SIZE = ((label_t)64); // log_2 N = 64

template <class TLT>
typename blist<TLT>::node* blist<TLT>::insert(blist<TLT>::node *base)
{
  assert(base);
  if (base->label == MAX_LABEL
      || (base->next && base->label + 1 == base->next->label))
    return nullptr; // no room

  blist<TLT>::node* n = parent_list<TLT>::insert(base);
  n->ds = this;
  return n;
}

template <class TLT>
typename TLT::node* blist<TLT>::split()
{
  // size_t num_lists = 0;
  assert(this->m_head);

  //tl_node* tl = above();
  //tl_node* saved_next = tl->next;
  blist::node* n = this->m_head;
  
  while (n) {
    //label_t l = MIN_LABEL;
    //for (size_t sz = 0; sz < SUBLIST_SIZE && n; sz++, n = n->next()) {
      
    //}
    

  }
}

} // namespace om
